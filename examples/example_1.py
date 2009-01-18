import sys
sys.path.append('../src')
from py_maestro import *


chorus = 'a'
segment = MajorChord(F(chorus))

print segment.Render()
