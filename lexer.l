%{
/* Лексичний аналізатор: перетворює текст на потік токенів */
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "parser.tab.h"
%}

%option noyywrap nounput noinput yylineno

DIGITS  [0-9]+
NUMBER  {DIGITS}(\.{DIGITS})?([eE][-+]?{DIGITS})?
IDENT   [A-Za-z_][A-Za-z0-9_]*

%%

[ \t\r]+        { /* пропускаємо пробіли */ }
"#".*           { /* коментар до кінця рядка */ }
{NUMBER}        { yylval.num = atof(yytext); return NUMBER; }
{IDENT}         { yylval.id = strdup(yytext); return IDENT; }
\n|";"          { return '\n'; /* кінець оператора */ }
[-+*/%^()=]     { return yytext[0]; }
<<EOF>>         {
                    /* щоб останній рядок без \n теж розібрався */
                    static int eol_sent = 0;
                    if (!eol_sent) { eol_sent = 1; return '\n'; }
                    return 0;
                }
.               { fprintf(stderr, "Лексична помилка (рядок %d): невідомий символ '%s'\n",
                          yylineno, yytext); }

%%
