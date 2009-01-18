/* maestro_lex.l */

%{
#include <iostream>
#include <string>

#include "yystype.h"
#include "maestro_yacc.hh"
%}

float_literal       ([0-9]*\.?[0-9]+)

%%

[ \t]*		    {}               // White space.
[\n]		    { yylineno++; }  // New lines.
"/*".*"*/"          { yylval.text = yytext; return COMMENT; }
"("                 { return START_RIFF; }
")"                 { return END_RIFF; }
"@"                 { return AT; }
"x"                 { return TIMES; }
"&"                 { return UNION; }
{float_literal}     { yylval.value = atof(yytext); return FLOAT_LITERAL; }
