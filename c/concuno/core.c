#include <math.h>
#include <string.h>
#include "core.h"


/**
 * Bubbles the item at the given index down the heap.
 */
void cnHeapDown(cnHeapAny heap, cnIndex index);


/**
 * Provides the kids of the given parent index, using -1 to mean no such kid.
 */
void cnHeapKids(
  cnHeapAny heap, cnIndex parentIndex,
  cnRef(cnIndex) kid1Index, cnRef(cnIndex) kid2Index
);


/**
 * Provides the parent of the given kid index, providing -1 for no parent (in
 * the case of the root at index 0).
 */
cnIndex cnHeapParent(cnHeapAny heap, cnIndex kidIndex);


/**
 * Bubbles the item at the given index up the heap.
 */
void cnHeapUp(cnHeapAny heap, cnIndex index);


void cnHeapClear(cnHeapAny heap) {
  if (heap->destroyItem) {
    cnListEachBegin(&heap->items, cnRefAny, item) {
      heap->destroyItem(heap->info, *item);
    } cnEnd;
  }
  cnListClear(&heap->items);
}


cnHeapAny cnHeapCreate(cnBool (*less)(cnRefAny info, cnRefAny a, cnRefAny b)) {
  cnHeapAny heap = malloc(sizeof(struct cnHeapAny));
  if (!heap) cnFailTo(DONE, "No heap.");
  heap->destroyInfo = NULL;
  heap->destroyItem = NULL;
  heap->info = NULL;
  cnListInit(&heap->items, sizeof(cnRefAny));
  heap->less = less;
  DONE:
  return heap;
}


void cnHeapDestroy(cnHeapAny heap) {
  // No op on null.
  if (!heap) return;

  // Items.
  cnHeapClear(heap);
  cnListDispose(&heap->items);

  // Info.
  if (heap->destroyInfo) heap->destroyInfo(heap->info);

  // Heap.
  // TODO Null everything out for extra safety?
  free(heap);
}


void cnHeapDown(cnHeapAny heap, cnIndex index) {
  while (index >= 0) {
    cnRef(cnRefAny) item = ((cnArr(cnRefAny))heap->items.items) + index;
    cnIndex kidIndex;
    cnRef(cnRefAny) kidItem;
    cnIndex kid2Index;

    // Find the kids, and see if they exist.
    cnHeapKids(heap, index, &kidIndex, &kid2Index);
    if (kidIndex >= 0) {
      // Presume swapping with kid 1 for now.
      kidItem = ((cnArr(cnRefAny)) heap->items.items) + kidIndex;
    } else {
      // We're at the bottom.
      break;
    }
    if (kid2Index >= 0) {
      cnRef(cnRefAny) kid2Item =
        ((cnArr(cnRefAny))heap->items.items) + kid2Index;
      // TODO Error checking on less? It hypothetically could fail ...
      if (heap->less(heap, *kid2Item, *kidItem)) {
        // Actually, kid 2 is smaller, so use it.
        kidIndex = kid2Index;
        kidItem = kid2Item;
      }
    }

    // Check position.
    // TODO Error checking on less? It hypothetically could fail ...
    if (heap->less(heap, *kidItem, *item)) {
      // Swap with the lesser kid.
      cnRefAny tempItem;
      tempItem = *item;
      *item = *kidItem;
      *kidItem = tempItem;
      // And move down the tree.
      index = kidIndex;
    } else {
      // We don't need to go any further.
      break;
    }
  }
}


void cnHeapKids(
  cnHeapAny heap, cnIndex parentIndex,
  cnRef(cnIndex) kid1Index, cnRef(cnIndex) kid2Index
) {
  // TODO Use a "B-Heap" or Emde Boas layout or some such for more efficient
  // TODO memory access.
  // The following is repetitive, but simple enough.
  if (kid1Index) {
    *kid1Index = 2 * parentIndex + 1;
    if (*kid1Index >= heap->items.count) *kid1Index = -1;
  }
  if (kid2Index) {
    *kid2Index = 2 * parentIndex + 2;
    if (*kid2Index >= heap->items.count) *kid2Index = -1;
  }
}


cnIndex cnHeapParent(cnHeapAny heap, cnIndex kidIndex) {
  // TODO Use a "B-Heap" or Emde Boas layout or some such for more efficient
  // TODO memory access.
  return kidIndex ? (kidIndex - 1) / 2 : -1;
}


cnRefAny cnHeapPeek(cnHeapAny heap) {
  // Dereference to the stored pointer, if we have anything.
  return heap->items.items ? *(cnRef(cnRefAny))heap->items.items : NULL;
}


cnRefAny cnHeapPull(cnHeapAny heap) {
  cnRefAny pulled = cnHeapPeek(heap);
  if (heap->items.count) {
    // TODO With a different memory layout, is the last item still always legit?
    // Swap the bottom item to the top.
    // We always store pointers here, so hack the array type.
    // For the last item, this is inefficient, but that's not a common use case.
    ((cnArr(cnRefAny))heap->items.items)[0] =
      ((cnArr(cnRefAny))heap->items.items)[heap->items.count - 1];
    ((cnArr(cnRefAny))heap->items.items)[heap->items.count - 1] = NULL;
    // We always remove the last, so this is easy.
    heap->items.count--;
    cnHeapDown(heap, 0);
  } else {
    // TODO Error or okay to pull from an empty heap?
  }
  return pulled;
}


cnBool cnHeapPush(cnHeapAny heap, cnRefAny item) {
  cnBool result = cnFalse;
  if (!cnListPush(&heap->items, &item)) cnFailTo(DONE, "No push.");
  cnHeapUp(heap, heap->items.count - 1);
  result = cnTrue;
  DONE:
  return result;
}


void cnHeapUp(cnHeapAny heap, cnIndex index) {
  while (index) {
    cnRef(cnRefAny) item = ((cnArr(cnRefAny))heap->items.items) + index;
    cnIndex parentIndex = cnHeapParent(heap, index);
    cnRef(cnRefAny) parentItem =
      ((cnArr(cnRefAny))heap->items.items) + parentIndex;
    // TODO Error checking on less? It hypothetically could fail ...
    if (heap->less(heap->info, *item, *parentItem)) {
      // Swap parent and kid.
      cnRefAny temp = *parentItem;
      *parentItem = *item;
      *item = temp;
      // And keep moving up the tree.
      index = parentIndex;
    } else {
      // All good now.
      break;
    }
  }
}


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


void cnListDestroy(cnListAny* list) {
  cnListDispose(list);
  free(list);
}


void cnListDispose(cnListAny* list) {
  if (!list) return;
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


void* cnListGetPointer(cnListAny* list, cnIndex index) {
  return *(void**)cnListGet(list, index);
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

  // If you add nothing to an empty list, it's successful, but there's no space
  // to use, so don't pretend there's any.
  if (!count) return NULL;

  // Handle the common cases.
  if (needed > list->reservedCount) {
    void* newItems;
    // Exponential growth to avoid slow pushing.
    cnCount wanted = 2 * list->reservedCount;
    if (wanted < needed) {
      // If pushing several, could go past 2 * current.
      wanted = 2 * needed; // or just needed?
    }
    if (!wanted) wanted = 1;
    newItems = realloc(list->items, wanted * list->itemSize);
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
  for (i = list->count - 1; i >= 1; i--) {
    // Find where we're going.
    void* a;
    void* b;
    cnIndex j = rand() % (i + 1);
    if (i == j) continue;
    a = ((char*)list->items) + (i * list->itemSize);
    b = ((char*)list->items) + (j * list->itemSize);
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
