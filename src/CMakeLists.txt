CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(MAESTRO)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})
INCLUDE(CMakeMacros.txt)

YACC_FILE(maestro_yacc.y)
LEX_FILE(maestro_lex.l)

ADD_DEFINITIONS()


ADD_LIBRARY(sound_utils STATIC
fft.cc fft.h
instrument.cc instrument.h
midi.cc midi.h
patch_instrument.cc patch_instrument.h
renderer.cc renderer.h
segment.cc segment.h
sound.cc sound.h)


ADD_EXECUTABLE(maestro
main.cc
maestro_yacc.cc
maestro_lex.cc)

SET_TARGET_PROPERTIES(maestro PROPERTIES COMPILE_FLAGS "-Wall -O0 -g")
TARGET_LINK_LIBRARIES(maestro asound fftw3 fl rt sndfile sound_utils)


ADD_EXECUTABLE(jack_pitch_modulator
jack_pitch_modulator.cc)

SET_TARGET_PROPERTIES(jack_pitch_modulator PROPERTIES COMPILE_FLAGS "-Wall -O0 -g")
TARGET_LINK_LIBRARIES(jack_pitch_modulator asound fftw3 fl jack rt sndfile sound_utils)
