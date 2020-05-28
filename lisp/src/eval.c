#include <stdio.h>
#include "include.h"


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
