#ifndef AST_H
#define AST_H

#include <stdio.h>

/* Типи вузлів абстрактного синтаксичного дерева (AST) */
typedef enum {
    N_NUM,     /* число:            3.14              */
    N_VAR,     /* змінна:           x                 */
    N_BINOP,   /* бінарна операція: left op right     */
    N_NEG,     /* унарний мінус:    -left             */
    N_ASSIGN   /* присвоєння:       name = left       */
} NodeType;

typedef struct Node {
    NodeType type;
    double num;           /* N_NUM                         */
    char *name;           /* N_VAR, N_ASSIGN               */
    char op;              /* N_BINOP: + - * / % ^          */
    struct Node *left;    /* операнд / права частина       */
    struct Node *right;   /* другий операнд (N_BINOP)      */
} Node;

/* Побудова вузлів (викликаються з дій граматики в parser.y) */
Node *node_num(double v);
Node *node_var(char *name);
Node *node_binop(char op, Node *l, Node *r);
Node *node_neg(Node *e);
Node *node_assign(char *name, Node *e);
void  node_free(Node *n);

/* Обробка одного розібраного оператора: обчислення, генерація коду, JSON */
void program_begin(const char *c_path, const char *json_path);
void handle_statement(Node *stmt);
void program_end(void);

#endif
