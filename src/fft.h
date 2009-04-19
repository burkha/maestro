#ifndef FFT_H_
#define FFT_H_

// Collection of high level routines for signal analysis.
class FFT {
public:
  // Find the single most dominant frequency in a signal given a discrete set of
  // samples. Return value indicates an error.
  template <typename SampleType>
  static bool FindDominantFrequency(size_t sample_frequency,
				    size_t sample_count,
				    SampleType* samples,
				    double* dominant_frequency);
};

#endif  // FFT_H_
