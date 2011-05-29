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


void* cnListEnd(const cnList* list) {
  return ((char*)list->items) + (list->count * list->itemSize);
}


void* cnListExpand(cnList* list, cnCount count) {
  void* formerEnd;
  cnCount needed = list->count + count;
  if (needed > list->reservedCount) {
    // Exponential growth to avoid slow pushing.
    cnCount wanted = 2 * list->reservedCount;
    if (wanted < needed) {
      // If pushing several, could go past 2 * current.
      wanted = 2 * needed; // or just needed?
    }
    if (!wanted) wanted = 1;
    void* newItems = realloc(list->items, wanted * list->itemSize);
    if (!newItems) {
      // No memory for this.
      printf("Failed to expand list.");
      return NULL;
    }
    // TODO Clear extra allocated memory?
    list->items = newItems;
    list->reservedCount = wanted;
  }
  // Get the end after possible reallocation and before changing count.
  formerEnd = cnListEnd(list);
  list->count = needed;
  return formerEnd;
}


void cnListInit(cnList* list, cnCount itemSize) {
  list->count = 0;
  list->itemSize = itemSize;
  list->items = NULL;
  list->reservedCount = 0;
}


void* cnListPush(cnList* list, void* item) {
  return cnListPushMulti(list, item, 1);
}


void* cnListPushAll(cnList* list, cnList* from) {
  if (list->itemSize != from->itemSize) {
    printf(
      "list itemSize %ld != from itemSize %ld\n",
      list->itemSize, from->itemSize
    );
    return NULL;
  }
  return cnListPushMulti(list, from->items, from->count);
}


void* cnListPushMulti(cnList* list, void* items, cnCount count) {
  void* formerEnd = cnListExpand(list, count);
  if (formerEnd) {
    memcpy(formerEnd, items, list->itemSize * count);
  }
  return formerEnd;
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


cnBool cnStringPushStr(cnString* string, char* str) {
  char* formerEnd;
  cnBool wasEmpty = !string->reservedCount;
  cnCount extra = strlen(str);
  if (wasEmpty) {
    // For the null char.
    extra++;
  }
  // Make space.
  formerEnd = cnListExpand(string, extra);
  if (!formerEnd) {
    return cnFalse;
  }
  if (!wasEmpty) {
    // Overwrite the old null char.
    formerEnd--;
  }
  // Copy including the null char, and we're done.
  strcpy(formerEnd, str);
  return cnTrue;
}
