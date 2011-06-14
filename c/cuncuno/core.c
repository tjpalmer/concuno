#include <math.h>
#include <stdio.h>
#include <string.h>
#include "core.h"


cnBool cnIsNaN(cnFloat x) {
  // TODO _isnan for Windows?
  #ifdef isnan
    return isnan(x);
  #else
    return x != x;
  #endif
}


void cnListClear(cnListAny* list) {
  list->count = 0;
}


void cnListDispose(cnListAny* list) {
  free(list->items);
  cnListInit(list, list->itemSize);
}


void* cnListGet(cnListAny* list, cnIndex index) {
  // TODO Support negative indexes from back?
  if (index < 0 || index >= list->count) {
    return NULL;
  }
  return ((char*)list->items) + (index * list->itemSize);
}


void* cnListEnd(const cnListAny* list) {
  return ((char*)list->items) + (list->count * list->itemSize);
}


void* cnListExpand(cnListAny* list) {
  return cnListExpandMulti(list, 1);
}


void* cnListExpandMulti(cnListAny* list, cnCount count) {
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


void cnListInit(cnListAny* list, cnCount itemSize) {
  list->count = 0;
  list->itemSize = itemSize;
  list->items = NULL;
  list->reservedCount = 0;
}


void* cnListPush(cnListAny* list, void* item) {
  return cnListPushMulti(list, item, 1);
}


void* cnListPushAll(cnListAny* list, cnListAny* from) {
  if (list->itemSize != from->itemSize) {
    printf(
      "list itemSize %ld != from itemSize %ld\n",
      list->itemSize, from->itemSize
    );
    return NULL;
  }
  return cnListPushMulti(list, from->items, from->count);
}


void* cnListPushMulti(cnListAny* list, void* items, cnCount count) {
  void* formerEnd = cnListExpandMulti(list, count);
  if (formerEnd) {
    memcpy(formerEnd, items, list->itemSize * count);
  }
  return formerEnd;
}


void cnListRemove(cnListAny* list, cnIndex index) {
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


void cnListShuffle(cnListAny* list) {
  // I'd rather just do simple copies than fancy n-byte xors.
  void* buffer = malloc(list->itemSize);
  // Fisher-Yates shuffling from:
  // http://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle#
  // The_modern_algorithm
  cnIndex i;
  for (i = list->count; i >= 1; i--) {
    // Find where we're going.
    cnIndex j = rand() % (i + 1);
    void* a = ((char*)list->items) + (i * list->itemSize);
    void* b = ((char*)list->items) + (j * list->itemSize);
    // Perform the swap.
    memcpy(buffer, a, list->itemSize);
    memcpy(a, b, list->itemSize);
    memcpy(b, buffer, list->itemSize);
  }
  // All done.
  free(buffer);
}


cnFloat cnNaN(void) {
  // TODO Something more efficient but still portable?
  static cnFloat nan = -1;
  if (nan < 0) {
    nan = strtod("NAN", NULL);
  }
  return nan;
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
  formerEnd = cnListExpandMulti(string, extra);
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
