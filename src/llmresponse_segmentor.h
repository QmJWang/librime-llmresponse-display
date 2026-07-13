#ifndef RIME_LLMRESPONSE_SEGMENTOR_H_
#define RIME_LLMRESPONSE_SEGMENTOR_H_

#include <rime/segmentor.h>

namespace rime {

// Detects the sentinel string pushed by Processor into input() and carves
// out an "llmresponse"-tagged segment; LLMResponseTranslator only fires on
// that tag.
//
// Must be listed first under the schema's engine/segmentors (before
// abc_segmentor): otherwise abc_segmentor would consume the sentinel's
// trailing pinyin-like characters before this segmentor sees the full
// string in one segment.
class LLMResponseSegmentor : public Segmentor {
 public:
  explicit LLMResponseSegmentor(const Ticket& ticket);

  bool Proceed(Segmentation* segmentation) override;
};

}  // namespace rime

#endif  // RIME_LLMRESPONSE_SEGMENTOR_H_
