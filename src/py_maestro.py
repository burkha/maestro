# Copyright 2008 and onwards Matthew Burkhart.
#
# py_maestro.py: Definition of the Python abstraction layer for the Maestro
# musical composition language. The purpose of this interface is to make musical
# composition as easy and as painless as possible.

from copy import deepcopy


# Note defines a sound of constant length.
class Note:
    def __init__(self, frequency = 0, volume = 1, length = 1):
        self.frequency = frequency
        self.volume = volume
        self.length = length

    def __pow__(self, other):
        return Note(self.frequency * other, self.volume, self.length)

    def Render(self):
        return '%.2f@%.5fx%f' % (self.frequency, self.volume, self.length)


# Segment class defines a list of notes and the times at which they should be
# played.
class Segment:
    def __init__(self, rifts = [[]]):
        self.rifts = deepcopy(rifts)

    # Duration of this segment (length of the longest rift).
    def Length(self):
        def LengthOfRift(list):
            total_length = 0
            for note in list:
                total_length += note.length
            return total_length
        longest = 0
        for rift in self.rifts:
            longest = max(longest, LengthOfRift(rift))
        return longest

    def map(self, function):
        return Segment(map(lambda x : map(function, x), self.rifts))

    # Append another segment to the end of ours. We do this by adding a 'rest'
    # to each of the segments equal to our length.
    def __iadd__(self, other):
        rest = [Note(0, 0, self.Length()),]
        for rift in other.rifts:
            self.rifts.append(rest + rift)
        return self

    def __iand__(self, other):
        for rift in other.rifts:
            self.rifts.append(rift)
        return self

    def __imul__(self, count):
        assert count > 0
        self_copy = Segment(self.rifts)
        for n in xrange(count - 1):
            self += self_copy
        return self

    def __add__(self, other):
        result = Segment(self.rifts)
        result += other
        return result

    def __and__(self, other):
        result = Segment(self.rifts)
        result &= other
        return result

    def __mul__(self, count):
        result = Segment(self.rifts)
        result *= count
        return result

    def Render(self):
        segment_string = ""
        for rift in self.rifts:
            if len(rift) == 0:
                continue
            rift_string = '('
            for note in rift:
                rift_string += note.Render() + ' '
            rift_string = rift_string[0:-1]
            rift_string += ') & '
            segment_string += rift_string
        segment_string = segment_string[0:-3]
        return segment_string


# Helper methods for bundling redundant code. This fundamentally adds no new
# functionality.
isiterable = lambda obj: isinstance(obj, basestring) \
    or getattr(obj, '__iter__', False)


def MajorChord(base_notes):
    return base_notes.map(lambda x : x ** (4.0 / 4.0)) & \
           base_notes.map(lambda x : x ** (5.0 / 4.0)) & \
           base_notes.map(lambda x : x ** (6.0 / 4.0))


def F(letters, volume=1, length=1):
    def LetterToNote(letter):
        return Note(440.0 * (2.0 ** (1.0 / 12.0)) ** (ord(letter) - ord('a')),
                    volume, length)
    return Segment([list(map(LetterToNote, letters)),])
