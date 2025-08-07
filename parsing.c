#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"

//Globals
static char buffer[2048];

/* If we are compiling on Windows compile these functions */
#ifdef _WIN32
#include <string.h>

int strcasecmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        int d = tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
        if (d != 0) return d;
        s1++; s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

/* Fake readline function */
char* readline(char* prompt) {
    fputs(prompt, stdout);
    fgets(buffer, 2048, stdin);
    char* cpy = malloc(strlen(buffer)+1);
    strcpy(cpy, buffer);
    cpy[strlen(cpy)-1] = '\0';
    return cpy;
}

/* Fake add_history function */
void add_history(char* unused) {}

/* Otherwise include the editline headers */
#else
#include <editline/readline.h>
#include <editline/history.h>
#include <ctype.h>
#endif

// Define enum for operator types
typedef enum {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_POW,
    OP_UNKNOWN
} OperatorType;

OperatorType get_operator_type(const char* op) {
    if (!op) return OP_UNKNOWN; // errore allocazione

    OperatorType result = OP_UNKNOWN;
    if (strcasecmp(op, "+") == 0 || strcasecmp(op, "add") == 0) result = OP_ADD;
    else if (strcasecmp(op, "-") == 0 || strcasecmp(op, "sub") == 0) result = OP_SUB;
    else if (strcasecmp(op, "*") == 0 || strcasecmp(op, "mul") == 0) result = OP_MUL;
    else if (strcasecmp(op, "/") == 0 || strcasecmp(op, "div") == 0) result = OP_DIV;
    else if (strcasecmp(op, "%") == 0 || strcasecmp(op, "mod") == 0) result = OP_MOD;
    else if (strcasecmp(op, "^") == 0 || strcasecmp(op, "pow") == 0) result = OP_POW;

    return result;
}

long eval_op(long x, long y, OperatorType op) {
    switch (op) {
        case OP_ADD: return x + y;
        case OP_SUB: return x - y;
        case OP_MUL: return x * y;
        case OP_DIV: return (y != 0) ? x / y : 0;
        case OP_MOD: return (y != 0) ? x % y : 0;
        case OP_POW: return (long)pow(x, y);
        default: 0; // Errore: operatore sconosciuto
    }
}

long eval(mpc_ast_t* t) {
    //If tagged as numerb return it
    if (strstr(t->tag, "number")) {
        return atoi(t->contents);
    }

    char* op = t->children[1]->contents;

    long x = eval(t->children[2]);

    for(int i = 3; i < t->children_num; i++){
        long y = eval(t->children[i]);
        OperatorType operator_type = get_operator_type(op);
        x = eval_op(x, y, operator_type);
    }

}

int main(int argc, char** argv) {

    mpc_parser_t* Number   = mpc_new("number");
    mpc_parser_t* Operator = mpc_new("operator");
    mpc_parser_t* Expr     = mpc_new("expr");
    mpc_parser_t* Lispy    = mpc_new("lispy");

    mpca_lang(MPCA_LANG_DEFAULT,
    "                                                     \
    number : /-?(?:[0-9]*\\.[0-9]+|[0-9]+)/ ;                             \
    operator : '+' | '-' | '*' | '/' | '%' | '^'| \"add\" | \"sub\" | \"mul\" | \"div\" | \"mod\" | \"pow\" ; \
    expr     : <number> | '(' <operator> <expr>+ ')' ;  \
    lispy    : /^/ <operator> <expr>+ /$/ ;             \
    ",
    Number, Operator, Expr, Lispy);

    puts("Lispy");
    puts("Press Ctrl+c to Exit\n");

    while (1) {

        char* input = readline("lispy> ");

        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r)) {
        /*Print the AST */
        mpc_ast_print(r.output);
        mpc_ast_delete(r.output);
        } else {
        /* Otherwise Print the Error */
        mpc_err_print(r.error);
        mpc_err_delete(r.error);
        }
        free(input);
    }

    mpc_cleanup(4, Number, Operator, Expr, Lispy);
    return 0;
}