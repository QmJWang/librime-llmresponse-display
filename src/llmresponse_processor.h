#ifndef RIME_LLMRESPONSE_PROCESSOR_H_
#define RIME_LLMRESPONSE_PROCESSOR_H_

#include <rime/common.h>
#include <rime/key_event.h>
#include <rime/processor.h>

namespace rime {

class Context;
class LLMResponseEngine;
class LLMResponseEngineComponent;

// Handles three things:
//   1. On trigger key (schema-configurable, default Ctrl+R): rescan the
//      pending list and push the sentinel string into context input if
//      anything is pending (LLMResponseSegmentor tags the resulting segment).
//   2. While showing: Tab / Shift+Tab cycle llm_engine_ via Next()/Prev().
//   3. On Context::commit_notifier(): if the committed candidate's type is
//      "llmresponse", archive it via llm_engine_->Clear().
class LLMResponseProcessor : public Processor {
 public:
  LLMResponseProcessor(const Ticket& ticket, an<LLMResponseEngine> engine);
  virtual ~LLMResponseProcessor();

  ProcessResult ProcessKeyEvent(const KeyEvent& key_event) override;

 protected:
  void OnCommit(Context* ctx);

 private:
  bool Showing(Context* ctx) const;

  // Named llm_engine_, not engine_: Processor already declares a member
  // Engine* engine_ (the session engine).
  an<LLMResponseEngine> llm_engine_;
  KeyEvent trigger_key_;
  connection commit_connection_;
};

class LLMResponseProcessorComponent : public LLMResponseProcessor::Component {
 public:
  explicit LLMResponseProcessorComponent(
      an<LLMResponseEngineComponent> engine_factory);
  virtual ~LLMResponseProcessorComponent();

  LLMResponseProcessor* Create(const Ticket& ticket) override;

 protected:
  an<LLMResponseEngineComponent> engine_factory_;
};

}  // namespace rime

#endif  // RIME_LLMRESPONSE_PROCESSOR_H_
