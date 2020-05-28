#include <stdio.h>
#include "include.h"

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
