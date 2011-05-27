#ifndef cuncuno_entity_h
#define cuncuno_entity_h


#include "core.h"


typedef struct cnSample {

  // TODO id?

  /**
   * List of void pointers here.
   */
  cnList entities;

  /**
   * Positive or negative sample.
   */
  cnBool label;

} cnSample;


void cnSampleDispose(cnSample* sample);


void cnSampleInit(cnSample* sample);


#endif
