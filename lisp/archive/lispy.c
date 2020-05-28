#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>

#include "mpc.h"

#define LASSERT(args, cond, fmt, ...) \
  if ( !(cond) ) { \
    lval *err = lval_err(fmt, ##__VA_ARGS__); \
    lval_del(args); \
    return err; \
  }

#define likely(x)      __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)

struct lval;
struct lenv;

typedef struct lval lval;
typedef struct lenv lenv;

enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_FUN,
        LVAL_SEXPR, LVAL_QEXPR };

typedef lval* (*lbuiltin) ( lenv*, lval* );

struct lval {
  int type;

  double num;
  char* err;
  char* sym;

  lbuiltin builtin;
  lenv *env;
  lval *formals;
  lval *body;

  int count;
  lval** cell;
};

struct lenv {
  lenv *par;
  int count;
  char **syms;
  lval **vals;
};

lenv *lenv_new( void );
void lenv_del( lenv *e );
lval *lenv_get( lenv *e, lval *k );
void lenv_put( lenv *e, lval *k, lval *v );
void lenv_def( lenv *e, lval *k, lval *v );
lenv *lenv_copy( lenv *e );
void lenv_add_builtin( lenv *e, char *name, lbuiltin func );
void lenv_add_builtins( lenv *e );

lval* lval_num( double x );
lval* lval_err( char *fmt, ... );
lval* lval_sym( char *s );
lval* lval_sexpr( void );
lval* lval_qexpr( void );
lval* lval_fun( lbuiltin func );
lval *lval_lambda( lval *formals, lval *body );
char *ltype_name( int t );
lval *lval_copy( lval *v );
lval *lval_call( lenv *e, lval *f, lval *a );

void lval_del( lval *v );

lval *lval_add( lval *v, lval *x );
lval *lval_pop( lval* v, int i );
lval *lval_push( lval* v, lval *x );
lval *lval_take( lval *v, int i );
lval *lval_join( lval *x, lval *y );

lval *lval_eval_sexpr( lenv *e, lval* v );
lval *lval_eval( lenv *e, lval *v );

lval *builtin_op( lenv *e, lval *a, char *op );
lval *builtin( lval *a, char *func );

lval *lval_read_num( mpc_ast_t *t );
lval *lval_read( mpc_ast_t* t );

void lval_expr_print( lval *v, char open, char close );
void lval_print( lval *v );
void lval_println( lval *v );

lval *builtin_add( lenv *e, lval *a );
lval *builtin_sub( lenv *e, lval *a );
lval *builtin_mul( lenv *e, lval *a );
lval *builtin_div( lenv *e, lval *a );
lval *builtin_mod( lenv *e, lval *a );
lval *builtin_not( lenv *e, lval *a );
lval *builtin_xor( lenv *e, lval *a );
lval *builtin_bwand( lenv *e, lval *a );
lval *builtin_bwor( lenv *e, lval *a );
lval *builtin_neg( lenv *e, lval *a );
lval *builtin_lshift( lenv *e, lval *a );
lval *builtin_rshift( lenv *e, lval *a );
lval *builtin_max( lenv *e, lval *a );
lval *builtin_min( lenv *e, lval *a );
lval *builtin_pow( lenv *e, lval *a );
lval *builtin_quit( lenv *e, lval *a );
lval *builtin_head( lenv *e, lval *a );
lval *builtin_tail( lenv *e, lval *a );
lval *builtin_list( lenv *e, lval *a );
lval *builtin_eval( lenv *e, lval *a );
lval *builtin_join( lenv *e, lval *a );
lval *builtin_init( lenv *e, lval *a );
lval *builtin_cons( lenv *e, lval *a );
lval *builtin_len( lenv *e, lval *a );
lval *builtin_rev( lenv *e, lval *a );
lval *builtin_if( lenv *e, lval *arguements );
lval *builtin_bool( lenv *e, lval *arguements );
lval *builtin_def( lenv *e, lval *a );
lval *builtin_env( lenv *e, lval *a );
lval *builtin_lambda( lenv *e, lval *a );
lval *builtin_put( lenv *e, lval *a );
lval *builtin_var( lenv *e, lval *a, char *func );

