/* AST: побудова, обчислення (семантика), генерація коду на C, експорт у JSON */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ast.h"

/* ---------- Побудова вузлів ---------- */

static Node *new_node(NodeType t)
{
    Node *n = calloc(1, sizeof(Node));
    if (!n) { perror("calloc"); exit(1); }
    n->type = t;
    return n;
}

Node *node_num(double v)            { Node *n = new_node(N_NUM);    n->num = v;  return n; }
Node *node_var(char *name)          { Node *n = new_node(N_VAR);    n->name = name; return n; }
Node *node_neg(Node *e)             { Node *n = new_node(N_NEG);    n->left = e; return n; }
Node *node_assign(char *name, Node *e) { Node *n = new_node(N_ASSIGN); n->name = name; n->left = e; return n; }
Node *node_binop(char op, Node *l, Node *r)
{
    Node *n = new_node(N_BINOP);
    n->op = op; n->left = l; n->right = r;
    return n;
}

void node_free(Node *n)
{
    if (!n) return;
    node_free(n->left);
    node_free(n->right);
    free(n->name);
    free(n);
}

/* ---------- Таблиця змінних ---------- */

#define MAX_VARS 256
static struct { char *name; double value; } vars[MAX_VARS];
static int nvars = 0;

static int find_var(const char *name)
{
    for (int i = 0; i < nvars; i++)
        if (strcmp(vars[i].name, name) == 0) return i;
    return -1;
}

static void set_var(const char *name, double v)
{
    int i = find_var(name);
    if (i < 0) {
        if (nvars == MAX_VARS) { fprintf(stderr, "Забагато змінних\n"); exit(1); }
        i = nvars++;
        vars[i].name = strdup(name);
    }
    vars[i].value = v;
}

/* ---------- Семантика: обчислення ---------- */

static int  eval_error;
static char eval_msg[128];

static double fail(const char *msg, const char *arg)
{
    if (!eval_error) {
        eval_error = 1;
        snprintf(eval_msg, sizeof eval_msg, msg, arg);
    }
    return 0;
}

/* Чисте обчислення: нічого не змінює (присвоєння виконує handle_statement) */
static double eval(const Node *n)
{
    switch (n->type) {
    case N_NUM:    return n->num;
    case N_VAR: {
        int i = find_var(n->name);
        return i < 0 ? fail("змінна '%s' не визначена", n->name) : vars[i].value;
    }
    case N_NEG:    return -eval(n->left);
    case N_ASSIGN: return eval(n->left);
    case N_BINOP: {
        double a = eval(n->left), b = eval(n->right);
        switch (n->op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': return b == 0 ? fail("ділення на нуль%s", "") : a / b;
        case '%': return b == 0 ? fail("ділення на нуль%s", "") : fmod(a, b);
        case '^': return pow(a, b);
        }
    }
    }
    return 0;
}

/* ---------- Друк виразу з повними дужками (показує, як розібрано) ---------- */

static void print_expr(FILE *f, const Node *n)
{
    switch (n->type) {
    case N_NUM:    fprintf(f, "%g", n->num); break;
    case N_VAR:    fprintf(f, "%s", n->name); break;
    case N_NEG:    fprintf(f, "(-"); print_expr(f, n->left); fprintf(f, ")"); break;
    case N_ASSIGN: fprintf(f, "%s = ", n->name); print_expr(f, n->left); break;
    case N_BINOP:
        fprintf(f, "(");
        print_expr(f, n->left);
        fprintf(f, " %c ", n->op);
        print_expr(f, n->right);
        fprintf(f, ")");
        break;
    }
}

/* ---------- Генерація коду на C ---------- */

static FILE *c_out;
static char *declared[MAX_VARS];
static int   ndeclared = 0;

static int is_declared(const char *name)
{
    for (int i = 0; i < ndeclared; i++)
        if (strcmp(declared[i], name) == 0) return 1;
    return 0;
}

static void gen_c_expr(FILE *f, const Node *n)
{
    switch (n->type) {
    case N_NUM: {
        /* літерал має бути double: 2 -> 2.0, інакше в C буде цілочисельна арифметика */
        char buf[64];
        snprintf(buf, sizeof buf, "%.17g", n->num);
        fprintf(f, "%s%s", buf, strpbrk(buf, ".eE") ? "" : ".0");
        break;
    }
    case N_VAR:    fprintf(f, "%s", n->name); break;
    case N_NEG:    fprintf(f, "(-"); gen_c_expr(f, n->left); fprintf(f, ")"); break;
    case N_ASSIGN: gen_c_expr(f, n->left); break;
    case N_BINOP:
        if (n->op == '^' || n->op == '%') {          /* у C немає таких операторів */
            fprintf(f, "%s(", n->op == '^' ? "pow" : "fmod");
            gen_c_expr(f, n->left);
            fprintf(f, ", ");
            gen_c_expr(f, n->right);
            fprintf(f, ")");
        } else {
            fprintf(f, "(");
            gen_c_expr(f, n->left);
            fprintf(f, " %c ", n->op);
            gen_c_expr(f, n->right);
            fprintf(f, ")");
        }
        break;
    }
}

