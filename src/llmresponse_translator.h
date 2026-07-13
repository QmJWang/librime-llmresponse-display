#ifndef RIME_LLMRESPONSE_TRANSLATOR_H_
#define RIME_LLMRESPONSE_TRANSLATOR_H_

#include <rime/translator.h>

namespace rime {

class LLMResponseEngine;
class LLMResponseEngineComponent;

// Thin wrapper: only active on "llmresponse"-tagged segments; display logic
// is delegated to the shared LLMResponseEngine::Translate().
class LLMResponseTranslator : public Translator {
 public:
  LLMResponseTranslator(const Ticket& ticket, an<LLMResponseEngine> engine);

  an<Translation> Query(const string& input, const Segment& segment) override;

 private:
  an<LLMResponseEngine> engine_;
};

class LLMResponseTranslatorComponent : public LLMResponseTranslator::Component {
 public:
  explicit LLMResponseTranslatorComponent(
      an<LLMResponseEngineComponent> engine_factory);
  virtual ~LLMResponseTranslatorComponent();

  LLMResponseTranslator* Create(const Ticket& ticket) override;

 protected:
  an<LLMResponseEngineComponent> engine_factory_;
};

}  // namespace rime

#endif  // RIME_LLMRESPONSE_TRANSLATOR_H_
