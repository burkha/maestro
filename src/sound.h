#ifndef SOUND_H_
#define SOUND_H_

#include <string>

// A collection of routines for sound acquisition.
class Sound {
public:
  // Allocates space for and reads samples from a file.
  template <typename SampleType>
  static void ReadFromFile(const std::string& path,
                           size_t* sample_frequency,
                           size_t* sample_count,
                           SampleType** samples);

  // Read from the microphone as many samples as are required to fill 'length'
  // seconds. The sample frequency is determined by the hardware. Note that this
  // is currently inefficient since the device is opened and closed on each
  // call.
  template <typename SampleType>
  static void ReadFromMicrophone(double length,
                                 size_t* sample_frequency,
                                 size_t* sample_count,
                                 SampleType** samples);
};

#endif  // SOUND_H_
