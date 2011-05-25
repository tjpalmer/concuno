#include <stdio.h>
#include <string.h>
#include "core.h"


void cnListClear(cnList* list) {
  list->count = 0;
}


void cnListDispose(cnList* list) {
  free(list->items);
  cnListInit(list, list->itemSize);
}


void* cnListGet(cnList* list, cnIndex index) {
  // TODO Support negative indexes from back?
  if (index < 0 || index >= list->count) {
    return NULL;
  }
  return ((char*)list->items) + (index * list->itemSize);
}


void* cnListEnd(cnList* list) {
  return ((char*)list->items) + (list->count * list->itemSize);
}


void cnListInit(cnList* list, cnCount itemSize) {
  list->count = 0;
  list->itemSize = itemSize;
  list->items = NULL;
  list->reservedCount = 0;
}


cnBool cnListPush(cnList* list, void* item) {
  cnCount needed = list->count + 1;
  if (needed > list->reservedCount) {
    // Exponential growth to avoid slow pushing.
    cnCount wanted = 2 * list->reservedCount;
    if (!wanted) wanted = 1;
    void* newItems = realloc(list->items, wanted * list->itemSize);
    if (!newItems) {
      // No memory for this.
      return cnFalse;
    }
    // TODO Clear extra allocated memory?
    list->items = newItems;
    list->reservedCount = wanted;
  }
  list->count = needed;
  memcpy(cnListGet(list, needed - 1), item, list->itemSize);
  return cnTrue;
}


void cnListRemove(cnList* list, cnIndex index) {
  char *begin = cnListGet(list, index);
  if (!begin) {
    printf("Bad index for remove: %ld\n", index);
    return;
  }
  list->count--;
  if (index == list->count) {
    return;
  }
  memmove(
    begin, begin + list->itemSize, (list->count - index) * list->itemSize
  );
}


char* cnStr(cnString* string) {
  return string->items ? (char*)string->items : (char*)"";
}


void cnStringClear(cnString* string) {
  if (string->items) {
    string->count = 1;
    *cnStr(string) = '\0';
  }
}


void cnStringDispose(cnString* string) {
  cnListDispose(string);
}


char cnStringGetChar(cnString* string, cnIndex index) {
  return *(char*)cnListGet(string, index);
}


void cnStringInit(cnString* string) {
  cnListInit(string, sizeof(char));
}


cnBool cnStringPushChar(cnString* string, char c) {
  char* str;
  // Push a char, even though we'll soon swap it.
  // This gives us at least the right space.
  // TODO Just a call to reserve for these one or two chars?
  if (!string->reservedCount) {
    // Extra allocation because we need two chars for the first.
    if (!cnListPush(string, &c)) {
      return cnFalse;
    }
  }
  if (!cnListPush(string, &c)) {
    // TODO Anything to do with null chars for failure?
    return cnFalse;
  }
  // Now swap the null char and the final.
  str = string->items;
  str[string->count - 2] = c;
  str[string->count - 1] = '\0';
  return cnTrue;
}
