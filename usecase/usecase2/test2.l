%{
#include <stdio.h>
%}
%%
"int"       { printf("keyword: int\n"); }
"return"    { printf("keyword: return\n"); }
[a-zA-Z_][a-zA-Z0-9_]*  { printf("identifier: %s\n", yytext); }
[ \t\n]+    /* 忽略空白 */
.           { printf("unknown: %c\n", yytext[0]); }
%%
int main() {
    yylex();
    return 0;
}
