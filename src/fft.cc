#include <assert.h>
#include <fftw3.h>
#include <string.h>
#include <cmath>
#include <iostream>
#include <fstream>
#include <limits>

#include "fft.h"
#include "util-inl.h"

using namespace std;

double Magnitude2(const fftw_complex& value) {
  return value[0] * value[0] + value[1] * value[1];
}

double Magnitude(const fftw_complex& value, size_t sample_count) {
  return std::sqrt(value[0] * value[0] + value[1] * value[1]) /
      static_cast<double>(sample_count);
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
bool FFT::FFTDecompose(size_t sample_count,
                       SampleType* samples,
                       FFTDecomposition* fft_decomposition) {
  assert(sample_count > 0);
  assert(samples != NULL);
  assert(fft_decomposition != NULL);

  // Perform the discrete Fourier transform. Since we use the fftw library, we
  // need to put the samples in a format it can understand. TODO: We can skip
  // the copy if SampleType is of type double.
  double* fftw_samples = (double*)fftw_malloc(sample_count * sizeof(double));
  size_t scale_count = sample_count / 2 + 1;
  fft_decomposition->fft_decomposition =
      (fftw_complex*)fftw_malloc(scale_count * sizeof(fftw_complex));
  fftw_plan fftw_dft_plan =
      fftw_plan_dft_r2c_1d(sample_count,
                           fftw_samples,
                           fft_decomposition->fft_decomposition,
                           0);
  fft_decomposition->sample_count = sample_count;
  CastCopy(fftw_samples, samples, sample_count);
  fftw_execute(fftw_dft_plan);
  fftw_destroy_plan(fftw_dft_plan);
  fftw_free(fftw_samples);
}

template <typename SampleType>
bool FFT::FFTRecompose(const FFTDecomposition& fft_decomposition,
                       SampleType* samples) {
  assert(fft_decomposition.sample_count > 0);
  assert(samples != NULL);

  size_t sample_count = fft_decomposition.sample_count;
  double* fftw_samples = (double*)fftw_malloc(sample_count * sizeof(double));
  fftw_plan fftw_dft_plan =
      fftw_plan_dft_c2r_1d(sample_count,
                           fft_decomposition.fft_decomposition,
                           fftw_samples,
                           FFTW_PRESERVE_INPUT);
  fftw_execute(fftw_dft_plan);
  double scale_factor = 1.0 / static_cast<double>(sample_count);
  for (size_t sample = 0; sample < sample_count; ++sample) {
    fftw_samples[sample] *= scale_factor;
  }
  CastCopy(samples, fftw_samples, sample_count);
  fftw_destroy_plan(fftw_dft_plan);
  fftw_free(fftw_samples);
}

bool FFT::PitchShift(size_t sample_rate,
                     double current_frequency,
                     double target_frequency,
                     FFTDecomposition* fft_decomposition) {
  assert(sample_rate > 0);
  assert(fft_decomposition != NULL);
  assert(fft_decomposition->sample_count > 0);

  scoped_array<fftw_complex> decomposition(
      new fftw_complex[fft_decomposition->sample_count]);
  memcpy(decomposition.get(), fft_decomposition->fft_decomposition,
         sizeof(fftw_complex) * fft_decomposition->sample_count);

  size_t scale_count =  fft_decomposition->sample_count / 2 + 1;
  double scale_shift = ((target_frequency - current_frequency) /
                        (sample_rate /  fft_decomposition->sample_count));
  for (size_t f = 0; f < scale_count - 1; ++f) {
    int source_bin = f - scale_shift;
    if (source_bin < 0 || source_bin >= fft_decomposition->sample_count) {
      fft_decomposition->fft_decomposition[f][0] =
          fft_decomposition->fft_decomposition[f][1] = 0;
    } else {
      fft_decomposition->fft_decomposition[f][0] = decomposition[source_bin][0];
      fft_decomposition->fft_decomposition[f][1] = decomposition[source_bin][1];
    }
  }
}

bool FFT::FindDominantFrequency(const FFTDecomposition& fft_decomposition,
                                size_t sample_rate,
				double* dominant_frequency) {
  assert(fft_decomposition.sample_count > 0);
  assert(sample_rate > 0);
  assert(dominant_frequency != NULL);

  // Given the decomposed signal into a discrete set of frequencies, we need to
  // localize response maxima precisely in frequency-space. This is done at an
  // accuracy greater than the steps between our basis frequencies by fitting a
  // parabola to filter responses.
  size_t sample_count = fft_decomposition.sample_count;
  size_t scale_count = sample_count / 2 + 1;
  scoped_array<double> fft_magnitudes(new double[scale_count]);
  double max_magnitude = 0;
  size_t max_scale = 0;
  for (size_t scale = 1; scale < scale_count - 1; ++scale) {
    double magnitude = Magnitude2(fft_decomposition.fft_decomposition[scale]);
    if (magnitude <= numeric_limits<double>::epsilon()) {
      continue;
    }
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
  *dominant_frequency = refined_scale * sample_rate / sample_count;
  return true;
}

// Explicit template instantiations for known valid types.
template bool FFT::FFTDecompose<double>(size_t, double*, FFTDecomposition*);
template bool FFT::FFTDecompose<float>(size_t, float*, FFTDecomposition*);

template bool FFT::FFTRecompose<double>(const FFTDecomposition&, double*);
template bool FFT::FFTRecompose<float>(const FFTDecomposition&, float*);
