#ifndef FFT_H_
#define FFT_H_

#include <fftw3.h>

// Collection of high level routines for signal analysis.
class FFT {
 public:
  struct FFTDecomposition {
    FFTDecomposition() : fft_decomposition(NULL) {}
    ~FFTDecomposition() {
      if (fft_decomposition != NULL) {
        fftw_free(fft_decomposition);
      }
    }

    fftw_complex* fft_decomposition;
    size_t sample_count;
  };

  // Decompose a 1D signal into the Fourier domain.
  template <typename SampleType>
  static bool FFTDecompose(size_t sample_count,
                           SampleType* samples,
                           FFTDecomposition* fft_decomposition);

  // Recompose a 1D signal in Fourier space previously decomposed by
  // FFTDecompose. This function is inverse of FFTDecompose.
  template <typename SampleType>
  static bool FFTRecompose(const FFTDecomposition& fft_decomposition,
                           SampleType* samples);

  // Modify the pitch of a decomposed signal.
  static bool PitchShift(size_t sample_frequency,
                         double current_frequency,
                         double target_frequency,
                         FFTDecomposition* fft_decomposition);

  // Find the single most dominant frequency in a signal given a discrete set of
  // samples. Return value indicates an error.
  static bool FindDominantFrequency(const FFTDecomposition& fft_decomposition,
                                    size_t sample_frequency,
				    double* dominant_frequency);
};

#endif  // FFT_H_
