#include <stdio.h>
#include "include.h"

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
  lenv_add_builtin( e, "\\", builtin_lambda );
  lenv_add_builtin( e, "=", builtin_put );
}
