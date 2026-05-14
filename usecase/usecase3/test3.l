%{
#include <stdio.h>
%}
%%
[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]+   { printf("email: %s\n", yytext); }
https?"://"[a-zA-Z0-9./?=&#_%-]+                  { printf("url: %s\n", yytext); }
[ \t\n]+                                         /* 忽略 */
.                                                /* 忽略其他 */
%%
int main() {
    yylex();
    return 0;
}
