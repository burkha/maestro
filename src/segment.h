#ifndef SEGMENT_H_
#define SEGMENT_H_

#include <string>
#include <vector>

#include "note.h"

// Segment represents a sequence of timed, potentially overlapping notes and
// their associated instruments. The Segment interface is not thread-safe.
template <typename SampleType>
class Segment {
 public:
  Segment();
  Segment(const Segment<SampleType>& segment);
  Segment(const Note& note);

  void operator=(const Segment<SampleType>& segment);

  float length() const;
  const std::vector<Note>& notes() const;

  // Append another segment to the end of this segment. The segment instance
  // length will be the sum of the length of each.
  void Concatenate(const Segment<SampleType>& segment);

  // Union with another segment. The segment instance length will be the max of
  // the length of each.
  void Union(const Segment<SampleType>& segment);

 private:
  float length_;

  std::vector<Note> notes_;
};

// Concatenate two segments. The resulting segment length is the sum of the two
// input lengths.
template <typename SampleType>
Segment<SampleType> Concatenate(const Segment<SampleType>& segment_a,
				const Segment<SampleType>& segment_b);

// Union two segments. The resulting segment length is the max of the two input
// lengths.
template <typename SampleType>
Segment<SampleType> Union(const Segment<SampleType>& segment_a,
                          const Segment<SampleType>& segment_b);

#endif  // SEGMENT_H_
