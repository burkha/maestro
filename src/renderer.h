#ifndef RENDERER_H_
#define RENDERER_H_

#include <string>

#include "segment.h"

template <typename SampleType, typename AccumulatorType>
class Renderer {
 public:
  void WriteWAV(const Segment<SampleType>& segment,
                const std::string& target_path);

 private:
  SampleType SoftClip(AccumulatorType sample) const;
};

#endif  // RENDERER_H_