double power( double base, long exp );
double min( double x, double y );
double max( double x, double y );

int main( int argc, char** argv )  {

  mpc_parser_t *Number    = mpc_new("number");
  mpc_parser_t *Symbol    = mpc_new("symbol");
  mpc_parser_t *Sexpr     = mpc_new("sexpr");
  mpc_parser_t *Qexpr     = mpc_new("qexpr");
  mpc_parser_t *Expr      = mpc_new("expr");
  mpc_parser_t *Lispy     = mpc_new("lispy");

  mpca_lang(MPCA_LANG_DEFAULT,
  "                                                                 \
    number    : /[0-9]+(\\.[0-9]*)?/ ;                                        \
    symbol    : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&|^~?%]+/ ; \
    sexpr     : '(' <expr>* ')' ;             \
    qexpr     : '{' <expr>* '}' ;             \
    expr      : <number> | <symbol> | <sexpr> | <qexpr> ;             \
    lispy     : /^/ <expr>* /$/ ;                        \
  ",
  Number, Symbol, Sexpr, Qexpr, Expr, Lispy);

  puts("  🤖 :: Lispy Version 0.0.0.0.1");
  puts("  🚫 :: Use `quit` to Exit");

  lenv *e = lenv_new();
  lenv_add_builtins(e);

  while ( likely(1) )  {
    char *input = readline("lispy> ");

    add_history(input);

    mpc_result_t r;
    if ( mpc_parse("<stdin>", input, Lispy, &r) )  {
      lval *x = lval_read(r.output);
      //lval_println(x);
      x = lval_eval(e, x);
      lval_println(x);
      lval_del(x);
      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }

  lenv_del(e);

  mpc_cleanup(6, Number, Symbol, Expr, Sexpr, Qexpr, Lispy);

  return 0;
}

lenv *lenv_new( void )  {
  lenv *e = malloc( sizeof(lenv*) );
  e->par = NULL;
  e->count = 0;
  e->syms = NULL;
  e->vals = NULL;
  return e;
}

void lenv_del( lenv *e )  {
  for ( size_t i = 0; i < e->count; i++ )  {
    free(e->syms[i]);
    lval_del(e->vals[i]);
  }
  free(e->syms);
  free(e->vals);
  free(e);
}

lval *lenv_get( lenv *e, lval *k )  {
  for ( size_t i = 0; i < e->count; i++ )  {
    if ( strcmp(e->syms[i], k->sym) == 0 )  {
      return lval_copy(e->vals[i]);
    }
  }

  if ( e->par )  {
    return lenv_get(e->par, k);
  } else {
    return lval_err("Unbound Symbol :: '%s'", k->sym);
  }
}

void lenv_put( lenv *e, lval *k, lval *v )  {
  for ( size_t i = 0; i < e->count; i++ )  {
    if ( strcmp( e->syms[i], k->sym ) == 0 )  {
      lval_del(e->vals[i]);
      e->vals[i] = lval_copy(v);
      return;
    }
  }

  // we havent found it yet so we need to realloc space
  e->count++;

  e->vals = realloc( e->vals, sizeof(lval*) * e->count );
  e->syms = realloc( e->syms, sizeof(char*) * e->count );

  e->vals[e->count - 1] = lval_copy(v);
  e->syms[e->count - 1] = malloc( strlen(k->sym) + 1 );
  strcpy( e->syms[e->count - 1], k->sym );
}

void lenv_def( lenv *e, lval *k, lval *v )  {
  while ( e->par ) { e = e->par; }
  lenv_put(e, k, v);
}

lenv *lenv_copy( lenv *e )  {
  lenv *n = malloc( sizeof(lenv) );
  n->par = e->par;
  n->count = e->count;
  n->syms = malloc( sizeof(char*) * n->count );
  n->vals = malloc( sizeof(lval*) * n->count );

  for ( size_t i = 0; i < e->count; i++ )  {
    n->syms[i] = malloc( strlen(e->syms[i]) + 1 );
    strcpy( n->syms[i], e->syms[i] );
    n->vals[i] = lval_copy(e->vals[i]);
  }
  return n;
}

void lenv_add_builtin( lenv *e, char *name, lbuiltin func )  {
  lval *k = lval_sym(name);
  lval *v = lval_fun(func);

  lenv_put( e, k, v );

  lval_del(k);
  lval_del(v);
}

void lenv_add_builtins( lenv *e )  {
  lenv_add_builtin( e, "list", builtin_list );
  lenv_add_builtin( e, "tail", builtin_tail );
  lenv_add_builtin( e, "head", builtin_head );
  lenv_add_builtin( e, "join", builtin_join );
  lenv_add_builtin( e, "eval", builtin_eval );
  lenv_add_builtin( e, "init", builtin_init );
  lenv_add_builtin( e, "cons", builtin_cons );
  lenv_add_builtin( e, "len", builtin_len );
  lenv_add_builtin( e, "rev", builtin_rev );
  lenv_add_builtin( e, "?", builtin_if );
  lenv_add_builtin( e, "bool", builtin_bool );
  lenv_add_builtin( e, "def", builtin_def );

  lenv_add_builtin( e, "+", builtin_add );
  lenv_add_builtin( e, "-", builtin_sub );
  lenv_add_builtin( e, "*", builtin_mul );
  lenv_add_builtin( e, "/", builtin_div );
  lenv_add_builtin( e, "%", builtin_mod );
  lenv_add_builtin( e, "&", builtin_bwand );
  lenv_add_builtin( e, "|", builtin_bwor );
  lenv_add_builtin( e, "!", builtin_not );
  lenv_add_builtin( e, "~", builtin_neg );
  lenv_add_builtin( e, "^", builtin_xor );
  lenv_add_builtin( e, "**", builtin_pow );
  lenv_add_builtin( e, ">>", builtin_rshift );
  lenv_add_builtin( e, "<<", builtin_lshift );
  lenv_add_builtin( e, "min", builtin_min );
  lenv_add_builtin( e, "max", builtin_max );
  lenv_add_builtin( e, "env", builtin_env );
  lenv_add_builtin( e, "quit", builtin_quit );
  //lenv_add_builtin( e, "\\", builtin_lambda );
  lenv_add_builtin( e, "=", builtin_put );

}

//
// LVAL CONSTRUCTORS AND DESTRUCTORS
//

lval* lval_num( double x )  {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

lval* lval_err( char *fmt, ... )  {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_ERR;

  va_list va;
  va_start(va, fmt);

  v->err = malloc(512);

  vsnprintf( v->err, 511, fmt, va );

  v->err = realloc( v->err, strlen(v->err) + 1 );

  va_end(va);
  return v;
}

lval* lval_sym( char *s )  {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);
  return v;
}

lval* lval_sexpr( void )  {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

lval* lval_qexpr( void )  {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_QEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

lval* lval_fun( lbuiltin func )  {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_FUN;
  v->builtin = func;
  return v;
}

lval *lval_lambda( lval *formals, lval *body )  {
  lval *v = malloc( sizeof(lval) );
  v->type = LVAL_FUN;

  v->builtin = NULL;

  v->env = lenv_new();

  v->formals = formals;
  v->body = body;
  return v;
}

lval *lval_copy( lval *v )  {
  lval *x = malloc( sizeof(lval) );
  x->type = v->type;

  switch ( x->type )  {
    case LVAL_NUM:
      x->num = v->num;
    break;
    case LVAL_FUN:
      if ( !v->builtin )  {
        x->builtin = NULL;
        x->env = lenv_copy(v->env);
        x->formals = lval_copy(v->formals);
        x->body = lval_copy(v->body);
      } else {
        x->builtin = v->builtin;
      }
    break;
    case LVAL_ERR:
      x->err = malloc( strlen(v->err) + 1 );
      strcpy(x->err, v->err);
    break;
    case LVAL_SYM:
      x->sym = malloc( strlen(v->sym) + 1 );
      strcpy(x->sym, v->sym);
    break;
    case LVAL_QEXPR:
    case LVAL_SEXPR:
      x->count = v->count;
      x->cell = malloc( sizeof(lval*) * v->count );
      for ( size_t i = 0; i < x->count; i++ )  {
        x->cell[i] = lval_copy(v->cell[i]);
      }
    break;
  }

  return x;
}

void lval_del( lval *v )  {
  switch ( v->type )  {
    case LVAL_NUM: break;
    case LVAL_FUN:
      if ( !v->builtin )  {
        lenv_del(v->env);
        lval_del(v->formals);
        lval_del(v->body);
      }
    break;
    case LVAL_ERR: free(v->err); break;
    case LVAL_SYM: free(v->sym); break;

    case LVAL_QEXPR:
    case LVAL_SEXPR:
      for ( size_t i = 0; i < v->count; i++ )  {
        lval_del(v->cell[i]);
      }

      free(v->cell);
    break;
  }

  free(v);
}

lval *lval_call( lenv *e, lval *f, lval *a )  {
  if ( f->builtin )  { return f->builtin(e, a); }

  int given = a->count;
  int total = f->formals->count;

  while ( a->count )  {
    if ( f->formals->count == 0 )  {
      lval_del(a);
      return lval_err("Function passed too many arguements!\n"
      "Recieved %d, expected %d", given, total);
    }

    lval *sym = lval_pop(f->formals, 0);
    lval *val = lval_pop(a, 0);

    lenv_put(f->env, sym, val);

    lval_del(sym);
    lval_del(val);
  }

  lval_del(a);

  if ( f->formals->count == 0 )  {
    f->env->par = e;
    return builtin_eval(f->env, lval_add(lval_sexpr(), lval_copy(f->body)));
  } else {
    return lval_copy(f);
  }
}

char *ltype_name( int t )  {
  switch ( t )  {
    case LVAL_NUM: return "Number";
    case LVAL_SYM: return "Symbol";
    case LVAL_FUN: return "Function";
    case LVAL_QEXPR: return "Q-Expression";
    case LVAL_SEXPR: return "S-Expression";
    case LVAL_ERR: return "Error";
    default: return "Unknown";
  }
}

//
// EVALUATION
//

// any s expression get evaluated with below function
// anything else gets left as is
lval *lval_eval( lenv *e, lval *v )  {
  if ( v->type == LVAL_SYM )  {
    lval *x = lenv_get(e, v);
    lval_del(v);

    return x;
  }
  if ( v->type == LVAL_SEXPR )  { return lval_eval_sexpr(e, v); }
  return v;
}

// to evaluate an s expression ...
// evaluate each individual members of the s expression
// numbers and symbols stay as is, nested expressions get recursively evaluated
// check if anything became an error
// return empty expressions, take pulls the lval out of the s expression for unary
// assert the first element is a symbol
lval* lval_eval_sexpr( lenv *e, lval* v )  {

  for ( size_t i = 0; i < v->count; i++ )  {
    v->cell[i] = lval_eval(e, v->cell[i]);
  }

  for ( size_t i = 0; i < v->count; i++ )  {
    if ( (v->cell[i])->type == LVAL_ERR )  { return lval_take(v, i); }
  }

  // empty expression
  if ( v->count == 0 )  { return v; }

  // unary expression
  if ( v->count == 1 )  { return lval_take(v, 0); }

  // assert the first element is a symbol
  lval *f = lval_pop(v, 0);
  if ( f->type != LVAL_FUN )  {
    lval_del(f);
    lval_del(v);
    return lval_err("S-expression does not start with function!");
  }

  // do the calculation
  lval *result = lval_call(e, f, v);
  lval_del(f);
  return result;
}

lval *lval_add( lval *v, lval *x )  {
  v->count++;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  v->cell[v->count - 1] = x;
  return v;
}

lval *lval_pop( lval* v, int i )  {
  lval *x = v->cell[i];

  // move the data after element i forward by one slot
  memmove(&v->cell[i], &v->cell[i + 1],
    sizeof(lval*) * (v->count-i-1));

  v->count--;

  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  return x;
}

lval *lval_push( lval *v, lval *x )  {

  // give the pointer more space ( +1 to be exact )
  v->count++;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);

  // move all of the data forward by one slot
  memmove(&v->cell[1], &v->cell[0],
    sizeof(lval*) * (v->count - 1));

  // stick the new element in slot 0
  v->cell[0] = x;
  return v;
}

// pull an lval out of a s expression
// delete the old s expr and return the lval
lval *lval_take( lval *v, int i )  {
  lval *x = lval_pop(v, i);
  lval_del(v);
  return x;
}

lval *lval_join( lval *x, lval *y )  {
  while ( y->count )  {
    lval_add(x, lval_pop(y, 0));
  }

  lval_del(y);
  return x;
}

lval *builtin_op( lenv *e, lval *a, char *op )  {
  for ( size_t i = 0; i < a->count; i++ )  {
    if ( a->cell[i]->type != LVAL_NUM )  {
      lval_del(a);
      return lval_err("Cannot operate on non-number!");
    }
  }

  lval *x = lval_pop(a, 0);

  // unary methods
  if ( a->count == 0 )  {
    if ( strcmp(op, "-") == 0 ) { x->num = -x->num; }
    if ( strcmp(op, "!") == 0 ) { x->num = !x->num; }
    if ( strcmp(op, "~") == 0 ) { x->num = ~(long)x->num; }
  }

  while ( a->count > 0 )  {
    lval *y = lval_pop(a, 0);

    if ( strcmp(op, "+") == 0 ) { x->num += y->num; }
    if ( strcmp(op, "-") == 0 ) { x->num -= y->num; }
    if ( strcmp(op, "*") == 0 ) { x->num *= y->num; }
    if ( strcmp(op, "/") == 0 ) {
      if ( y->num == 0 )  {
        lval_del(y);
        lval_del(x);
        x = lval_err("Division by zero!");
        break;
      }
      x->num /= y->num;
    }
    if ( strcmp(op, "%") == 0 ) { x->num = (long)x->num % (long)y->num; }
    if ( strcmp(op, "|") == 0 ) { x->num = (long)x->num | (long)y->num; }
    if ( strcmp(op, "&") == 0 ) { x->num = (long)x->num & (long)y->num; }
    if ( strcmp(op, "^") == 0 ) { x->num = (long)x->num ^ (long)y->num; }
    if ( strcmp(op, ">>") == 0 ) { x->num = (long)x->num >> (long)y->num; }
    if ( strcmp(op, "<<") == 0 ) { x->num = (long)x->num << (long)y->num; }
    if ( strcmp(op, "**") == 0 ) { x->num = power(x->num, (long)y->num); }
    if ( strcmp(op, "max") == 0 ) { x->num = max(x->num, y->num); }
    if ( strcmp(op, "min") == 0 ) { x->num = min(x->num, y->num); }

    lval_del(y);
  }

  lval_del(a);
  return x;
}

//
// READING
//

lval* lval_read_num( mpc_ast_t *t )  {
  errno = 0;
  double x = strtod(t->contents, NULL);

  return errno != ERANGE ?
    lval_num(x) : lval_err("Invalid Number");
}

lval* lval_read( mpc_ast_t* t )  {
  if ( strstr(t->tag, "number") )  { return lval_read_num(t); }
  if ( strstr(t->tag, "symbol") )  { return lval_sym(t->contents); }

  lval *x = NULL;
  if ( strcmp(t->tag, ">") == 0 || strstr(t->tag, "sexpr") )  {
    x = lval_sexpr();
  }

  if ( strstr(t->tag, "qexpr") )  { x = lval_qexpr(); }

  for ( size_t i = 0; i < t->children_num; i++ )  {
    if ( strcmp(t->children[i]->contents, "(") == 0 )  { continue; }
    if ( strcmp(t->children[i]->contents, ")") == 0 )  { continue; }
    if ( strcmp(t->children[i]->contents, "{") == 0 )  { continue; }
    if ( strcmp(t->children[i]->contents, "}") == 0 )  { continue; }
    if ( strcmp(t->children[i]->tag,  "regex") == 0 )  { continue; }
    x = lval_add(x, lval_read(t->children[i]));
  }

  return x;
}

//
// PRINTING
//

void lval_expr_print( lval *v, char open, char close )  {
  putchar(open);
  for ( size_t i = 0; i < v->count; i++ )  {
    lval_print(v->cell[i]);
    if ( i != v->count - 1 )  {
      putchar(' ');
    }
  }
  putchar(close);
}

void lval_print( lval *v )  {
  switch ( v->type )  {
    case LVAL_NUM:
      printf( "%.2f", v->num );
    break;
    case LVAL_ERR:
      printf( "  🚩 :: Exception %s", v->err );
    break;
    case LVAL_SYM:
      printf( "%s", v->sym );
    break;
    case LVAL_FUN:
      if ( !v->builtin )  {
        printf("(\\ ");
        lval_print(v->formals);
        putchar(' ');
        lval_print(v->body);
        putchar(')');
      } else {
        printf( "<builtin>" );
      }
    break;
    case LVAL_SEXPR:
      lval_expr_print(v, '(', ')');
    break;
    case LVAL_QEXPR:
      lval_expr_print(v, '{', '}');
    break;
  }
}

void lval_println( lval *v ) {
  lval_print( v );
  putchar('\n');
}

//
// OPERATIONS
//

lval *builtin_add( lenv *e, lval *a )  {
  return builtin_op( e, a, "+" );
}

lval *builtin_sub( lenv *e, lval *a )  {
  return builtin_op( e, a, "-" );
}

lval *builtin_mul( lenv *e, lval *a )  {
  return builtin_op( e, a, "*" );
}

lval *builtin_div( lenv *e, lval *a )  {
  return builtin_op( e, a, "/" );
}

lval *builtin_mod( lenv *e, lval *a )  {
  return builtin_op( e, a, "%" );
}

lval *builtin_not( lenv *e, lval *a )  {
  return builtin_op( e, a, "!" );
}

lval *builtin_xor( lenv *e, lval *a )  {
  return builtin_op( e, a, "^" );
}

lval *builtin_bwand( lenv *e, lval *a )  {
  return builtin_op( e, a, "&" );
}

lval *builtin_bwor( lenv *e, lval *a )  {
  return builtin_op( e, a, "|" );
}

lval *builtin_neg( lenv *e, lval *a )  {
  return builtin_op( e, a, "~" );
}

lval *builtin_lshift( lenv *e, lval *a )  {
  return builtin_op( e, a, "<<" );
}

lval *builtin_rshift( lenv *e, lval *a )  {
  return builtin_op( e, a, ">>" );
}

lval *builtin_min( lenv *e, lval *a )  {
  return builtin_op( e, a, "min" );
}

lval *builtin_max( lenv *e, lval *a )  {
  return builtin_op( e, a, "max" );
}

lval *builtin_pow( lenv *e, lval *a )  {
  return builtin_op( e, a, "**" );
}

lval *builtin_quit( lenv *e, lval *a )  {
  lenv_del( e );
  printf("Quitting! 👋\n");
  exit(0);
}

lval *builtin_head( lenv *e, lval *a ) {
  LASSERT(a, a->count == 1,
    "Function 'head' passed too many arguments!\n"
    "\tRecieved %d, expected %d", a->count, 1);

  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
    "Function 'head' passed incorrect type!\n"
    "\tRecieved %s, expected %s",
    ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));

  LASSERT(a, a->cell[0]->count != 0,
    "Function 'head' passed {}!");

  lval *v = lval_take(a, 0);
  while ( v->count > 1 )  { lval_del(lval_pop(v, 1)); }
  return v;
}

