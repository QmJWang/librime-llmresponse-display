#ifndef RIME_LLMRESPONSE_DB_H_
#define RIME_LLMRESPONSE_DB_H_

#include <rime/common.h>

namespace rime {

namespace llmresponse {

// One candidate reply for the pending title (the other party's message).
struct CandidateEntry {
  string text;    // Full reply text.
  string label;
  string angle;
};

}  // namespace llmresponse

// Loads a single contact's candidate JSON, reparsing lazily based on mtime.
// Each file represents one pending message: a title plus up to 8 replies.
class LLMResponseDb {
 public:
  // Returns whether candidates are available (false clears state on missing
  // file or parse failure).
  bool MaybeReload(const path& file_path);

  // Clears current content, e.g. when there is no pending file to point to.
  void Reset() {
    last_path_.clear();
    title_.clear();
    is_question_ = false;
    candidates_.clear();
  }

  const string& title() const { return title_; }
  bool is_question() const { return is_question_; }
  const vector<llmresponse::CandidateEntry>& candidates() const {
    return candidates_;
  }

 private:
  path last_path_;
  std::filesystem::file_time_type last_mtime_{};

  string title_;
  bool is_question_ = false;
  vector<llmresponse::CandidateEntry> candidates_;
};

}  // namespace rime

#endif  // RIME_LLMRESPONSE_DB_H_
