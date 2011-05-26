#ifndef cuncuno_entity_h
#define cuncuno_entity_h


#include "core.h"


typedef struct cnSample {

  /**
   * List of void pointers here.
   */
  cnList entities;

  /**
   * Positive or negative sample.
   */
  cnBool label;

} cnSample;


void cnSampleInit(cnSample* sample);


#endif