lval *builtin_tail( lenv *e, lval *a ) {
  LASSERT(a, a->count == 1,
    "Function 'tail' passed too many arguments!\n"
    "\tRecieved %d, expected %d", a->count, 1);

  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
    "Function 'tail' passed incorrect type!\n"
    "\tRecieved %s, expected %s",
    ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));

  LASSERT(a, a->cell[0]->count != 0,
    "Function 'tail' passed {}! ");

  lval *v = lval_take(a, 0);
  lval_del(lval_pop(v, 0));
  return v;
}

lval *builtin_list( lenv *e, lval *a )  {
  a->type = LVAL_QEXPR;
  return a;
}

lval *builtin_eval( lenv *e, lval *a ) {
  LASSERT(a, a->count == 1,
    "Function 'eval' passed too many arguments!\n"
    "\tRecieved %d, expected %d", a->count, 1);

  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
    "Function 'eval' passed incorrect type!\n"
    "\tRecieved %s, expected %s",
    ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));

  lval *x = lval_take(a, 0);
  x->type = LVAL_SEXPR;
  return lval_eval(e, x);
}

lval *builtin_join( lenv *e, lval *a )  {

  for ( size_t i = 0; i < a->count; i++ )  {
    LASSERT(a, a->cell[i]->type == LVAL_QEXPR,
      "Function 'join' passed incorrect type!\n"
      "\tRecieved %s, expected %s",
      ltype_name(a->cell[i]->type), ltype_name(LVAL_QEXPR));
  }

  lval *x = lval_pop(a, 0);

  while ( a->count )  {
    x = lval_join(x, lval_pop(a, 0));
  }

  lval_del(a);
  return x;
}

