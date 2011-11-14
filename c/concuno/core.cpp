#include <iostream>
#include <limits>
#include <math.h>
#include <sstream>
#include <string.h>
#include "core.h"

using namespace std;


namespace concuno {


Error::Error(const string& what): runtime_error(what) {}


Error::Error(const basic_ostream<char>& what):
  runtime_error(dynamic_cast<const stringstream&>(what).str())
{}


/**
 * Bubbles the item at the given index down the heap.
 */
void cnHeapDown(HeapAny* heap, Index index);


/**
 * Provides the kids of the given parent index, using -1 to mean no such kid.
 */
void cnHeapKids(
  HeapAny* heap, Index parentIndex,
  Index* kid1Index, Index* kid2Index
);


/**
 * Provides the parent of the given kid index, providing -1 for no parent (in
 * the case of the root at index 0).
 */
Index cnHeapParent(HeapAny heap, Index kidIndex);


/**
 * Bubbles the item at the given index up the heap.
 */
void cnHeapUp(HeapAny* heap, Index index);


void cnHeapClear(HeapAny* heap) {
  if (heap->destroyItem) {
    cnListEachBegin(&heap->items, RefAny, item) {
      heap->destroyItem(heap->info, *item);
    } cnEnd;
  }
  cnListClear(&heap->items);
}


HeapAny* cnHeapCreate(bool (*less)(RefAny info, RefAny a, RefAny b)) {
  HeapAny* heap = cnAlloc(HeapAny, 1);
  if (!heap) cnErrTo(DONE, "No heap.");
  heap->destroyInfo = NULL;
  heap->destroyItem = NULL;
  heap->info = NULL;
  new(&heap->items) List<RefAny>;
  heap->less = less;
  DONE:
  return heap;
}


void cnHeapDestroy(HeapAny* heap) {
  // No op on null.
  if (!heap) return;

  // Items.
  cnHeapClear(heap);
  heap->items.~List<RefAny>();

  // Info.
  if (heap->destroyInfo) heap->destroyInfo(heap->info);

  // Heap.
  // TODO Null everything out for extra safety?
  free(heap);
}


void cnHeapDown(HeapAny* heap, Index index) {
  while (index >= 0) {
    RefAny* item = ((RefAny*)heap->items.items) + index;
    Index kidIndex;
    RefAny* kidItem;
    Index kid2Index;

    // Find the kids, and see if they exist.
    cnHeapKids(heap, index, &kidIndex, &kid2Index);
    if (kidIndex >= 0) {
      // Presume swapping with kid 1 for now.
      kidItem = ((RefAny*) heap->items.items) + kidIndex;
    } else {
      // We're at the bottom.
      break;
    }
    if (kid2Index >= 0) {
      RefAny* kid2Item =
        ((RefAny*)heap->items.items) + kid2Index;
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
      RefAny tempItem;
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
  HeapAny* heap, Index parentIndex,
  Index* kid1Index, Index* kid2Index
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


Index cnHeapParent(HeapAny* heap, Index kidIndex) {
  // TODO Use a "B-Heap" or Emde Boas layout or some such for more efficient
  // TODO memory access.
  return kidIndex ? (kidIndex - 1) / 2 : -1;
}


RefAny cnHeapPeek(HeapAny* heap) {
  // Dereference to the stored pointer, if we have anything.
  return heap->items.items ? *(RefAny*)heap->items.items : NULL;
}


RefAny cnHeapPull(HeapAny* heap) {
  RefAny pulled = cnHeapPeek(heap);
  if (heap->items.count) {
    // TODO With a different memory layout, is the last item still always legit?
    // Swap the bottom item to the top.
    // We always store pointers here, so hack the array type.
    // For the last item, this is inefficient, but that's not a common use case.
    ((RefAny*)heap->items.items)[0] =
      ((RefAny*)heap->items.items)[heap->items.count - 1];
    ((RefAny*)heap->items.items)[heap->items.count - 1] = NULL;
    // We always remove the last, so this is easy.
    heap->items.count--;
    cnHeapDown(heap, 0);
  } else {
    // TODO Error or okay to pull from an empty heap?
  }
  return pulled;
}


bool cnHeapPush(HeapAny* heap, RefAny item) {
  bool result = false;
  if (!cnListPush(&heap->items, &item)) cnErrTo(DONE, "No push.");
  cnHeapUp(heap, heap->items.count - 1);
  result = true;
  DONE:
  return result;
}


void cnHeapUp(HeapAny* heap, Index index) {
  while (index) {
    RefAny* item = ((RefAny*)heap->items.items) + index;
    Index parentIndex = cnHeapParent(heap, index);
    RefAny* parentItem =
      ((RefAny*)heap->items.items) + parentIndex;
    // TODO Error checking on less? It hypothetically could fail ...
    if (heap->less(heap->info, *item, *parentItem)) {
      // Swap parent and kid.
      RefAny temp = *parentItem;
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


void* ListAny::get(Index index) {
  // TODO Support negative indexes from back?
  if (index < 0 || index >= count) {
    // TODO Say which index?
    throw Error("Bad list index.");
  }
  return ((char*)items) + (index * itemSize);
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


void* cnListGetPointer(ListAny* list, Index index) {
  return *(void**)list->get(index);
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


void* cnListPush(ListAny* list, const void* item) {
  return cnListPushMulti(list, item, 1);
}


void* cnListPushAll(ListAny* list, const ListAny* from) {
  if (list->itemSize != from->itemSize) {
    printf(
      "list itemSize %ld != from itemSize %ld\n",
      list->itemSize, from->itemSize
    );
    return NULL;
  }
  return cnListPushMulti(list, from->items, from->count);
}


void* cnListPushMulti(ListAny* list, const void* items, Count count) {
  void* formerEnd = cnListExpandMulti(list, count);
  if (formerEnd) {
    memcpy(formerEnd, items, list->itemSize * count);
  }
  return formerEnd;
}


void cnListRemove(ListAny* list, Index index) {
  char *begin = reinterpret_cast<char*>(list->get(index));
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
  return *reinterpret_cast<char*>(string->get(index));
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


ostream& Buf::operator<<(const char* str) {
  return dynamic_cast<ostream&>(*this) << str;
}


ostream& Buf::operator<<(Float $float) {
  return dynamic_cast<ostream&>(*this) << $float;
}


ostream& Buf::operator<<(const std::string& str) {
  return dynamic_cast<ostream&>(*this) << str;
}


void log(const char* topic, const char* message) {
  cout << topic << ": " << message << endl;
}


void log(const char* topic, const std::string& message) {
  cout << topic << ": " << message << endl;
}


void log(const char* topic, const std::basic_ostream<char>& message) {
  log(str(message));
}


void log(const char* message) {
  log("info", message);
}


void log(const std::string& message) {
  log("info", message);
}


void log(const std::basic_ostream<char>& message) {
  log("info", message);
}


bool logging(const char* topic) {
  return true;
}


std::string str(const std::basic_ostream<char>& buffer) {
  return dynamic_cast<const stringstream&>(buffer).str();
}


}
