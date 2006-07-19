/**
 * The [matches] class in Pure Data is an external to verify that a subject 
 * matches a Perl-compatible regular expression mask.
 * 
 * http://alexandre.quessy.net
 * 
 * @author Alexandre Quessy
 * @c GNU General Public License 2006
 */
 
#include "m_pd.h"
#include <string.h>
#include <stdlib.h>
#include <pcre.h>

#define CAPTUREVECTORSIZE 30 /* multiple of 3 */
#define MATCHES_ERROR_PREFIX "[matches]: "
#define MATCHES_FALSE_MASK "xxxxxxxxxxxxxxxxxxxxxx"
#define MATCHES_DECIMAL_PRECISION_SIZE_T 4

/** variables of the pd object */
typedef struct matches {
  t_object x_ob; /* contient inlets et outlets */
  t_outlet *x_outlet;
  pcre *regex; /* mask */
} t_matches;

/** functions */
char *_str_replace(const char search, const char replace, const char *subject);
int _isMatching(t_matches *x, char *text);
void _compile_pattern(t_matches *x, const char *pattern);


/** left inlet: subject */
void matches_symbol0(t_matches *x, t_symbol *arg) {
  int isOk =_isMatching(x, arg->s_name);
  outlet_float(x->x_outlet, isOk);
}

/* left inlet: subject (when a float) 
void matches_float0(t_matches *x, t_floatarg arg) {
  char *str_float = sprintf("%f0", arg);
  int isOk =_isMatching(x, str_float);
  outlet_float(x->x_outlet, isOk);
}
*/


/** right inlet : mask */
void matches_symbol1(t_matches *x, t_symbol *arg) {
  _compile_pattern(x, arg->s_name);
}

/* right inlet 
void matches_float1(t_matches *x, t_floatarg arg) {
  char *str_float = sprintf("%f0", arg);
  _compile_pattern(x, str_float);
}
*/
t_class *matches_class;

/** constructor */
void *matches_new(t_symbol *selector, int argc, t_atom *argv) {
  int is_valid = 0;
  t_matches *x = (t_matches *) pd_new(matches_class);
  
  /* set the mask */
  if (argc < 1) {
    post("%s No mask given as argument. Please supply one as message.\n", MATCHES_ERROR_PREFIX);
  } else {
    /* if (argv[0].a_type == A_SYMBOL) { */
      t_symbol *tmp = atom_getsymbol(&argv[0]);
      _compile_pattern(x, tmp->s_name);
      is_valid = 1;
      /*
    } else {
      post("%s Argument should be a symbol\n", MATCHES_ERROR_PREFIX);
    }
    */
  }
  if (is_valid == 0) {
    _compile_pattern(x, MATCHES_FALSE_MASK);
  }
  /* add inlets */
  inlet_new(&x->x_ob, &x->x_ob.ob_pd, gensym("symbol"), gensym("mask")); /* selecteur, renomme mask */
  
  /* add outlet */
  x->x_outlet = outlet_new(&x->x_ob, gensym("float"));
  return (void *)x;
}

/** setup */
void matches_setup(void) {
  matches_class = class_new(gensym("matches"), (t_newmethod) matches_new, 0, sizeof(t_matches), 0, A_GIMME, 0);
  class_addmethod(matches_class, (t_method) matches_symbol0, gensym("symbol"), A_SYMBOL, 0);
  class_addmethod(matches_class, (t_method) matches_symbol1, gensym("mask"), A_SYMBOL, 0);
  /* 
  class_addmethod(matches_class, (t_method) matches_float0, gensym("float"), A_FLOAT, 0);
  class_addmethod(matches_class, (t_method) matches_float1, gensym("float"), A_FLOAT, 0);
  */
}



/*

matju aalex: pour les inlets acceptant plusieurs s�lecteurs, check gridflow/bridge/puredata.c
matju aalex: �a fait: inlet_new(self->bself, &p->ob_pd, 0,0);
matju aalex: avec un proxy inlet
aalex ???? MiS : http://www.tigerdirect.ca/applications/SearchTools/item-details.asp?EdpNo=1068181&CatId=107
aalex ah
aalex matju  : il me faut absolument un proxy-inlet, donc
matju aalex: dans cet exemple, self->bself est d'un soustype de t_object* et &p->ob_pd est un t_pd*
matju aalex: je crois que oui
matju aalex: &p->ob_pd est en fait la m�me chose que (t_pd *)p, si je ne m'abuse

*/



/* ############################### functions ###################### */




/** clone of the PHP function */
char *_str_replace(const char search, const char replace, const char *subject) {
  int i, len;
  char *result = strdup(subject);
  len = strlen(result);
  for (i = 0; i <= len; i++) {
    if(result[i] == search) { 
      result[i] = replace;
    }
  }
  return result;
}

/** 
 * returns 1 if matches, 0 if not 
 * TODO use stdbool.h, I think
 */
int _isMatching(t_matches *x, char *text) { /* 2nd is const */
  int capturevector[CAPTUREVECTORSIZE];
  int rc;
  int i;
  int result = 0;
  
  rc = pcre_exec(x->regex, NULL, text, (int) strlen(text), 0, 0, capturevector, CAPTUREVECTORSIZE);
  
  if (rc < 0) {
    result = 0;
  } else {
    result = 1;
  }
  
  /* ok, a partir d'ici c'est different de l'autre exemple, sauf qu'on avait mis des parentheses dans le pattern */
  if (rc == 0) {
    rc = CAPTUREVECTORSIZE / 3;
    post("ovector only has room for %d captured substrings\n", rc - 1);
  }
  for (i = 0; i < rc; i++) {
    char *substring_start = text + capturevector[2 * i]; /* ovector */
    int substring_length = capturevector[2 * i + 1] - capturevector[2 * i];
    post("%2d matching: %.*s", i, substring_length, substring_start);
  }
  /* I think that from here I should clean up my memory */
  
  return result;
}

/* ############## begin class matches ####################### */

/** 
 * private method to set and compile the mask 
 * TODO : use pcre_malloc() and pcre_free()
 */
void _compile_pattern(t_matches *x, const char *pattern) {
  pcre *regex;
  const char *regex_error; 
  int erroroffset;
  char *mask = _str_replace('`', '\\', pattern);
  
  regex = pcre_compile(mask, 0, &regex_error, &erroroffset, NULL);
  if (regex == NULL) {
    post("%s Compilation failed at offset %d : %s", MATCHES_ERROR_PREFIX, erroroffset, regex_error);
    regex = pcre_compile(MATCHES_FALSE_MASK, 0, &regex_error, &erroroffset, NULL); /* will always return false if invalid pattern */
  } else {
    post("%s New PCRE mask: %s", MATCHES_ERROR_PREFIX, mask);
  }
  /* free(x->regex); */
  x->regex = regex;
}
