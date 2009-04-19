#include <cmath>
#include <iostream>
#include <fstream>
#include <limits>

#include "midi.h"
#include "util-inl.h"

using namespace std;

// Read from a file a value encoded with Little Endian byte ordering and then
// return the value in Big Endian.
template <typename Type>
Type ReadLE(ifstream* file) {
  Type result;
  for (int b = sizeof(Type) - 1; b >= 0; --b) {
    file->read(&((char*)&result)[b], sizeof(char));
  }
  return result;
}

string ReadString(ifstream* file, size_t length) {
  scoped_array<char> buffer(new char[length+1]);
  file->read(buffer.get(), length);
  buffer[length] = 0;
  return string(buffer.get());
}

unsigned ReadVariableLengthValue(ifstream* file) {
  unsigned value = 0;
  unsigned char segment = 0;
  do {
    file->read((char*)&segment, sizeof(segment));
    value = (value << 7) | (segment & 0x7F);
  } while(segment & 0x80);
  return value;
}

double NoteToFrequency(int midi_note) {
  const double kBase = 440.0;
  const int kOffset = -57;  // May be -69 depending on "tuning".
  return kBase * std::pow(2.0, static_cast<double>(midi_note + kOffset) / 12.0);
}

// The details of the MIDI format are implemented using the description of the
// format found on:
// http://www.sonicspot.com/guide/midifiles.html and
// http://java.sun.com/docs/books/tutorial/sound/MIDI-seq-intro.html
bool MIDI::ReadEventMap(const string& midi_path,
			EventMap* event_map) {
  assert(!midi_path.empty());
  assert(event_map != NULL);
  assert(event_map->empty());

  typedef unsigned char byte;
  ASSERT_EQ(sizeof(byte), 1);
  typedef unsigned short uint16;
  ASSERT_EQ(sizeof(uint16), 2);
  typedef unsigned int uint32;
  ASSERT_EQ(sizeof(uint32), 4);

  enum EventType {
    SYSEX = 0xF0,
    META = 0xFF,
  };
  enum MetaEventType {
    TEXT = 0x01,
    TRACK_NAME = 0x03,
    LYRIC = 0x05,
    END_OF_TRACK = 0x2F,
    TEMPO = 0x51,
    SMPTE_OFFSET = 0x54,
  };
  enum ControllerEventType {
    NOTE_OFF = 0x8,
    NOTE_ON = 0x9,
  };

  ifstream midi_file(midi_path.c_str(), ios::binary);
  if (!midi_file.is_open()) {
    cout << "Error, could not open file: " << midi_path << endl;
    return false;
  }

  uint32 MThd = ReadLE<uint32>(&midi_file);
  uint32 header_size = ReadLE<uint32>(&midi_file);
  uint16 format_type = ReadLE<uint16>(&midi_file);
  uint16 track_count = ReadLE<uint16>(&midi_file);
  uint16 time_division = ReadLE<uint16>(&midi_file);
  ASSERT_EQ(MThd, 0x4D546864);
  ASSERT_EQ(header_size, 0x06);
  assert(format_type <= 2);

  // Due to the fact that tempo change events may occur in any track and apply
  // to *all* tracks, we must make two passes over the MIDI event data in order
  // to make sense of the MIDI delta-time values.
  Track tempo_events;

  for (size_t track = 0; track < track_count; ++track) {
    string track_name;
    Track track_events;
    Track track_tempo_events;

    assert(!midi_file.eof());
    uint32 MTrk = ReadLE<uint32>(&midi_file);
    uint32 track_size = ReadLE<uint32>(&midi_file);
    ASSERT_EQ(MTrk, 0x4D54726B);

    size_t track_end = static_cast<size_t>(midi_file.tellg()) + track_size;
    //int last_track_tempo_size = -1;
    //int last_track_event_size = -1;
    while (static_cast<size_t>(midi_file.tellg()) < track_end) {
      //assert(track_tempo_events.size() > last_track_tempo_size);
      //last_tack_tempo_size = track_tempo_events.size();
      //assert(track_tempo_events.size() > last_track_tempo_size);
      //last_tack_tempo_size = track_tempo_events.size();

      // We must keep track if we have added events to our tracks so that we can
      // make sure to add rests if we don't since all the timings are deltas at
      // this point.
      bool tempo_event_added = false;
      bool track_event_added = false;

      assert(!midi_file.eof());
      uint32 delta_time = ReadVariableLengthValue(&midi_file);
      byte event_type = ReadLE<byte>(&midi_file);

      // SYSEX event.
      if (event_type == SYSEX) {
        uint32 event_size = ReadVariableLengthValue(&midi_file);
        midi_file.seekg(event_size, ios_base::cur);
      }
      // META events.
      else if (event_type == META) {
        byte meta_event_type = ReadLE<byte>(&midi_file);
        uint32 meta_event_size = ReadVariableLengthValue(&midi_file);

        if (meta_event_type == TRACK_NAME) {
          assert(track_name.empty());
          track_name = ReadString(&midi_file, meta_event_size);
	  assert(!track_name.empty());
        } else if (meta_event_type == LYRIC || meta_event_type == TEXT) {
	  track_events.push_back(
	    Event(delta_time, ReadString(&midi_file, meta_event_size)));
	  track_event_added = true;
	} else if (meta_event_type == TEMPO) {
          ASSERT_EQ(meta_event_size, 3);
          uint32 tempo_raw = 0;
          for (int b = 0; b < 3; ++b)
            tempo_raw = (tempo_raw << 8) | ReadLE<byte>(&midi_file);
          double tempo = 60000000.0 / static_cast<double>(tempo_raw);
	  track_tempo_events.push_back(Event(delta_time, Event::TEMPO, tempo));
	  tempo_event_added = true;
        } else if (meta_event_type == SMPTE_OFFSET) {
          assert(false);  // Event type not supported.
        } else if (meta_event_type == END_OF_TRACK) {
	  ASSERT_EQ(meta_event_size, 0);
	  break;
	} else {
          midi_file.seekg(meta_event_size, ios_base::cur);
        }
      }
      // MIDI control events.
      else {
        byte control_event_type = (event_type & 0xF0) >> 4;
        // All events of this type will be following by either one or two data
        // bytes signified by the presence of the MSB.
        byte data[2];
        for (size_t n = 0; n < 5; ++n) {
          data[n] = ReadLE<byte>(&midi_file);
          if (!(data[n] & 0x80)) {
            midi_file.unget();
            break;
          }
        }
	if (control_event_type == NOTE_OFF) {
	  track_events.push_back(
	      Event(delta_time, Event::NOTE_OFF, NoteToFrequency(data[0])));
	  track_event_added = true;
	} else if (control_event_type == NOTE_ON) {
	  track_events.push_back(
	      Event(delta_time, Event::NOTE_ON,  NoteToFrequency(data[0])));
	  track_event_added = true;
	}
      }

      if (!tempo_event_added && delta_time > 0) {
	track_tempo_events.push_back(Event(delta_time));
      }
      if (!track_event_added && delta_time > 0) {
	track_events.push_back(Event(delta_time));
      }
    }
    // The following check indicates either a deviation from the MIDI file
    // standard (such as an improper size in the track header) or a bug in the
    // parsing code. Either way, just warn and try to recover.
    size_t over_step = static_cast<size_t>(midi_file.tellg()) - track_end;
    if (over_step != 0) {
      cout << "Warning: Track size disagreement = " << over_step << endl;
    }

    // Ensure that this track has been named and then add the track events to
    // the event map.
    if (track_name.empty()) {
      cout << "Warning: Encountered track with no name. Skipping." << endl;
    } else {
      (*event_map)[track_name] = track_events;
    }

    // Accumulate tempo events encountered within this track in with the master
    // tempot track.
    Track accumulated_tempo_events(tempo_events);
    tempo_events.clear();
    MergeTrackEvents(accumulated_tempo_events, track_tempo_events,
		     &tempo_events, true);
    ConsolidateRestEvents(&tempo_events);
  }
  assert(event_map->find(string()) == event_map->end());

  // As mentioned above, by merging the accumulated tempo events into each track
  // event list, we can then calculate real times.
  for (EventMap::iterator track = event_map->begin();
       track != event_map->end(); ++track) {
    Track track_events(track->second);
    track->second.clear();
    MergeTrackEvents(tempo_events, track_events, &track->second, true);
    TimeTrackEvents(&track->second, time_division);
  }

  return true;
}

