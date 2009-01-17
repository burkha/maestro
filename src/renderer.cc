#include "renderer.h"

#include <assert.h>
#include <sndfile.h>
#include <cmath>
#include <deque>
#include <limits>
#include <vector>

#include "instrument.h"

const float kChunkLength = 1.0f;  // Seconds.
const int kSampleRate = 22000;    // Samples / second.
const int kChunkSampleSize = static_cast<int>(kChunkLength * kSampleRate);

template <typename SampleType, typename AccumulatorType>
void Renderer<SampleType, AccumulatorType>::WriteWAV(
    const Segment<SampleType>& segment,
    const std::string& target_path) {
  assert(!target_path.empty());

  struct SF_INFO sound_format;
  sound_format.samplerate = kSampleRate;
  sound_format.channels = 1;
  sound_format.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
  assert(sf_format_check(&sound_format) == 1);
  SNDFILE* sound_file = sf_open(target_path.c_str(), SFM_WRITE, &sound_format);
  assert(sound_file);

  // In order to simplify the rendering process later, we want to sort the notes
  // here by their temporal position in the result.
  std::deque<Note> notes(segment.notes().begin(), segment.notes().end());
  std::sort(notes.begin(), notes.end());

  // The following algorithm is as follows: step through time in intervals of
  // kChunkLength. For each time interval chunk, combine instrument samples,
  // clip, and write out to the target WAV file.
  std::vector<AccumulatorType> accumulator_buffer(kChunkSampleSize);
  std::vector<SampleType> sample_buffer(kChunkSampleSize);
  for (float time = 0; time <= segment.length(); time += kChunkLength) {
    // Render notes within the chunk range to the accumulator buffer and delete
    // notes we've passed in time.
    std::fill(accumulator_buffer.begin(), accumulator_buffer.end(), 0);
    for (std::deque<Note>::iterator note = notes.begin();
         note != notes.end(); ++note) {
      while (note != notes.end() && note->time() + note->length() < time) {
        note = notes.erase(note);
      }
      if (note == notes.end() || note->time() >= time + kChunkLength) {
        break;
      }

      int sample_offset =
          std::max<int>(0, static_cast<int>((time - note->time()) * kSampleRate));
      float sample_start =
          std::max(note->time(), time);
      float sample_end =
          std::min(note->time() + note->length(), time + kChunkLength);
      int sample_count =
          static_cast<int>((sample_end - sample_start) * kSampleRate);
      assert(sample_count >= 0);
      ToneGeneratorInstrument<SampleType> instrument;
      instrument.Generate(note->frequency(),
                          note->amplitude(),
                          note->length(),
                          kSampleRate,
                          sample_offset,
                          sample_count,
                          &sample_buffer.front());

      int accumulator_offset =
          std::max<int>(0, static_cast<int>((note->time() - time) * kSampleRate));
      for (int sample_index = 0; sample_index < sample_count; ++sample_index) {
        accumulator_buffer[accumulator_offset + sample_index] +=
            sample_buffer[sample_index];
      }
    }

    // Clip / re-sample the accumulator buffer into the sample buffer and then
    // write the result out to the WAV file.
    typename std::vector<AccumulatorType>::const_iterator accumulator =
        accumulator_buffer.begin();
    typename std::vector<SampleType>::iterator sample = sample_buffer.begin();
    for (; accumulator != accumulator_buffer.end(); ++accumulator, ++sample) {
      *sample = SoftClip(*accumulator);
    }
    assert(sf_write_int(sound_file, &sample_buffer.front(), kChunkSampleSize) ==
           kChunkSampleSize);
  }
  assert(!sf_close(sound_file));
}

template <typename SampleType, typename AccumulatorType>
SampleType Renderer<SampleType, AccumulatorType>::SoftClip(
    AccumulatorType sample) const {
  static const AccumulatorType kMax = std::numeric_limits<SampleType>::max();

  double d_sample =
      std::abs(static_cast<double>(sample) / static_cast<double>(kMax));
  double magnitude =
      -std::log(std::exp(-d_sample) + 1.0) +
      std::log(std::exp(0.0) + 1.0);
  magnitude *= static_cast<double>(kMax);

  SampleType result =
      static_cast<SampleType>(sample < 0 ? -magnitude : magnitude);
  return result;
}

// Explicit template instantiations of supported types.
template class Renderer<int, long long>;
