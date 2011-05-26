#include "entity.h"


void cnSampleInit(cnSample* sample) {
  sample->label = cnFalse;
  cnListInit(&sample->entities, sizeof(void*));
}
