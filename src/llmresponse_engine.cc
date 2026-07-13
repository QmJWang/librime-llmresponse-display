#include "llmresponse_engine.h"

#include <algorithm>
#include <filesystem>
#include <rime/candidate.h>
#include <rime/schema.h>
#include <rime/segmentation.h>
#include <rime/ticket.h>
#include <rime/translation.h>

namespace rime {

namespace {

const size_t kHistoryLimit = 20;

}  // namespace

LLMResponseEngine::LLMResponseEngine(const string& output_dir)
    : output_dir_(output_dir) {
  ScanPending();
}

LLMResponseEngine::~LLMResponseEngine() {}

void LLMResponseEngine::ScanPending() {
  // Preserve the currently viewed file across rescans if it still exists.
  path current =
      (!paths_.empty() && index_ < paths_.size()) ? paths_[index_] : path();

  vector<path> new_paths;
  std::error_code ec;
  path candidate_root = path(output_dir_) / "candidate";
  if (!output_dir_.empty() && std::filesystem::is_directory(candidate_root, ec)) {
    for (const auto& contact_entry :
         std::filesystem::directory_iterator(candidate_root, ec)) {
      if (!contact_entry.is_directory()) continue;
      std::error_code ec2;
      for (const auto& file_entry :
           std::filesystem::directory_iterator(contact_entry.path(), ec2)) {
        if (file_entry.path().extension() == ".json") {
          new_paths.push_back(file_entry.path());
        }
      }
    }
  }
  // Filenames are "<ms-timestamp>_<uuid8>.json"; lexical sort ~= chronological order.
  std::sort(new_paths.begin(), new_paths.end());

  paths_ = std::move(new_paths);
  index_ = 0;
  if (!current.empty()) {
    auto it = std::find(paths_.begin(), paths_.end(), current);
    if (it != paths_.end()) {
      index_ = static_cast<size_t>(std::distance(paths_.begin(), it));
    }
  }
  SyncCurrent();
}

void LLMResponseEngine::Clear() {
  if (paths_.empty() || index_ >= paths_.size()) return;
  path current = paths_[index_];

  // <output_dir>/candidate/<contact>/xxx.json -> <output_dir>/history/<contact>/xxx.json
  string contact = current.parent_path().filename().string();
  path output_root = current.parent_path().parent_path().parent_path();
  path history_dir = output_root / "history" / contact;
  std::error_code ec;
  std::filesystem::create_directories(history_dir, ec);

  path dest = history_dir / current.filename();
  std::filesystem::rename(current, dest, ec);
  if (ec) {
    // rename() failed (e.g. cross-volume move); fall back to copy + delete.
    ec.clear();
    std::filesystem::copy_file(
        current, dest, std::filesystem::copy_options::overwrite_existing, ec);
    std::filesystem::remove(current, ec);
  }

  // Cap history size; oldest files (by filename timestamp prefix) go first.
  vector<path> history_files;
  std::error_code ec3;
  for (const auto& entry :
       std::filesystem::directory_iterator(history_dir, ec3)) {
    if (entry.path().extension() == ".json") {
      history_files.push_back(entry.path());
    }
  }
  std::sort(history_files.begin(), history_files.end());
  while (history_files.size() > kHistoryLimit) {
    std::error_code ec4;
    std::filesystem::remove(history_files.front(), ec4);
    history_files.erase(history_files.begin());
  }

  paths_.erase(paths_.begin() + index_);
  if (index_ >= paths_.size() && index_ > 0) {
    index_ = paths_.size() - 1;
  }
  SyncCurrent();
}

an<Translation> LLMResponseEngine::Translate(const Segment& segment) const {
  const auto& entries = candidates();
  if (entries.empty()) return nullptr;

  auto translation = New<FifoTranslation>();
  const string& current_title = title();
  for (const auto& entry : entries) {
    translation->Append(New<SimpleCandidate>(
        "llmresponse", segment.start, segment.end, entry.text, entry.label,
        current_title));
  }
  return translation;
}

LLMResponseEngineComponent::LLMResponseEngineComponent() {}

LLMResponseEngineComponent::~LLMResponseEngineComponent() {}

LLMResponseEngine* LLMResponseEngineComponent::Create(const Ticket& ticket) {
  // Config always lives under the fixed "llmresponse" key, independent of
  // which component's ticket triggers Create() first.
  string output_dir;
  if (auto* schema = ticket.schema) {
    schema->config()->GetString("llmresponse/output_dir", &output_dir);
  }
  return new LLMResponseEngine(output_dir);
}

an<LLMResponseEngine> LLMResponseEngineComponent::GetInstance(
    const Ticket& ticket) {
  if (Schema* schema = ticket.schema) {
    auto found = engine_by_schema_id_.find(schema->schema_id());
    if (found != engine_by_schema_id_.end()) {
      if (auto instance = found->second.lock()) {
        return instance;
      }
    }
    an<LLMResponseEngine> new_instance{Create(ticket)};
    if (new_instance) {
      engine_by_schema_id_[schema->schema_id()] = new_instance;
      return new_instance;
    }
  }
  return nullptr;
}

}  // namespace rime
