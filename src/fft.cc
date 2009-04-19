#include <assert.h>
#include <fftw3.h>
#include <cmath>
#include <iostream>
#include <fstream>

#include "fft.h"
#include "util-inl.h"

using namespace std;

double Magnitude(const fftw_complex& value) {
  return value[0] * value[0] + value[1] * value[1];
}

// Fit a 2D parabola in the [-1, 1] region given samples (in a 3x1 matrix) at
// -1, 0, and 1. Output argument peak is set to the location of parabola
// extremum. Returns true if an extremum is located within 0.5 of the center
// sample.
template <typename Type>
bool PeakOfParabolicFit(const Type samples[3], Type *peak) {
  assert(samples != NULL);
  assert(peak != NULL);

  // Inverse parabolic interpolation. See Num. Rec. in C, ch. 10.2, p. 402. a =
  // -1.0, b = 0.0, c = 1.0;
  const Type &fa = samples[0], &fb = samples[1], &fc = samples[2];
  *peak = -0.5 * (fa - fc) / ((fb - fc) + (fb - fa));
  return !std::isinf(*peak) && *peak >= -0.5 && *peak < 0.5;
}

template <typename SampleType>
bool FFT::FindDominantFrequency(size_t sample_frequency,
				size_t sample_count,
				SampleType* samples,
				double* dominant_frequency) {
  assert(sample_frequency > 0);
  assert(sample_count > 0);
  assert(samples != NULL);
  assert(dominant_frequency != NULL);

  // Perform the discrete Fourier transform. Since we use the fftw library, we
  // need to put the samples in a format it can understand. Element 0 of the
  // result vector is the "DC" and the n/2-th is the "Nyquist" frequency. TODO:
  // We can skip the copy if SampleType is of type double.
  double* fftw_samples = (double*)fftw_malloc(sample_count * sizeof(double));
  size_t scale_count = sample_count / 2 + 1;
  fftw_complex* fftw_result =
    (fftw_complex*)fftw_malloc(scale_count * sizeof(fftw_complex));
  fftw_plan fftw_dft_plan =
      fftw_plan_dft_r2c_1d(sample_count, fftw_samples, fftw_result, 0);
  CastCopy(fftw_samples, samples, sample_count);
  fftw_execute(fftw_dft_plan);
  fftw_destroy_plan(fftw_dft_plan);

  // Now that we have (fftw has) decomposed the signal into a discrete set of
  // frequencies, we need to localize response maxima precisely in
  // frequency-space. This is done at an accuracy greater than the steps between
  // our basis frequencies by fitting a parabola to filter responses.
  scoped_array<double> fft_magnitudes(new double[scale_count]);
  double max_magnitude = 0;
  size_t max_scale = 0;

  for (size_t scale = 1; scale < scale_count -1; ++scale) {
    double magnitude = Magnitude(fftw_result[scale]);
    assert(magnitude > 0);
    if (magnitude > max_magnitude) {
      max_magnitude = magnitude;
      max_scale = scale;
    }
    fft_magnitudes[scale] = magnitude;
  }

  if (max_scale == 0 || max_scale >= scale_count) {
    return false;
  }

  double scale_offset;
  assert(PeakOfParabolicFit(fft_magnitudes.get() + max_scale - 1, &scale_offset));
  double refined_scale = max_scale + scale_offset;
  *dominant_frequency = refined_scale *
    1.0 / (static_cast<double>(sample_count) / static_cast<double>(sample_frequency));

  fftw_free(fftw_samples);
  fftw_free(fftw_result);
  return true;
}

// Explicit template instantiations for known valid types.
template bool FFT::FindDominantFrequency<double>(size_t, size_t, double*, double*);