void MIDI::MergeTrackEvents(const Track& events_a,
			    const Track& events_b,
			    Track* events,
			    bool delta_timing) {
  assert(events != NULL);
  ASSERT_EQ(events->size(), 0);
  assert(delta_timing == true);
  Track::const_iterator iterator_a = events_a.begin();
  Track::const_iterator iterator_b = events_b.begin();
  double offset_a = 0;
  double offset_b = 0;
  while (iterator_a != events_a.end() || iterator_b != events_b.end()) {
    double time_a = iterator_a != events_a.end() ?
        iterator_a->time - offset_a : numeric_limits<double>::max();
    double time_b = iterator_b != events_b.end() ?
        iterator_b->time - offset_b : numeric_limits<double>::max();

    if (time_a < time_b) {
      events->push_back(*(iterator_a++));
      if (delta_timing) {
	events->back().time -= offset_a;
	offset_b += events->back().time;
	offset_a = 0;
      }
    } else {
      events->push_back(*(iterator_b++));
      if (delta_timing) {
	events->back().time -= offset_b;
	offset_a += events->back().time;
	offset_b = 0;
      }
    }
  }
}

void MIDI::ConsolidateRestEvents(Track* track) {
  assert(track != NULL);

  Event* previous_rest = NULL;
  for (Track::iterator event = track->begin(); event != track->end();) {
    if (event->type == Event::REST) {
      if (previous_rest == NULL) {
	previous_rest = &*event;
      } else {
	previous_rest->time += event->time;
	event = track->erase(event);
	continue;
      }
    } else {
      previous_rest = NULL;
    }
    ++event;
  }
}

