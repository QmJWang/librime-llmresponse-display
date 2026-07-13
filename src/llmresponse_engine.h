#ifndef RIME_LLMRESPONSE_ENGINE_H_
#define RIME_LLMRESPONSE_ENGINE_H_

#include "llmresponse_db.h"
#include <rime/component.h>
#include <rime/common.h>

namespace rime {

class Context;
struct Segment;
struct Ticket;
class Translation;

// One instance shared per schema; Processor and Translator both operate on
// the same pending-list/index_ state.
class LLMResponseEngine : public Class<LLMResponseEngine, const Ticket&> {
 public:
  explicit LLMResponseEngine(const string& output_dir);
  virtual ~LLMResponseEngine();

  void ScanPending();  // Scans <output_dir>/candidate/*/*.json into paths_.
  void Clear();  // Archives the selected reply into history (cap: 20).

  void Next() {
    if (paths_.empty()) return;
    index_ = (index_ + 1) % paths_.size();
    SyncCurrent();
  }
  void Prev() {
    if (paths_.empty()) return;
    index_ = (index_ + paths_.size() - 1) % paths_.size();
    SyncCurrent();
  }

  an<Translation> Translate(const Segment& segment) const;

  const string& title() const { return db_.title(); }
  bool is_question() const { return db_.is_question(); }
  const vector<llmresponse::CandidateEntry>& candidates() const {
    return db_.candidates();
  }

  // Number of pending contacts (Tab list length), not the candidate count
  // for the current one (see candidates(), capped at 8).
  size_t PendingCount() const { return paths_.size(); }

 private:
  void SyncCurrent() {
    if (!paths_.empty()) {
      db_.MaybeReload(paths_[index_]);
    } else {
      db_.Reset();
    }
  }

  string output_dir_;
  vector<path> paths_;
  size_t index_ = 0;
  LLMResponseDb db_;  // Persistent member; reparses lazily via its own mtime check.
};

class LLMResponseEngineComponent : public LLMResponseEngine::Component {
 public:
  LLMResponseEngineComponent();
  virtual ~LLMResponseEngineComponent();

  LLMResponseEngine* Create(const Ticket& ticket) override;

  an<LLMResponseEngine> GetInstance(const Ticket& ticket);

 protected:
  map<string, weak<LLMResponseEngine>> engine_by_schema_id_;
};

}  // namespace rime

#endif  // RIME_LLMRESPONSE_ENGINE_H_
