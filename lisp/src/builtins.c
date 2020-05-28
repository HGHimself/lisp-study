#include <stdio.h>
#include "include.h"

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
