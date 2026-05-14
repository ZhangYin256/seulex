%{
int char_count = 0, digit_count = 0, space_count = 0, line_count = 0;
%}
%%
[a-z]   { char_count++; }
[0-9]   { digit_count++; }
[ \t]   { space_count++; }
\n      { line_count++; }
.       { /* 忽略其他字符 */ }
%%
int main() {
    yylex();
    printf("Chars: %d, Digits: %d, Spaces: %d, Lines: %d\n",
           char_count, digit_count, space_count, line_count);
    return 0;
}
