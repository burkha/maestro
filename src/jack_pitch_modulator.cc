#include <jack/jack.h>
#include <jack/midiport.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <set>

#include "fft.h"
#include "midi.h"

jack_port_t* input_port_midi = NULL;
jack_port_t* input_port_audio = NULL;
jack_port_t* output_port_audio = NULL;

std::set<double> active_notes;

int process_midi(jack_nframes_t nframes, void* args) {
  void* midi_port_buffer = jack_port_get_buffer(input_port_midi, nframes);
  jack_nframes_t midi_event_count = jack_midi_get_event_count(midi_port_buffer);
  for (size_t midi_event = 0; midi_event < midi_event_count; ++midi_event) {
    jack_midi_event_t jack_midi_event;
    jack_midi_event_get(&jack_midi_event, midi_port_buffer, midi_event);

    MIDI::RawEvent raw_midi_event;
    raw_midi_event.time = jack_midi_event.time;
    raw_midi_event.size = jack_midi_event.size;
    memcpy(&raw_midi_event.data, jack_midi_event.buffer, raw_midi_event.size);

    MIDI::Event midi_event;
    MIDI::InterpretRawEvent(raw_midi_event, &midi_event);
    if (midi_event.type == MIDI::Event::RESET) {
      active_notes.clear();
    }
    if (midi_event.type == MIDI::Event::NOTE_ON) {
      active_notes.insert(midi_event.real_value);
      printf("New frequency target: %f\n", midi_event.real_value);
    }
    if (midi_event.type == MIDI::Event::NOTE_OFF) {
      active_notes.erase(midi_event.real_value);
    }
  }
  return 0;
}

int process_audio(jack_nframes_t nframes, void* args) {
  jack_default_audio_sample_t* input_audio =
      (jack_default_audio_sample_t*)jack_port_get_buffer(input_port_audio, nframes);
  jack_default_audio_sample_t* output_audio =
      (jack_default_audio_sample_t*)jack_port_get_buffer(output_port_audio, nframes);
  FFT::FFTDecomposition fft_decomposition;
  FFT::FFTDecompose(nframes, input_audio, &fft_decomposition);

  double target_frequency = 440.0;
  if (active_notes.size() > 0) {
    target_frequency = *active_notes.begin();
  }
  double dominant_frequency = 0.0;
  if (FFT::FindDominantFrequency(fft_decomposition, 48000, &dominant_frequency)) {
    FFT::PitchShift(48000, dominant_frequency, target_frequency, &fft_decomposition);
    printf("%f -> %f\n", dominant_frequency, target_frequency);
  }

  FFT::FFTRecompose(fft_decomposition, output_audio);
  return 0;
}

// This is the shutdown callback for this JACK application. It is called by JACK
// if the server ever shuts down or decides to disconnect the client.
void jack_shutdown(void *arg) {
  exit(1);
}

int main(int argc, char** argv) {
  if (argc != 1) {
    fprintf(stderr, "Usage: jack_pitch_modulator\n");
    return 1;
  }

  jack_client_t *client = jack_client_new("jack_pitch_modulator");
  jack_client_t *controller_client = jack_client_new("jack_pitch_modulator_controller");
  if (client == NULL || controller_client == NULL) {
    fprintf(stderr, "Could not create the Jack clients. Ensure that the Jack "
            "server is running.\n");
    return 1;
  }

  jack_set_process_callback(client, process_audio, NULL);
  jack_set_process_callback(controller_client, process_midi, NULL);
  jack_on_shutdown(client, jack_shutdown, NULL);
  jack_on_shutdown(controller_client, jack_shutdown, NULL);

  printf("Engine sample rate: %d\n", int(jack_get_sample_rate(client)));

  input_port_midi = jack_port_register(
      controller_client, "input_midi", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
  input_port_audio = jack_port_register(
      client, "input_audio", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
  output_port_audio = jack_port_register(
      client, "output_audio", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

  if (jack_activate(client)) {
    fprintf(stderr, "Cannot activate client");
    return 1;
  }
  if (jack_activate(controller_client)) {
    fprintf(stderr, "Cannot activate controller client");
    return 1;
  }

  while (true) {
    sleep(1);
  }
  return 0;
}
