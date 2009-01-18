/* maestro_yacc.y */

%{
#include <iostream>
#include <string>

#include "yystype.h"
#include "note.h"

extern int yyerror(Segment<SampleType>* result, const char* message);
extern int yylex(void);
%}

%parse-param {Segment<SampleType>* result}

%start	input

%token  COMMENT
%token  FLOAT_LITERAL
%token	START_RIFF
%token	END_RIFF
%token  AT
%token  TIMES
%token  UNION

%%

input:		/* empty */
                | input COMMENT { *result = Concatenate(*result, $1.segment); }
                | input segment	{ *result = Concatenate(*result, Concatenate($1.segment, $2.segment)); }
		;

segment:	riff { $$.segment = $1.segment; }
                | segment UNION riff { $$.segment = Union($1.segment, $3.segment); }
		;

riff:           START_RIFF note_list END_RIFF  { $$.segment = $2.segment; }
                ;
note_list:      note_list note { $$.segment = Concatenate($1.segment, $2.segment); }
                | note { $$.segment = $1.segment; }
                ;
note:           FLOAT_LITERAL { $$.segment = Note(1.0, $1.value, 1.0); }
                | FLOAT_LITERAL AT FLOAT_LITERAL TIMES FLOAT_LITERAL { $$.segment = Note($3.value, $1.value, $5.value); }
                ;

%%

int yyerror(Segment<SampleType>* result, const char* message) {
  extern int yylineno;	// defined and maintained in lex.c
  extern char *yytext;	// defined and maintained in lex.c

  std::cerr << "ERROR: " << message << " at symbol \"" << yytext
            << "\" on line " << yylineno << std::endl;
  exit(1);

  return -1;
}
