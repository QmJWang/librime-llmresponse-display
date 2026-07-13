#include "llmresponse_processor.h"

#include <rime/candidate.h>
#include <rime/composition.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/schema.h>
#include <rime/segmentation.h>

#include "llmresponse_constants.h"
#include "llmresponse_engine.h"

namespace rime {

LLMResponseProcessor::LLMResponseProcessor(const Ticket& ticket,
                                           an<LLMResponseEngine> engine)
    : Processor(ticket), llm_engine_(engine) {
  trigger_key_.Parse("Control+r");  // Default; overridable via schema.
  if (ticket.schema) {
    string trigger_key_repr;
    if (ticket.schema->config()->GetString(name_space_ + "/trigger_key",
                                            &trigger_key_repr)) {
      trigger_key_.Parse(trigger_key_repr);
    }
  }
  if (engine_) {
    commit_connection_ = engine_->context()->commit_notifier().connect(
        [this](Context* ctx) { OnCommit(ctx); });
  }
}

LLMResponseProcessor::~LLMResponseProcessor() {
  commit_connection_.disconnect();
}

bool LLMResponseProcessor::Showing(Context* ctx) const {
  return ctx && !ctx->composition().empty() &&
         ctx->composition().back().HasTag("llmresponse");
}

ProcessResult LLMResponseProcessor::ProcessKeyEvent(
    const KeyEvent& key_event) {
  if (!engine_ || !llm_engine_) {
    return kNoop;
  }
  if (key_event.release()) {
    // Handle key-down only, matching other processors (e.g. selector.cc).
    return kNoop;
  }
  Context* ctx = engine_->context();

  if (key_event == trigger_key_) {
    llm_engine_->ScanPending();
    if (llm_engine_->PendingCount() == 0) {
      return kNoop;  // Nothing pending; let normal typing proceed.
    }
    ctx->PushInput(llmresponse::kTriggerSentinel);
    return kAccepted;
  }

  if (Showing(ctx) && key_event.keycode() == XK_Tab) {
    if (key_event.shift()) {
      llm_engine_->Prev();
    } else {
      llm_engine_->Next();
    }
    ctx->RefreshNonConfirmedComposition();  // Re-segment/translate to refresh candidates.
    return kAccepted;
  }

  return kNoop;
}

void LLMResponseProcessor::OnCommit(Context* ctx) {
  if (!ctx || !llm_engine_ || ctx->composition().empty()) return;
  auto candidate = ctx->composition().back().GetSelectedCandidate();
  if (candidate && candidate->type() == "llmresponse") {
    llm_engine_->Clear();
  }
}

LLMResponseProcessorComponent::LLMResponseProcessorComponent(
    an<LLMResponseEngineComponent> engine_factory)
    : engine_factory_(engine_factory) {}

LLMResponseProcessorComponent::~LLMResponseProcessorComponent() {}

LLMResponseProcessor* LLMResponseProcessorComponent::Create(
    const Ticket& ticket) {
  return new LLMResponseProcessor(ticket, engine_factory_->GetInstance(ticket));
}

}  // namespace rime
