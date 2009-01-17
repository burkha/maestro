import sys
sys.path.append('../src')
from py_maestro import *

chorus = MajorChord(F('ac', 0.2, 2)) * 2
melody = F('de', 1, 1) + F('c', 1, 0.5) + F('d') + F('e', 1, 2)

song = chorus & melody
print song.Render()
