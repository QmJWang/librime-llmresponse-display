#ifndef RIME_LLMRESPONSE_CONSTANTS_H_
#define RIME_LLMRESPONSE_CONSTANTS_H_

namespace rime {
namespace llmresponse {

// Sentinel pushed into context input() to mark an llmresponse trigger.
// Shared by Segmentor and Processor. Not schema-configurable; the
// user-facing trigger key lives in LLMResponseProcessor's config instead.
inline constexpr char kTriggerSentinel[] = "\x01llm";

}  // namespace llmresponse
}  // namespace rime

#endif  // RIME_LLMRESPONSE_CONSTANTS_H_
