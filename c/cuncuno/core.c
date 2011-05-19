#include "core.h"


void cnListClean(cnList* list) {
}


void cnListInit(cnList* list, cnCount itemSize) {
  list->count = 0;
  list->itemSize = 0;
  list->items = NULL;
  list->reservedCount = 0;
}


cnBool cnListPush(cnList* list, void* items) {
}
