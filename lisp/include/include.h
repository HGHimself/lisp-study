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
  int none;
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
