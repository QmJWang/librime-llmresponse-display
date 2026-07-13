#include <rime/component.h>
#include <rime/registry.h>
#include <rime_api.h>

#include "llmresponse_engine.h"
#include "llmresponse_processor.h"
#include "llmresponse_segmentor.h"
#include "llmresponse_translator.h"

using namespace rime;

static void rime_llmresponse_initialize() {
  Registry& r = Registry::instance();
  an<LLMResponseEngineComponent> engine_factory = New<LLMResponseEngineComponent>();
  r.Register("llmresponse_segmentor", new Component<LLMResponseSegmentor>());
  r.Register("llmresponse_processor",
             new LLMResponseProcessorComponent(engine_factory));
  r.Register("llmresponse_translator",
             new LLMResponseTranslatorComponent(engine_factory));
  // processor and translator share engine_factory; GetInstance() caches by
  // schema_id so both resolve to the same LLMResponseEngine instance.
}

static void rime_llmresponse_finalize() {}

RIME_REGISTER_MODULE(llmresponse)
