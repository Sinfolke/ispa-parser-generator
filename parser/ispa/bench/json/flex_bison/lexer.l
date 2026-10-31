%option noyywrap nounput noinput
%{
#include "parser.tab.h"
%}
%%
[ \t\r\n]+      ;
\"[^\"]*\"      { return STRING; }
[0-9]+(\.[0-9]+)? { return NUMBER; }
"true"|"false"  { return BOOLEAN; }
[\[\]{},:]      { return yytext[0]; }
.               { return YYUNDEF; }
%%
