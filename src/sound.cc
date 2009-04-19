#include <assert.h>
#include <sndfile.h>
#include <alsa/asoundlib.h>
#include <iostream>

#include "sound.h"
#include "util-inl.h"

using namespace std;

template <typename SampleType>
void ReadSamplesFromFile(SNDFILE* sound_file,
                         size_t sample_count,
                         SampleType* samples) {
  assert(false);  // Reading for samples of this type not defined.
}

template < >
void ReadSamplesFromFile(SNDFILE* sound_file,
                         size_t sample_count,
                         double* samples) {
  sf_read_double(sound_file, samples, sample_count);
}

template <typename SampleType>
void Sound::ReadFromFile(const string& path,
                         size_t* sample_frequency,
                         size_t* sample_count,
                         SampleType** samples) {
  assert(!path.empty());
  assert(sample_count != NULL);
  assert(samples != NULL);

  SF_INFO sound_file_info;
  SNDFILE* sound_file = sf_open(path.c_str(), SFM_READ, &sound_file_info);

  *sample_frequency = sound_file_info.samplerate;
  *sample_count = sound_file_info.frames;
  *samples = new SampleType[*sample_count];
  ReadSamplesFromFile<SampleType>(sound_file, *sample_count, *samples);

  sf_close(sound_file);
}

template <typename SampleType>
void Sound::ReadFromMicrophone(double length,
                               size_t* sample_frequency,
                               size_t* sample_count,
                               SampleType** samples) {
  int err;
  snd_pcm_t *capture_handle;
  snd_pcm_hw_params_t *hw_params;

  string device = "default";
  unsigned int desired_sample_frequency = 44100;

  if ((err = snd_pcm_open(&capture_handle, device.c_str(), SND_PCM_STREAM_CAPTURE, 0)) < 0) {
    fprintf(stderr, "cannot open audio device %s (%s)\n", device.c_str(), snd_strerror(err));
    exit(1);
  }

  if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
    fprintf(stderr, "cannot allocate hardware parameter structure (%s)\n", snd_strerror(err));
    exit(1);
  }

  if ((err = snd_pcm_hw_params_any(capture_handle, hw_params)) < 0) {
    fprintf(stderr, "cannot initialize hardware parameter structure (%s)\n", snd_strerror(err));
    exit(1);
  }

  if ((err = snd_pcm_hw_params_set_access(capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
    fprintf(stderr, "cannot set access type (%s)\n", snd_strerror(err));
    exit(1);
  }

  if ((err = snd_pcm_hw_params_set_format(capture_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
    fprintf (stderr, "cannot set sample format (%s)\n", snd_strerror(err));
    exit(1);
  }

  if ((err = snd_pcm_hw_params_set_rate_near(capture_handle, hw_params, &desired_sample_frequency, 0)) < 0) {
    fprintf(stderr, "cannot set sample rate (%s)\n", snd_strerror(err));
    exit(1);
  }
  *sample_frequency = desired_sample_frequency;
  *sample_count = static_cast<size_t>(length * static_cast<double>(*sample_frequency));
  *samples = new SampleType[*sample_count];

  if ((err = snd_pcm_hw_params_set_channels(capture_handle, hw_params, 1)) < 0) {
    fprintf(stderr, "cannot set channel count (%s)\n", snd_strerror(err));
    exit(1);
  }

  if ((err = snd_pcm_hw_params(capture_handle, hw_params)) < 0) {
    fprintf(stderr, "cannot set parameters (%s)\n", snd_strerror(err));
    exit(1);
  }

  snd_pcm_hw_params_free(hw_params);

  if ((err = snd_pcm_prepare(capture_handle)) < 0) {
    fprintf(stderr, "cannot prepare audio interface for use (%s)\n", snd_strerror(err));
    exit(1);
  }

  scoped_array<short> buffer(new short[*sample_count]);
  if ((err = snd_pcm_readi(capture_handle, buffer.get(), *sample_count)) != static_cast<int>(*sample_count)) {
    if (err <= 0) {
      fprintf(stderr, "read from audio interface failed (%s)\n", snd_strerror(err));
      exit(1);
    } else {
      *sample_count = err;
    }
  }
  CastCopy(*samples, buffer.get(), *sample_count);

  snd_pcm_close(capture_handle);
}

// Explicit template instantiations for known valid types.
template void Sound::ReadFromFile<double>(const string&, size_t*, size_t*, double**);
template void Sound::ReadFromMicrophone<double>(double, size_t*, size_t*, double**);
