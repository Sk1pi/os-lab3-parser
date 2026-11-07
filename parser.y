%{
/* Синтаксичний аналізатор (bison = сучасний yacc).
   Граматика арифметичних виразів зі змінними. */
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"

int yylex(void);
extern int yylineno;
void yyerror(const char *s);
%}

%define parse.error verbose

%union {
    double num;
    char  *id;
    Node  *node;
}

%token <num> NUMBER
%token <id>  IDENT
%type  <node> stmt expr

/* Пріоритети й асоціативність: від найнижчого до найвищого.
   Саме вони розвʼязують shift/reduce конфлікти граматики expr. */
%left  '+' '-'
%left  '*' '/' '%'
%precedence NEG          /* унарний мінус */
%right '^'               /* степінь: 2^3^2 = 2^(3^2) */

%%

program
    : %empty
    | program line
    ;

line
    : '\n'                      /* порожній рядок */
    | stmt '\n'                 { handle_statement($1); }
    | error '\n'                { yyerrok; /* відновлення після помилки */ }
    ;

stmt
    : IDENT '=' expr            { $$ = node_assign($1, $3); }
    | expr                      { $$ = $1; }
    ;

expr
    : NUMBER                    { $$ = node_num($1); }
    | IDENT                     { $$ = node_var($1); }
    | expr '+' expr             { $$ = node_binop('+', $1, $3); }
    | expr '-' expr             { $$ = node_binop('-', $1, $3); }
    | expr '*' expr             { $$ = node_binop('*', $1, $3); }
    | expr '/' expr             { $$ = node_binop('/', $1, $3); }
    | expr '%' expr             { $$ = node_binop('%', $1, $3); }
    | expr '^' expr             { $$ = node_binop('^', $1, $3); }
    | '-' expr %prec NEG        { $$ = node_neg($2); }
    | '(' expr ')'              { $$ = $2; }
    ;

%%

void yyerror(const char *s)
{
    fprintf(stderr, "Синтаксична помилка (рядок %d): %s\n", yylineno, s);
}
