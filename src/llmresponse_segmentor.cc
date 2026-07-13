#include "llmresponse_segmentor.h"

#include <cstring>
#include <rime/segmentation.h>

#include "llmresponse_constants.h"

namespace rime {

LLMResponseSegmentor::LLMResponseSegmentor(const Ticket& ticket)
    : Segmentor(ticket) {}

bool LLMResponseSegmentor::Proceed(Segmentation* segmentation) {
  if (!segmentation->empty() && segmentation->back().HasTag("llmresponse")) {
    return true;  // Already segmented.
  }

  // PushInput() writes the whole sentinel at once, so it's already fully
  // present in input() from start onward.
  size_t start = segmentation->GetCurrentStartPosition();
  const string& input = segmentation->input();
  const size_t sentinel_len = std::strlen(llmresponse::kTriggerSentinel);
  if (start + sentinel_len > input.length() ||
      input.compare(start, sentinel_len, llmresponse::kTriggerSentinel) != 0) {
    return true;  // Not our sentinel; let other segmentors handle it.
  }

  // Claim everything from here to the end of input as one segment.
  Segment segment(start, input.length());
  segment.tags.insert("llmresponse");
  segmentation->AddSegment(segment);
  return false;  // exclusive: stop the segmentor chain for this segment.
}

}  // namespace rime
