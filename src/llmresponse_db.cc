#include "llmresponse_db.h"

#include <rime/config.h>

namespace rime {

bool LLMResponseDb::MaybeReload(const path& file_path) {
  std::error_code ec;
  auto mtime = std::filesystem::last_write_time(file_path, ec);
  if (ec) {
    // File missing (e.g. already processed/moved).
    Reset();
    return false;
  }

  if (file_path == last_path_ && mtime == last_mtime_) {
    // Unchanged; reuse cached content.
    return !candidates_.empty();
  }

  Config config;
  if (!config.LoadFromFile(file_path)) {
    return false;
  }

  string title;
  bool is_question = false;
  config.GetString("title", &title);
  config.GetBool("is_question", &is_question);

  vector<llmresponse::CandidateEntry> candidates;
  if (auto list = config.GetList("responses")) {
    for (size_t i = 0; i < list->size(); ++i) {
      auto item = As<ConfigMap>(list->GetAt(i));
      if (!item) continue;
      llmresponse::CandidateEntry entry;
      if (auto v = item->GetValue("text")) entry.text = v->str();
      if (auto v = item->GetValue("label")) entry.label = v->str();
      if (auto v = item->GetValue("angle")) entry.angle = v->str();
      candidates.push_back(std::move(entry));
    }
  }

  if (title.empty() && candidates.empty()) {
    // Incomplete content (e.g. file still being written); treat as failure.
    return false;
  }

  title_ = std::move(title);
  is_question_ = is_question;
  candidates_ = std::move(candidates);
  last_path_ = file_path;
  last_mtime_ = mtime;
  return !candidates_.empty();
}

}  // namespace rime
