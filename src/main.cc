#define YY_NO_UNPUT

#include <assert.h>
#include <stdlib.h>
#include <iostream>
#include <string>

#include "renderer.h"
#include "segment.h"
#include "yystype.h"

extern int yyparse(Segment<SampleType>*);

using namespace std;

int main(int argc, char **argv) {
  if ((argc > 1) && (freopen(argv[1], "r", stdin) == NULL)) {
    cerr << argv[0] << ": File " << argv[1] << " cannot be opened.\n";
    exit(1);
  }

  //
  cout << "Parsing [" << argv[1] << "]..." << endl;
  Segment<SampleType> segment;
  assert(!yyparse(&segment));

  //
  cout << "Rendering..." << endl;
  Renderer<SampleType, AccumulatorType> renderer;
  renderer.WriteWAV(segment, "result.wav");

  return 0;
}
