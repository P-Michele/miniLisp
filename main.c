#include <stdio.h>
#include <stdlib.h>
#include <math.h>
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

typedef struct {
    int type;

    union {
            long l;   // long per interi
            double d; // double per float
        };
    
    char* err;

} lval;

enum { LVAL_INT, LVAL_FLOAT, LVAL_ERR };

enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

/*
    Constructor for an integer value
*/
lval* lval_int(long x) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_INT;
    v->l = x;
    return v;
}

/*
    Constructor for a float value
*/
lval* lval_float(double x) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_FLOAT;
    v->d = x;
    return v;
}

/*
    Constructor for an error value
*/
lval* lval_err(char* m) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    v->err = malloc(strlen(m) + 1);
    strcpy(v->err, m);
    return v;
}

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
      "                                                  \
    number : /-?[0-9]+(\\.[0-9]+)?/ ;                    \
    operator : '+' | '-' | '*' | '/' ;                   \
    expr     : <number> | '(' <operator> <expr>+ ')' ;   \
    miniLisp    : /^/ '(' <operator> <expr>+ ')' /$/ ;   \
  ",
      Number, Operator, Expr, miniLisp);
}

/* 
    This function will return the appropriate type of lval
    for a given number in the ast
*/
lval* lval_read_num(mpc_ast_t* t) {
    char* contents = t->contents;

    //float case
    if (strchr(contents, '.')) { 

        double x = strtod(contents, NULL);

        errno = 0;
        if (errno == ERANGE) {
            return lval_err("Invalid number: outside range");
        }
        
        return lval_float(x);
    //long case
    } else {
        errno = 0; 
        long x = strtol(contents, NULL, 10);
        if (errno == ERANGE) { 
            return lval_err("Invalid number: exceed long max value");
        }
        
        return lval_int(x);
    }
}


/*
    Helper function to convert lval to double
*/
double lval_to_double(lval* v) {
    return (v->type == LVAL_FLOAT) ? v->d : (double)v->l;
}

/*
    Addition operation
*/
lval* lval_add(lval* x, lval* y) {
    if (x->type == LVAL_FLOAT || y->type == LVAL_FLOAT) {
        double xv = lval_to_double(x);
        double yv = lval_to_double(y);
        free(x);
        free(y);
        return lval_float(xv + yv);
    } else {
        x->l += y->l;
        free(y);
        return x;
    }
}

/*
    Subtraction operation
*/
lval* lval_sub(lval* x, lval* y) {
    if (x->type == LVAL_FLOAT || y->type == LVAL_FLOAT) {
        double xv = lval_to_double(x);
        double yv = lval_to_double(y);
        free(x);
        free(y);
        return lval_float(xv - yv);
    } else {
        x->l -= y->l;
        free(y);
        return x;
    }
}

/*
    Multiplication operation
*/
lval* lval_mul(lval* x, lval* y) {
    if (x->type == LVAL_FLOAT || y->type == LVAL_FLOAT) {
        double xv = lval_to_double(x);
        double yv = lval_to_double(y);
        free(x);
        free(y);
        return lval_float(xv * yv);
    } else {
        x->l *= y->l;
        free(y);
        return x;
    }
}

/*
    Division operation with zero check
*/
lval* lval_div(lval* x, lval* y) {
    if ((y->type == LVAL_INT && y->l == 0) || 
        (y->type == LVAL_FLOAT && fabs(y->d) < 1e-10)) {
        free(x);
        free(y);
        return lval_err("Division by zero");
    }
    
    double xv = lval_to_double(x);
    double yv = lval_to_double(y);
    free(x);
    free(y);
    return lval_float(xv / yv);
}

/*
    Evaluate operation based on operator
*/
lval* lval_eval_op(lval* x, char* op, lval* y) {

    if(x->err != NULL) { free(y); return x;}
    if(y->err != NULL) { free(x); return y;}

    if (strcmp(op, "+") == 0) {
        return lval_add(x, y);
    }
    if (strcmp(op, "-") == 0) {
        return lval_sub(x, y);
    }
    if (strcmp(op, "*") == 0) {
        return lval_mul(x, y);
    }
    if (strcmp(op, "/") == 0) {
        return lval_div(x, y);
    }

    free(x);
    free(y);
    return lval_err("Unknown operator");
}

/*
    This function will evaluate the AST and return an lval
*/
lval* lval_eval(mpc_ast_t* t) {
    //If the node is a number, return it directly
    if (strstr(t->tag, "number")) {
        return lval_read_num(t);
    }

    //The operator is always the second child
    char* op = t->children[1]->contents;

    //Store the first expression
    lval* x = lval_eval(t->children[2]);

    //Iterate over the remaining expressions
    int i = 3;
    //A number is always an expression
    while (i < t->children_num && strstr(t->children[i]->tag, "expr")) {
        lval* y = lval_eval(t->children[i]);
        x = lval_eval_op(x, op, y);
        i++;
    }

    return x;
}

void lval_print(lval* val){
    if (val->type == LVAL_INT) {
        printf("%li", val->l);
    } else if (val->type == LVAL_FLOAT) {
        printf("%lf", val->d);
    } else if (val->type == LVAL_ERR) {
        printf("Error: %s", val->err);
    }
}

void lval_delete(lval* val){
    if (val->type == LVAL_ERR) {
        free(val->err);
    }
    free(val);
}

int main(void) {
    puts("MiniLisp version 0.0.1");
    parserSetup();

        while(1) {
            char* input = readline("miniLisp> \n");
            add_history(input);

            //Start parsing
            mpc_result_t result;
            if (mpc_parse("<stdin>", input, miniLisp, &result)) {
                mpc_ast_t* tree = result.output;
                //Evaluate the AST
                lval* result_val = lval_eval(tree);
                //Print the result
                lval_print(result_val);
                lval_delete(result_val);

                mpc_ast_delete(result.output);
            } else {
                mpc_err_print(result.error);    // parsing failed
                mpc_err_delete(result.error);
            }

            free(input);
        }
        
    mpc_cleanup(4, Number, Operator, Expr, miniLisp);
    return 0;
}