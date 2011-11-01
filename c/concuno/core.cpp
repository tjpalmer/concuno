#include <limits>
#include <math.h>
#include <string.h>
#include "core.h"

using namespace std;


namespace concuno {


Error::Error(const std::string& what): runtime_error(what) {}


/**
 * Bubbles the item at the given index down the heap.
 */
void cnHeapDown(cnHeapAny* heap, Index index);


/**
 * Provides the kids of the given parent index, using -1 to mean no such kid.
 */
void cnHeapKids(
  cnHeapAny* heap, Index parentIndex,
  cnRef(Index) kid1Index, cnRef(Index) kid2Index
);


/**
 * Provides the parent of the given kid index, providing -1 for no parent (in
 * the case of the root at index 0).
 */
Index cnHeapParent(cnHeapAny heap, Index kidIndex);


/**
 * Bubbles the item at the given index up the heap.
 */
void cnHeapUp(cnHeapAny* heap, Index index);


void cnHeapClear(cnHeapAny* heap) {
  if (heap->destroyItem) {
    cnListEachBegin(&heap->items, cnRefAny, item) {
      heap->destroyItem(heap->info, *item);
    } cnEnd;
  }
  cnListClear(&heap->items);
}


cnHeapAny* cnHeapCreate(bool (*less)(cnRefAny info, cnRefAny a, cnRefAny b)) {
  cnHeapAny* heap = cnAlloc(cnHeapAny, 1);
  if (!heap) cnErrTo(DONE, "No heap.");
  heap->destroyInfo = NULL;
  heap->destroyItem = NULL;
  heap->info = NULL;
  heap->items.init(sizeof(cnRefAny));
  heap->less = less;
  DONE:
  return heap;
}


void cnHeapDestroy(cnHeapAny* heap) {
  // No op on null.
  if (!heap) return;

  // Items.
  cnHeapClear(heap);

  // Info.
  if (heap->destroyInfo) heap->destroyInfo(heap->info);

  // Heap.
  // TODO Null everything out for extra safety?
  free(heap);
}


void cnHeapDown(cnHeapAny* heap, Index index) {
  while (index >= 0) {
    cnRef(cnRefAny) item = ((cnArr(cnRefAny))heap->items.items) + index;
    Index kidIndex;
    cnRef(cnRefAny) kidItem;
    Index kid2Index;

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
  cnHeapAny* heap, Index parentIndex,
  cnRef(Index) kid1Index, cnRef(Index) kid2Index
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


Index cnHeapParent(cnHeapAny* heap, Index kidIndex) {
  // TODO Use a "B-Heap" or Emde Boas layout or some such for more efficient
  // TODO memory access.
  return kidIndex ? (kidIndex - 1) / 2 : -1;
}


cnRefAny cnHeapPeek(cnHeapAny* heap) {
  // Dereference to the stored pointer, if we have anything.
  return heap->items.items ? *(cnRef(cnRefAny))heap->items.items : NULL;
}


cnRefAny cnHeapPull(cnHeapAny* heap) {
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


bool cnHeapPush(cnHeapAny* heap, cnRefAny item) {
  bool result = false;
  if (!cnListPush(&heap->items, &item)) cnErrTo(DONE, "No push.");
  cnHeapUp(heap, heap->items.count - 1);
  result = true;
  DONE:
  return result;
}


void cnHeapUp(cnHeapAny* heap, Index index) {
  while (index) {
    cnRef(cnRefAny) item = ((cnArr(cnRefAny))heap->items.items) + index;
    Index parentIndex = cnHeapParent(heap, index);
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


bool cnIsNaN(Float x) {
  // TODO Technique from:
  // http://stackoverflow.com/questions/570669/...
  // checking-if-a-double-or-float-is-nan-in-c
  // TODO _isnan for Windows?
  //  volatile double d = x;
  //  return d != d;
  #ifdef isnan
    return isnan(x);
  #else
    return x != x;
  #endif
}


ListAny::ListAny() {
  // TODO Is it a good idea to have itemSize 0? I guess init needs called again
  // TODO later, either way, and some init here seems safer than nothing.
  init(0);
}


ListAny::ListAny(Count itemSize) {
  init(itemSize);
}


ListAny::~ListAny() {
  dispose();
}


void ListAny::dispose() {
  if (!this) return;
  free(items);
  init(itemSize);
}


void ListAny::init(Count itemSize) {
  count = 0;
  this->itemSize = itemSize;
  items = NULL;
  reservedCount = 0;
}


void cnListClear(ListAny* list) {
  list->count = 0;
}


void cnListDestroy(ListAny* list) {
  list->dispose();
  free(list);
}


void* cnListGet(ListAny* list, Index index) {
  // TODO Support negative indexes from back?
  if (index < 0 || index >= list->count) {
    return NULL;
  }
  return ((char*)list->items) + (index * list->itemSize);
}


void* cnListGetPointer(ListAny* list, Index index) {
  return *(void**)cnListGet(list, index);
}


void* cnListEnd(const ListAny* list) {
  return ((char*)list->items) + (list->count * list->itemSize);
}


void* cnListExpand(ListAny* list) {
  return cnListExpandMulti(list, 1);
}


void* cnListExpandMulti(ListAny* list, Count count) {
  void* formerEnd;
  Count needed = list->count + count;

  // If you add nothing to an empty list, it's successful, but there's no space
  // to use, so don't pretend there's any.
  if (!count) return NULL;

  // Handle the common cases.
  if (needed > list->reservedCount) {
    void* newItems;
    // Exponential growth to avoid slow pushing.
    Count wanted = 2 * list->reservedCount;
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


void* cnListPush(ListAny* list, void* item) {
  return cnListPushMulti(list, item, 1);
}


void* cnListPushAll(ListAny* list, ListAny* from) {
  if (list->itemSize != from->itemSize) {
    printf(
      "list itemSize %ld != from itemSize %ld\n",
      list->itemSize, from->itemSize
    );
    return NULL;
  }
  return cnListPushMulti(list, from->items, from->count);
}


void* cnListPushMulti(ListAny* list, void* items, Count count) {
  void* formerEnd = cnListExpandMulti(list, count);
  if (formerEnd) {
    memcpy(formerEnd, items, list->itemSize * count);
  }
  return formerEnd;
}


void cnListRemove(ListAny* list, Index index) {
  char *begin = reinterpret_cast<char*>(cnListGet(list, index));
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


void cnListShuffle(ListAny* list) {
  // I'd rather just do simple copies than fancy n-byte xors.
  void* buffer = malloc(list->itemSize);
  // Fisher-Yates shuffling from:
  // http://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle#
  // The_modern_algorithm
  Index i;
  for (i = list->count - 1; i >= 1; i--) {
    // Find where we're going.
    void* a;
    void* b;
    Index j = rand() % (i + 1);
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


Float cnNaN(void) {
  return numeric_limits<Float>::quiet_NaN();
}


char* cnStr(String* string) {
  return string->items ? (char*)string->items : (char*)"";
}


void cnStringClear(String* string) {
  if (string->items) {
    string->count = 1;
    *cnStr(string) = '\0';
  }
}


char cnStringGetChar(String* string, Index index) {
  return *(char*)cnListGet(string, index);
}


bool cnStringPushChar(String* string, char c) {
  char* str;
  // Push a char, even though we'll soon swap it.
  // This gives us at least the right space.
  // TODO Just a call to reserve for these one or two chars?
  if (!string->reservedCount) {
    // Extra allocation because we need two chars for the first.
    if (!cnListPush(string, &c)) {
      return false;
    }
  }
  if (!cnListPush(string, &c)) {
    // TODO Anything to do with null chars for failure?
    return false;
  }
  // Now swap the null char and the final.
  str = reinterpret_cast<char*>(string->items);
  str[string->count - 2] = c;
  str[string->count - 1] = '\0';
  return true;
}


bool cnStringPushStr(String* string, const char* str) {
  char* formerEnd;
  bool wasEmpty = !string->reservedCount;
  Count extra = strlen(str);
  if (wasEmpty) {
    // For the null char.
    extra++;
  }
  // Make space.
  formerEnd = reinterpret_cast<char*>(cnListExpandMulti(string, extra));
  if (!formerEnd) {
    return false;
  }
  if (!wasEmpty) {
    // Overwrite the old null char.
    formerEnd--;
  }
  // Copy including the null char, and we're done.
  strcpy(formerEnd, str);
  return true;
}


}
