#include "llmresponse_translator.h"

#include "llmresponse_engine.h"
#include <rime/segmentation.h>
#include <rime/translation.h>

namespace rime {

LLMResponseTranslator::LLMResponseTranslator(const Ticket& ticket,
                                             an<LLMResponseEngine> engine)
    : Translator(ticket), engine_(engine) {}

an<Translation> LLMResponseTranslator::Query(const string& input,
                                             const Segment& segment) {
  if (!engine_ || !segment.HasTag("llmresponse")) {
    return nullptr;
  }
  return engine_->Translate(segment);
}

LLMResponseTranslatorComponent::LLMResponseTranslatorComponent(
    an<LLMResponseEngineComponent> engine_factory)
    : engine_factory_(engine_factory) {}

LLMResponseTranslatorComponent::~LLMResponseTranslatorComponent() {}

LLMResponseTranslator* LLMResponseTranslatorComponent::Create(
    const Ticket& ticket) {
  return new LLMResponseTranslator(ticket, engine_factory_->GetInstance(ticket));
}

}  // namespace rime
