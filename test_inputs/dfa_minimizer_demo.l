%{
#include <stdio.h>
%}

LETTER    [a-zA-Z_]
DIGIT     [0-9]
ID        {LETTER}({LETTER}|{DIGIT})*

%%
"ab"                { return 1; }
"a"                 { return 2; }
{ID}                 { return 3; }
[0-9]+               { return 4; }
[ \t\r\n]+          { /* skip */ }
%%

int yywrap(void) {
  return 1;
}