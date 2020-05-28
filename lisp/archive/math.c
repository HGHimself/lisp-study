#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>

#include "mpc.h"

typedef struct {
  int type;
  double num;
  int err;
} lval;

enum { LVAL_NUM, LVAL_ERR };

enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

void print_ast( mpc_ast_t *a, size_t depth );
void print_n_tabs( size_t n );

lval eval_op( lval x, char *op, lval y );
lval eval_func( lval x, char *op );
lval eval( mpc_ast_t *t );

lval lval_num( double x );
lval lval_err( int x );

void lval_print( lval v );
void lval_println( lval v );

double power( double base, long exp );
double min( double x, double y );
double max( double x, double y );

int main( int argc, char** argv )  {

  mpc_parser_t* Number    = mpc_new("number");
  mpc_parser_t* Operator  = mpc_new("operator");
  mpc_parser_t* Expr      = mpc_new("expr");
  mpc_parser_t* Lispy     = mpc_new("lispy");

  mpca_lang(MPCA_LANG_DEFAULT,
  "                                                                 \
    number    : /[0-9]+(\\.[0-9]*)?/ ;                                        \
    operator  : '+' | '-' | '/' | '*' | '%' | '!' | '|' | '&' | '^' | '~' | \"<<\" | \">>\" | \"min\" | \"max\" ; \
    expr      : <number> | '(' <operator> <expr>+ ')' ;             \
    lispy     : /^/ <operator> <expr>+ /$/ ;                        \
  ",
  Number, Operator, Expr, Lispy);

  puts("Lispy Version 0.0.0.0.1");
  puts("Press Ctrl+c to Exit");

  while ( 1 )  {
    char* input = readline("lispy> ");

    add_history(input);

    mpc_result_t r;
    if ( mpc_parse("<stdin>", input, Lispy, &r) )  {
      lval result = eval(r.output);
      lval_println(result);
      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }

  mpc_cleanup(4, Number, Operator, Expr, Lispy);

  return 0;
}

lval lval_num( double x )  {
  lval v;
  v.type = LVAL_NUM;
  v.num = x;
  return v;
}

lval lval_err( int x )  {
  lval v;
  v.type = LVAL_ERR;
  v.err = x;
  return v;
}

lval eval( mpc_ast_t *t )  {
  // base case, a number is simple enough to convert to int
  if ( strstr(t->tag, "number") )  {
    errno = 0;
    double x = strtod(t->contents, NULL);

    return errno != ERANGE
      ? lval_num(x)
      : lval_err(LERR_BAD_NUM);
  }
  // first child is the opening parenthesis => ignore

  // operator is always the second child
  char* op = t->children[1]->contents;
  // eval this, could be an expression or a number
  // ?wonder what happens if its a parenthesis?
  // looks like we just return x
  lval x = eval(t->children[2]);

  int i = 3;
  while ( strstr(t->children[i]->tag, "exp") )  {
    // we will continue to accumulate into x
    // by evaluating each child and executing 'op' to it
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }

  if ( i == 3 )  {
    // single arity function, special cases
    x = eval_func(x, op);
  }

  return x;
}

lval eval_op( lval x, char *op, lval y )  {
  if ( x.type == LVAL_ERR )  { return x; }
  if ( y.type == LVAL_ERR )  { return y; }

  double a = x.num;
  double b = y.num;

  if ( strcmp(op, "+") == 0 ) { return lval_num(a + b); }
  if ( strcmp(op, "-") == 0 ) { return lval_num(a - b); }
  if ( strcmp(op, "*") == 0 ) { return lval_num(a * b); }
  if ( strcmp(op, "/") == 0 ) {
    return  b == 0
      ? lval_err(LERR_DIV_ZERO)
      : lval_num(a / b);
  }
  if ( strcmp(op, "%") == 0 ) { return lval_num((long)a % (long)b); }
  if ( strcmp(op, "|") == 0 ) { return lval_num((long)a | (long)b); }
  if ( strcmp(op, "&") == 0 ) { return lval_num((long)a & (long)b); }
  if ( strcmp(op, "^") == 0 ) { return lval_num((long)a ^ (long)b); }
  if ( strcmp(op, ">>") == 0 ) { return lval_num((long)a >> (long)b); }
  if ( strcmp(op, "<<") == 0 ) { return lval_num((long)a << (long)b); }
  if ( strcmp(op, "**") == 0 ) { return lval_num(power(a, b)); }
  if ( strcmp(op, "min") == 0 ) { return lval_num(min(a, b)); }
  if ( strcmp(op, "min") == 0 ) { return lval_num(max(a, b)); }

  return lval_err(LERR_BAD_OP);
}

lval eval_func( lval x, char *op )  {
  if ( x.type == LVAL_ERR )  { return x; }

  double a = x.num;

  if ( strcmp(op, "-") == 0 ) { return lval_num( -a ); }
  if ( strcmp(op, "!") == 0 ) { return lval_num( !a ); }
  if ( strcmp(op, "~") == 0 ) { return lval_num( ~((long) a) ); }

  return lval_err(LERR_BAD_OP);
}

void lval_print( lval v )  {
  switch ( v.type )  {
    case LVAL_NUM:
      printf( "%f", v.num );
    break;

    case LVAL_ERR:
      if ( v.err == LERR_DIV_ZERO )  {
        printf( "Error: Dvision by zero!" );
      }
      else if ( v.err == LERR_BAD_OP )  {
        printf( "Error: Invalid operation!" );
      }
      else if ( v.err == LERR_BAD_NUM )  {
        printf( "Error: Invalid number!" );
      }
    break;
  }
}

void lval_println( lval v ) {
  lval_print( v );
  putchar('\n');
}

void print_ast( mpc_ast_t *a, size_t depth )  {
  print_n_tabs( depth );
  printf( "Tag::%s\n", a->tag );
  print_n_tabs( depth );
  printf( "Contents::%s\n", a->contents );
  print_n_tabs( depth );
  printf( "#ofChildren::%d\n", a->children_num );
  printf( "-----------------------\n");

  for ( size_t i = 0; i < a->children_num; i++ )  {
    print_ast(a->children[i], depth + 1);
  }
}

void print_n_tabs( size_t n )  {
  for ( size_t i = 0; i < n; i++ )  {
    printf("  ");
  }
}

double power(double base, long exp)  {
  if(0 == exp) return 1;
  double n = power(base, exp/2);
  if(0 == exp%2) return n*n;
  if(exp > 0) return base*n*n;
  return (n*n)/base;
}

double min( double x, double y )  {
  if ( x <= y ) return x;
  return y;
}

double max( double x, double y )  {
  if ( x >= y ) return x;
  return y;
}
