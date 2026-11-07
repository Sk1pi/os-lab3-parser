/* Точка входу: ./calc [-t] [файл]
   -t  лише показати токени (робота лексера) */
#include <stdio.h>
#include <string.h>
#include "ast.h"
#include "parser.tab.h"

extern FILE *yyin;
extern char *yytext;
int yylex(void);
int yyparse(void);

static void dump_tokens(void)
{
    int t;
    while ((t = yylex()) != 0) {
        if (t == NUMBER)      printf("NUMBER  %s\n", yytext);
        else if (t == IDENT)  printf("IDENT   %s\n", yytext);
        else if (t == '\n')   printf("EOL\n");
        else                  printf("'%c'\n", t);
    }
}

int main(int argc, char **argv)
{
    int tokens_only = 0;
    const char *path = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0) tokens_only = 1;
        else path = argv[i];
    }

    if (path && !(yyin = fopen(path, "r"))) { perror(path); return 1; }

    if (tokens_only) { dump_tokens(); return 0; }

    program_begin("out.c", "viz/ast.js");
    int rc = yyparse();
    program_end();

    fprintf(stderr, "\nЗгенеровано: out.c (код на C), viz/ast.js (AST для візуалізації)\n");
    return rc;
}
