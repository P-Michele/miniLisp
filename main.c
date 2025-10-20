#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"
#include "mpc.c"

#ifdef _WIN32
#include <string.h>

static char buffer[2048];

char* readline(const char* prompt) {
    fputs(prompt, stdout);
    fflush(stdout);
    if (!fgets(buffer, sizeof buffer, stdin))
        return NULL;
    //removes spaces
    buffer[strcspn(buffer, "\r\n")] = 0;
    size_t len = strlen(buffer);
    char *result = malloc(len + 1);
    if (!result)
        return NULL;
    strcpy(result, buffer);
    return result;
}

void add_history(char* unused) {
    //Do Nothing
}

#else
#include <editline/readline.h>
#include <editline/history.h>
#endif

//Globals
mpc_parser_t* Number;
mpc_parser_t* Operator;
mpc_parser_t* Expr;
mpc_parser_t* miniLisp;


/*
    Setup a parser for miniLisp, it includes float,integer and math operations
 *
 */
void parserSetup() {
    Number   = mpc_new("number");
    Operator = mpc_new("operator");
    Expr     = mpc_new("expr");
    miniLisp    = mpc_new("miniLisp");

    /* Define them with the following Language */
    mpca_lang(MPCA_LANG_DEFAULT,
      "                                          \
    number : /-?[0-9]+(\\.[0-9]+)?/ ;                    \
    operator : '+' | '-' | '*' | '/' ;                   \
    expr     : <number> | '(' <operator> <expr>+ ')' ;   \
    miniLisp    : /^/ <operator> <expr>+ /$/ ;           \
  ",
      Number, Operator, Expr, miniLisp);
}

int main(void) {
    puts("MiniLisp version 0.0.1");
    parserSetup();


        char* input = readline("miniLisp> ");
        add_history(input);

        //Start parsing
        mpc_result_t result;
        if (mpc_parse("<stdin>", input, miniLisp, &result)) {
            mpc_ast_t* tree = result.output;
            mpc_ast_print(result.output);   // parsing ok

            printf("tag: %s\n", tree->tag);
            printf("contents: %s\n", tree->contents);
            printf("children_num: %i\n", tree->children_num);

            mpc_ast_delete(result.output);
        } else {
            mpc_err_print(result.error);    // parsing fallito
            mpc_err_delete(result.error);
        }

    mpc_cleanup(4, Number, Operator, Expr, miniLisp);
    free(input);
    return 0;
}