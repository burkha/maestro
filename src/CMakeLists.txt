CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(MAESTRO)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})
INCLUDE(CMakeMacros.txt)

YACC_FILE(maestro_yacc.y)
LEX_FILE(maestro_lex.l)

ADD_EXECUTABLE(maestro
main.cc

maestro_yacc.cc
maestro_lex.cc

instrument.cc instrument.h
renderer.cc renderer.h
segment.cc segment.h
)

ADD_DEFINITIONS(
)

SET_TARGET_PROPERTIES(maestro PROPERTIES COMPILE_FLAGS "-Wall -O0 -g")

TARGET_LINK_LIBRARIES(maestro
fl sndfile
)
