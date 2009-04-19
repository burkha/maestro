#include <jack/jack.h>
#include <stdlib.h>
#include <stdio.h>

jack_port_t *input_port;
jack_port_t *output_port;

// The process callback for this JACK application. It is called by JACK at the
// appropriate times.
int process(jack_nframes_t nframes, void *arg) {
  jack_default_audio_sample_t* out =
      (jack_default_audio_sample_t*)jack_port_get_buffer(output_port, nframes);
  jack_default_audio_sample_t* in =
      (jack_default_audio_sample_t*)jack_port_get_buffer(input_port, nframes);

  return 0;
}

// This is the shutdown callback for this JACK application. It is called by JACK
// if the server ever shuts down or decides to disconnect the client.
void jack_shutdown(void *arg) {
  exit(1);
}

int main(int argc, char** argv) {
  jack_client_t *client;
  const char **ports;

  if (argc != 2) {
    fprintf(stderr, "Usage: jack_simple_client <name>\n");
    return 1;
  }

  if ((client = jack_client_new(argv[1])) == 0) {
    fprintf(stderr, "Jack server not running.\n");
    return 1;
  }

  jack_set_process_callback(client, process, NULL);
  jack_on_shutdown(client, jack_shutdown, NULL);

  return 0;
}
