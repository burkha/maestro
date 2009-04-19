#ifndef MIDI_H_
#define MIDI_H_

#include <map>
#include <string>
#include <vector>

class MIDI {
 public:
  struct Event {
    double time;

    enum Type {
      REST,
      NOTE_ON,
      NOTE_OFF,
      TEMPO,
      LYRIC,
    } type;

    double real_value;  // Valid for NOTE_ON, NODE_OFF, TEMPO types.
    std::string string_value;  // Valid for LYRIC type.

    explicit Event(double t) : time(t), type(REST) {}
    Event(double t, Type e, double v) : time(t), type(e), real_value(v) {}
    Event(double t, const std::string& v) : time(t), type(LYRIC), string_value(v) {}
  };

  typedef std::vector<Event> Track;
  typedef std::map<std::string, Track> EventMap;

  // ReadEventMap(...) reads all NOTE_ON, NOTE_OFF, and LYRIC events from all
  // tracks from the specified midi file path. Event times are to be interpreted
  // as 'real time' (in seconds) from the beginning of the track.
  static bool ReadEventMap(const std::string& midi_path,
			   EventMap* event_map);

  // GuessLyricTrack returns the name of the single track within the user
  // specified event map which has the most lyric events.
  static std::string GuessLyricTrack(const EventMap& event_map);

  // GuessMelodyTrack attempts to find and return the name of the single track
  // which has note events most similar / corresponding to the lyric events in
  // the specified "lyric_track".
  static std::string GuessMelodyTrack(const EventMap& event_map,
				      const Track& lyric_track);

  static void MergeTrackEvents(const Track& events_a,
			       const Track& events_b,
			       Track* events,
			       bool delta_timing = false);

  static void ConsolidateRestEvents(Track* track);

  static void TimeTrackEvents(Track* events,
			      unsigned short time_division);
};

#endif  // MIDI_H__
