// Copyright 2008 and onwards Matthew Burkhart.

#ifndef NOTE_H_
#define NOTE_H_

// The Note class defines the atomic abstract unit of sound.
class Note {
 public:
  Note();
  Note(float amplitude, float frequency, float length)
      : amplitude_(amplitude), frequency_(frequency), length_(length), time_(0.0f) {
  }

  float amplitude() const { return amplitude_; }
  float frequency() const { return frequency_; }
  float length() const { return length_; }
  float time() const { return time_; }

  bool operator<(const Note& note) const { return time_ < note.time_; }

  void set_amplitude(float amplitude) { amplitude_ = amplitude; }
  void set_frequency(float frequency) { frequency_ = frequency; }
  void set_length(float length) { length_ = length; }
  void set_time(float time) { time_ = time; }

 private:
  float amplitude_;  // [0-1].
  float frequency_;  // Hz.
  float length_;     // Seconds.
  float time_;       // Seconds from song start.
};

#endif  // NOTE_H__
