#include "entity.h"


void cnSampleDispose(cnSample* sample) {
  cnListDispose(&sample->entities);
  cnSampleInit(sample);
}


void cnSampleInit(cnSample* sample) {
  sample->label = cnFalse;
  cnListInit(&sample->entities, sizeof(void*));
}
