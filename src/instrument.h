#ifndef INSTRUMENT_H_
#define INSTRUMENT_H_

// Instrument defines the interface to a waveform / patch generator.
template <typename SampleType>
class Instrument {
 public:
  virtual ~Instrument() {}

  virtual void Generate(float frequency,  // Hz.
                        float amplitude,  // [0-1].
                        float length,     // Seconds.
                        int sample_rate,  // Samples / second.
                        int sample_offset,
                        int sample_count,
                        SampleType* samples) = 0;
};

// Tone generator "reference" instrument implementation.
template <typename SampleType>
class ToneGeneratorInstrument : public Instrument<SampleType> {
 public:
  virtual ~ToneGeneratorInstrument() {}

  virtual void Generate(float frequency,  // Hz.
                        float amplitude,  // [0-1].
                        float length,     // Seconds.
                        int sample_rate,  // Samples / second.
                        int sample_offset,
                        int sample_count,
                        SampleType* samples);
};

#endif  // INSTRUMENT_H_
