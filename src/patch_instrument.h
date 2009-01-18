#ifndef PATCH_INSTRUMENT_H_
#define PATCH_INSTRUMENT_H_

#include "instrument.h"

template <typename SampleType>
class PatchInstrument {
 public:
  virtual ~PatchInstrument() {}

  virtual void Generate(float frequency,  // Hz.
                        float amplitude,  // [0-1].
                        float length,     // Seconds.
                        int sample_rate,  // Samples / second.
                        int sample_offset,
                        int sample_count,
                        SampleType* samples);
};

#endif  // PATCH_INSTRUMENT_H_