lval *builtin_init( lenv *e, lval *a )  {
  LASSERT(a, a->count == 1,
    "Function 'init' passed too many arguments!\n"
    "\tRecieved %d, expected %d", a->count, 1);

  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
    "Function 'init' passed incorrect type!\n"
    "\tRecieved %s, expected %s",
    ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));

  LASSERT(a, a->cell[0]->count != 0,
    "Function 'init' passed {}! ");

  lval *x = lval_take(a, 0);
  lval *q = lval_qexpr();

  while (x->count > 1)  {
    q = lval_add(q, lval_pop(x, 0));
  }

  lval_del(x);
  return q;
}

lval *builtin_cons( lenv *e, lval *a )  {
  LASSERT(a, a->count == 2,
    "Function 'cons' passed too many arguments!\n"
    "\tRecieved %d, expected %d", a->count, 2);

  LASSERT(a, a->cell[1]->type == LVAL_QEXPR,
    "Function 'cons' passed incorrect type!\n"
    "\tRecieved %s, expected %s",
    ltype_name(a->cell[1]->type), ltype_name(LVAL_QEXPR));

  lval *x = lval_pop(a, 0);
  lval *y = lval_pop(a, 0);
  y = lval_push(y, x);

  lval_del(a);
  return y;
}

lval *builtin_len( lenv *e, lval *a )  {
  LASSERT(a, a->count == 1,
    "Function 'len' passed too many arguments!\n"
    "\tRecieved %d, expected %d", a->count, 1);

  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
    "Function 'len' passed incorrect type!\n"
    "\tRecieved %s, expected %s",
    ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));

  int len = a->cell[0]->count;
  lval_del(lval_take(a, 0));
  return lval_num(len);
}

