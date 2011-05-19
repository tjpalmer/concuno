#include <string.h>
#include "core.h"


void cnListClean(cnList* list) {
  if (list->reservedCount) {
    free(list->items);
  }
  cnListInit(list, list->itemSize);
}


void* cnListGet(cnList* list, cnIndex index) {
  // TODO Support negative indexes from back?
  return ((char*)list->items) + (index * list->itemSize);
}


void cnListInit(cnList* list, cnCount itemSize) {
  list->count = 0;
  list->itemSize = 0;
  list->items = NULL;
  list->reservedCount = 0;
}


cnBool cnListPush(cnList* list, void* item) {
  cnCount needed = list->count + 1;
  if (needed > list->reservedCount) {
    // Exponential growth to avoid slow pushing.
    needed = 2 * list->reservedCount;
    void* newItems = realloc(list->items, needed);
    if (!newItems) {
      // No memory for this.
      return cnFalse;
    }
    // TODO Clear extra allocated memory?
    list->items = newItems;
    list->reservedCount = needed;
  }
  memcpy(cnListGet(list, list->count), item, list->itemSize);
  return cnTrue;
}


void cnStringClean(cnString* string) {
  cnListClean(string);
  // TODO Avoid extra cnListInit call?
  cnStringInit(string);
}


void cnStringInit(cnString* string) {
  cnListInit(string, sizeof(char));
  string->items = "";
}


cnBool cnStringPushChar(cnString* string, char c) {
  char* str;
  // Push a char, even though we'll soon swap it.
  // This gives us at least the right space.
  // TODO Just a call to reserve for these one or two chars?
  if (!string->reservedCount) {
    // Extra allocation because we need two chars for the first.
    cnListPush(string, &c);
  }
  cnListPush(string, &c);
  // Now swap the null char and the final.
  str = string->items;
  str[string->count - 2] = c;
  str[string->count - 1] = '\0';
}


char* cnStringStr(cnString* string) {
  return (char*)string->items;
}
