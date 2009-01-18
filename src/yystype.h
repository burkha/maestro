// Define the type used in the parser.

#ifndef YYSTYPE_H_
#define YYSTYPE_H_

#include "segment.h"

typedef int SampleType;
typedef long long AccumulatorType;

struct yystype {
  Segment<SampleType> segment;
  double value;
  std::string text;
};

#define YYSTYPE yystype

#endif  // YYSTYPE_H_