lval *builtin_rev( lenv *e, lval *a )  {
  LASSERT(a, a->count == 1,
    "Function 'rev' passed too many arguments!\n"
    "\tRecieved %d, expected %d", a->count, 1);

  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
    "Function 'rev' passed incorrect type!\n"
    "\tRecieved %s, expected %s",
    ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));

  lval *x = lval_take(a, 0);
  lval *q = lval_qexpr();

  while (x->count > 0)  {
    q = lval_push(q, lval_pop(x, 0));
  }

  lval_del(x);
  return q;
}

lval *builtin_if( lenv *e, lval *arguements )  {
  LASSERT(arguements, arguements->count == 2,
    "Function '?' passed too many arguments!\n"
    "\tRecieved %d, expected %d", arguements->count, 2);

  LASSERT(arguements, (arguements->cell[0]->type != LVAL_QEXPR
    && arguements->cell[0]->type != LVAL_SYM),
    "Function '?' passed incorrect type!\n"
    "\tRecieved %s, expected %s or ",
    ltype_name(arguements->cell[0]->type), ltype_name(LVAL_QEXPR), ltype_name(LVAL_SYM));

  for ( size_t i = 1; i < arguements->count; i++ )  {
    LASSERT(arguements, arguements->cell[i]->type == LVAL_QEXPR,
      "Function '?' passed incorrect type!\n"
      "\tRecieved %s, expected %s",
      ltype_name(arguements->cell[i]->type), ltype_name(LVAL_QEXPR));
  }

  lval *clause = lval_pop(arguements, 0);
  lval *then = lval_pop(arguements, 0);
  lval *_else = lval_pop(arguements, 0);
  lval_del(arguements);

  lval *result = lval_eval(e, clause);
  if ( result->num )  {
    lval_del(_else);
    lval_del(result);
    return then;
  } else {
    lval_del(then);
    lval_del(result);
    return _else;
  }
}

