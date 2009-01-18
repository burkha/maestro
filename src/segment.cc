#include <assert.h>
#include <vector>

#include "segment.h"

using namespace std;

template <typename SampleType>
Segment<SampleType>::Segment()
    : length_(0.0f) {
}

template <typename SampleType>
Segment<SampleType>::Segment(const Segment& segment)
    : length_(segment.length_), notes_(segment.notes_) {
}

template <typename SampleType>
Segment<SampleType>::Segment(const Note& note) {
  length_ = note.length();
  notes_.push_back(note);
}

template <typename SampleType>
void Segment<SampleType>::operator=(const Segment& segment) {
  length_ = segment.length_;
  notes_ = segment.notes_;
}

template <typename SampleType>
float Segment<SampleType>::length() const {
  return length_;
}

template <typename SampleType>
const vector<Note>& Segment<SampleType>::notes() const {
  return notes_;
}

template <typename SampleType>
void Segment<SampleType>::Concatenate(const Segment& segment) {
  notes_.reserve(notes_.size() + segment.notes_.size());
  for (vector<Note>::const_iterator note = segment.notes_.begin();
       note != segment.notes_.end(); ++note) {
    notes_.push_back(*note);
    notes_.back().set_time(length() + note->time());
  }
  length_ += segment.length_;
}

template <typename SampleType>
void Segment<SampleType>::Union(const Segment& segment) {
  notes_.insert(notes_.end(), segment.notes_.begin(), segment.notes_.end());
  length_ = std::max(length_, segment.length_);
}

template <typename SampleType>
Segment<SampleType> Concatenate(const Segment<SampleType>& segment_a,
				const Segment<SampleType>& segment_b) {
  Segment<SampleType> result(segment_a);
  result.Concatenate(segment_b);
  return result;
}

template <typename SampleType>
Segment<SampleType> Union(const Segment<SampleType>& segment_a,
                          const Segment<SampleType>& segment_b) {
  Segment<SampleType> result(segment_a);
  result.Union(segment_b);
  return result;
}


// Explicit template instantiations of supported types.
template class Segment<int>;
template Segment<int> Concatenate(const Segment<int>&, const Segment<int>&);
template Segment<int> Union(const Segment<int>&, const Segment<int>&);