/* Генерує один оператор C у потік f */
static void gen_c_stmt(FILE *f, const Node *s)
{
    if (s->type == N_ASSIGN) {
        if (is_declared(s->name)) {
            fprintf(f, "    %s = ", s->name);
        } else {
            fprintf(f, "    double %s = ", s->name);
            declared[ndeclared++] = strdup(s->name);
        }
        gen_c_expr(f, s->left);
        fprintf(f, ";\n    printf(\"%s = %%g\\n\", %s);\n", s->name, s->name);
    } else {
        fprintf(f, "    printf(\"%%g\\n\", ");
        gen_c_expr(f, s);
        fprintf(f, ");\n");
    }
}

/* ---------- Експорт AST у JSON для візуалізації ---------- */

static FILE *json_out;
static int   stmt_count = 0;

static void json_str(FILE *f, const char *s)
{
    fputc('"', f);
    for (; *s; s++) {
        if (*s == '"' || *s == '\\') { fputc('\\', f); fputc(*s, f); }
        else if (*s == '\n') fputs("\\n", f);
        else fputc(*s, f);
    }
    fputc('"', f);
}

static void json_value(FILE *f, const Node *n)
{
    eval_error = 0;
    double v = eval(n);
    if (!eval_error && isfinite(v)) fprintf(f, ",\"value\":%.10g", v);
}

static void json_node(FILE *f, const Node *n)
{
    char label[64];
    const char *kind = "";
    switch (n->type) {
    case N_NUM:    kind = "number";   snprintf(label, sizeof label, "%g", n->num); break;
    case N_VAR:    kind = "variable"; snprintf(label, sizeof label, "%s", n->name); break;
    case N_NEG:    kind = "unary";    snprintf(label, sizeof label, "-"); break;
    case N_BINOP:  kind = "operator"; snprintf(label, sizeof label, "%c", n->op); break;
    case N_ASSIGN: kind = "assign";   snprintf(label, sizeof label, "="); break;
    }
    fprintf(f, "{\"name\":");
    json_str(f, label);
    fprintf(f, ",\"kind\":\"%s\"", kind);
    json_value(f, n);

    fprintf(f, ",\"children\":[");
    if (n->type == N_ASSIGN) {
        fprintf(f, "{\"name\":");
        json_str(f, n->name);
        fprintf(f, ",\"kind\":\"target\",\"children\":[]},");
    }
    if (n->left)  json_node(f, n->left);
    if (n->right) { fputc(',', f); json_node(f, n->right); }
    fprintf(f, "]}");
}

/* ---------- Програма: обробка операторів по черзі ---------- */

void program_begin(const char *c_path, const char *json_path)
{
    c_out = fopen(c_path, "w");
    json_out = fopen(json_path, "w");
    if (!c_out || !json_out) { perror("fopen"); exit(1); }

    fprintf(c_out,
        "/* Згенеровано транслятором calc. Збирання: gcc out.c -o out -lm */\n"
        "#include <stdio.h>\n"
        "#include <math.h>\n\n"
        "int main(void)\n{\n");
    fprintf(json_out, "window.AST_DATA = [\n");
}

void handle_statement(Node *s)
{
    stmt_count++;

    /* 1. Обчислення (до зміни змінних, щоб значення в JSON були коректні) */
    eval_error = 0;
    double result = eval(s);
    int    failed = eval_error;
    char   msg[128];
    strcpy(msg, eval_msg);

    /* 2. Текст виразу з дужками та згенерований C-код як рядки */
    char *text = NULL, *code = NULL;
    size_t tlen, clen;
    FILE *tf = open_memstream(&text, &tlen);
    print_expr(tf, s);
    fclose(tf);

    FILE *cf = open_memstream(&code, &clen);
    if (!failed) gen_c_stmt(cf, s);
    fclose(cf);

    /* 3. JSON для візуалізації */
    if (stmt_count > 1) fprintf(json_out, ",\n");
    fprintf(json_out, "{\"index\":%d,\"text\":", stmt_count);
    json_str(json_out, text);
    if (failed) {
        fprintf(json_out, ",\"error\":");
        json_str(json_out, msg);
    } else if (isfinite(result)) {
        fprintf(json_out, ",\"result\":%.10g", result);
    }
    fprintf(json_out, ",\"code\":");
    json_str(json_out, code);
    fprintf(json_out, ",\"ast\":");
    json_node(json_out, s);
    fprintf(json_out, "}");

    /* 4. Результат і генерація коду */
    if (failed) {
        fprintf(stderr, "[%d] %s  ->  помилка: %s\n", stmt_count, text, msg);
    } else {
        fprintf(stderr, "[%d] %s\n", stmt_count, text);
        if (s->type == N_ASSIGN) {
            set_var(s->name, result);
            printf("%s = %g\n", s->name, result);
        } else {
            printf("%g\n", result);
        }
        fputs(code, c_out);
    }

    free(text);
    free(code);
    node_free(s);
}

void program_end(void)
{
    fprintf(c_out, "    return 0;\n}\n");
    fprintf(json_out, "\n];\n");
    fclose(c_out);
    fclose(json_out);
}