lval *builtin_bool( lenv *e, lval *arguements )  {
  LASSERT(arguements, arguements->count == 1,
    "Function 'bool' passed too many arguments!\n"
    "\tRecieved %d, expected %d", arguements->count, 1);

  LASSERT(arguements, arguements->cell[0]->type == LVAL_SYM,
    "Function 'bool' passed incorrect type!\n"
    "\tRecieved %s, expected %s",
    ltype_name(arguements->cell[0]->type), ltype_name(LVAL_SYM));

  lval *clause = lval_take(arguements, 0);
  lval *result = lval_eval(e, clause);

  if ( result->type == LVAL_NUM )  {
    return result;
  } else if ( result->type == LVAL_QEXPR )  {
    // turn the qexpr into an argument
    // ( {qexpr} )
    lval *arguement = lval_add(lval_sexpr(), result);
    return builtin_len(e, arguement);
  }
  lval_del(result);
  return lval_num(0);
}

lval *builtin_var( lenv *e, lval *a, char *func )  {
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
    "Function '%s' passed incorrect type!\n"
    "\tRecieved %s, expected %s",
    func, ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));

  lval *syms = a->cell[0];

  for ( size_t i = 0; i < syms->count; i++ )  {
    LASSERT(a, syms->cell[i]->type == LVAL_SYM,
      "Function '%s' passed incorrect type!\n"
      "\tRecieved %s, expected %s",
      func, ltype_name(syms->cell[i]->type), ltype_name(LVAL_SYM));
  }

  LASSERT(a, (syms->count == a->count - 1),
    "Function '%s' passed incorrect number of arguments for symbols!"
    "Recieved %d, expected %d",
    func, syms->count, a->count - 1);

  for ( size_t i = 0; i < syms->count; i++ )  {
    if ( strcmp(func, "def") == 0 )  {
      lenv_def(e, syms->cell[i], a->cell[i + 1]);
    }
    if ( strcmp(func, "=") == 0 )  {
      lenv_put(e, syms->cell[i], a->cell[i + 1]);
    }
  }

  lval_del(a);
  return lval_sexpr();
}

