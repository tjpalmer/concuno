#ifndef cuncuno_entity_h
#define cuncuno_entity_h


#include "core.h"


typedef struct cnBag {

  // TODO id?

  /**
   * List of void pointers here.
   */
  cnList entities;

  /**
   * Positive or negative bag.
   */
  cnBool label;

} cnBag;


void cnBagDispose(cnBag* bag);


void cnBagInit(cnBag* bag);


#endif