void MIDI::TimeTrackEvents(Track* track,
			   unsigned short time_division) {
  assert(track != NULL);
  // Time division as frames per second not yet supported.
  assert(!(time_division & 0x8000));
  time_division = time_division & 0x7FFF;

  double current_track_time = 0;
  double current_track_tempo = 120.0;  // Beats per minute.
  for (Track::iterator event = track->begin(); event != track->end();) {
    double tick_length = 1.0 /
      (static_cast<double>(time_division) * (current_track_tempo / 60.0));
    current_track_time += static_cast<double>(event->time) * tick_length;

    event->time = current_track_time;

    if (event->type == MIDI::Event::TEMPO) {
      current_track_tempo = event->real_value;
      event = track->erase(event);
    } else if (event->type == MIDI::Event::REST) {
      event = track->erase(event);
    } else {
      ++event;
    }
  }
}

std::string MIDI::GuessLyricTrack(const EventMap& event_map) {
  assert(event_map.size() > 0);

  string lyric_track;
  size_t lyric_track_count = 0;
  for (EventMap::const_iterator track = event_map.begin();
       track != event_map.end(); ++track) {
    size_t lyric_count = 0;
    for (Track::const_iterator event = track->second.begin();
	 event != track->second.end(); ++event) {
      if (event->type == Event::LYRIC) {
	++lyric_count;
      }
    }
    if (lyric_count > lyric_track_count) {
      lyric_track = track->first;
      lyric_track_count = lyric_count;
    }
  }
  assert(!lyric_track.empty());
  return lyric_track;
}

const MIDI::Event* FindNearestEvent(double time, MIDI::Event::Type type,
				    const MIDI::Track& track) {
  const MIDI::Event* nearest_event = NULL;
  double nearest_event_time = std::numeric_limits<double>::max();

  for (MIDI::Track::const_iterator event = track.begin();
       event != track.end(); ++event) {
    if (event->type == type) {
      double event_time = std::abs(time - event->time);
      if (event_time < nearest_event_time) {
	nearest_event = &(*event);
	nearest_event_time = event_time;
      }
    }
  }
  return nearest_event;
}

std::string MIDI::GuessMelodyTrack(const EventMap& event_map,
				   const Track& lyric_track) {
  assert(event_map.size() > 0);
  assert(lyric_track.size() > 0);

  string melody_track;
  double melody_track_divergence = std::numeric_limits<double>::max();
  for (EventMap::const_iterator track = event_map.begin();
       track != event_map.end(); ++track) {
    double track_divergence = 0;
    for (Track::const_iterator event = lyric_track.begin();
	 event != lyric_track.end(); ++event) {
      if (event->type != Event::LYRIC) {
	continue;
      }
      const MIDI::Event* nearest_event =
	FindNearestEvent(event->time, Event::NOTE_ON, track->second);
      track_divergence += nearest_event == NULL ? 10 :
	std::log(1.0 + std::abs(event->time - nearest_event->time));
    }
    cout << "TD: " << track->first << " - " << track_divergence << endl;

    if (track_divergence < melody_track_divergence) {
      melody_track = track->first;
      melody_track_divergence = track_divergence;
    }
  }

  assert(!melody_track.empty());
  return melody_track;
}