lval *builtin_def( lenv *e, lval *a )  {
  return builtin_var(e, a, "def");
}

lval *builtin_put( lenv *e, lval *a )  {
  return builtin_var(e, a, "=");
}

lval *builtin_env( lenv *e, lval *a )  {
  lval *exp = lval_qexpr();

  for ( size_t i = 0; i < e->count; i++ )  {
    lval *q = lval_qexpr();
    //
    q = lval_add( q, lval_sym(e->syms[i]) );
    q = lval_add( q, lval_copy(e->vals[i]) );
    exp = lval_add(exp, q);
  }

  return exp;
}

lval *builtin_lambda( lenv *e, lval *a )  {
  LASSERT(a, a->count == 2,
    "Function '\\' passed too many arguments!\n"
    "\tRecieved %d, expected %d", a->count, 2);

  for ( size_t i = 0; i < 2; i++)  {
    LASSERT(a, a->cell[i]->type == LVAL_QEXPR,
      "Function '\\' passed incorrect type!\n"
      "\tRecieved %s, expected %s",
      ltype_name(a->cell[i]->type), ltype_name(LVAL_QEXPR));
  }

  for ( size_t i = 0; i < a->cell[0]->count; i++)  {
    LASSERT(a, a->cell[0]->cell[i]->type == LVAL_SYM,
      "Cannot define non-symbol!\n"
      "\tRecieved %s, expected %s",
      ltype_name(a->cell[0]->cell[i]->type), ltype_name(LVAL_SYM));
  }

  lval *formals = lval_pop(a, 0);
  lval *body = lval_pop(a, 0);
  lval_del(a);

  return lval_lambda(formals, body);
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
