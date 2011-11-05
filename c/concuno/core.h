#ifndef concuno_core_h
#define concuno_core_h

// TODO Abstract out standard libraries?
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <vector>


#ifdef __cplusplus
  #define cnCBegin extern "C" {
  #define cnCEnd }
#else
  #define cnCBegin
  #define cnCEnd
#endif


namespace concuno {


// Prettier in some contexts than the braces.
#define cnEnd }


typedef unsigned char Byte;


typedef long Int;


typedef Int Count;


typedef double Float;


typedef Int Index;


/**
 * Pointer to a single anything.
 */
typedef void* RefAny;


struct Error: std::runtime_error {

  Error(const std::string& what);

};


/**
 * This automanaged list is more dynamic than what's done with vector.
 */
struct ListAny {

  ListAny();

  ListAny(Count itemSize);

  /**
   * Null out the items in advance if you don't want them freed.
   */
  ~ListAny();

  /**
   * Call this manually every time when done, sadly.
   */
  void dispose();

  /**
   * Get a pointer into the items array at a given index.
   */
  void* get(Index index);

  /**
   * Call this manually if you didn't use the itemSize constructor.
   */
  void init(Count itemSize);

  Count count;

  Count itemSize;

  void* items;

  Count reservedCount;

};


template<typename Item>
struct List: ListAny {

  /**
   * Note count not item size here! This allows for vectors of elements as each
   * item, rather than a single item.
   *
   * TODO Reconsider this! Go to grid or something!
   */
  List(Count vectorSize = 1): ListAny(vectorSize * sizeof(Item)) {}

  Item& first() {
    return *reinterpret_cast<Item*>(items);
  }

  void init(Count vectorSize = 1) {
    ListAny::init(vectorSize * sizeof(Item));
  }

  Item& operator[](Index index) {
    return *reinterpret_cast<Item*>(get(index));
  }

  /**
   * TODO This only applies for pointers. More to a pointer-specific something?
   */
  void pushOrDelete(Item& item) {
    if (!cnListPush(this, &item)) {
      delete item;
      throw Error("Push failed.");
    }
  }

  Count vectorSize() {
    return itemSize / sizeof(Item);
  }

};


/**
 * A vector only for pointer items, whose items are deleted automatically on
 * destruct.
 */
template<typename Item>
struct AutoVec {

  ~AutoVec() {
    // TODO Iterators here, I guess.
    for (size_t i = 0; i < items.size(); i++) {
      delete items[i];
    }
  }

  Item& operator[](size_t i) {
    return items[i];
  }

  std::vector<Item>& operator*() {
    return items;
  }

  std::vector<Item>* operator->() {
    return &items;
  }

  void push(const Item& item) {
    items.push_back(item);
  }

private:

  std::vector<Item> items;

};


template<typename Item>
void pushOrDelete(std::vector<Item>& container, Item item) {
  try {
    container.push_back(item);
  } catch (std::exception& e) {
    delete item;
    throw;
  }
}


/**
 * Null-terminated UTF-8 string.
 */
typedef List<char> String;


/**
 * Macro to simplify malloc in move to cpp.
 */
#define cnAlloc(Type, count) \
  reinterpret_cast<Type*>(malloc(count * sizeof(Type)))


/**
 * Stores an ND array of items.
 *
 * TODO It would actually be good to use this sometime. For now, I just hack
 * TODO Lists into submission.
 */
struct GridAny {

  /**
   * The count for each dimension, and dims.count is the number of dimensions.
   *
   * It is safest not to modify these values directly, so that consistency with
   * the data can be maintained.
   *
   * TODO Matlab style or Numpy style?
   */
  List<Count> dims;

  /**
   * Value item storage. Note that values.count must equal the product of the
   * dimensions. There is some redundancy in this, but it's convenient enough.
   *
   * It is safest not to resize the values directly, so that consistency with
   * the dims can be maintained. Modifying the actual values should be
   * reasonably safe, however.
   */
  ListAny values;

};


/**
 * Use when not caring about messages.
 *
 * TODO Use __FILE__ but wrapped to show just the last file name, not full path.
 */
#define cnFailTo(label) { \
  printf("Failed (in %s at line %d)\n", __FUNCTION__, __LINE__); \
  goto label; \
}


/**
 * Use to fail with a particular message.
 */
#define cnErrTo(label, message, ...) { \
  printf( \
    message " (in %s at line %d)\n", ## __VA_ARGS__, __FUNCTION__, __LINE__); \
  goto label; \
}


/**
 * A binary heap usable as a priority queue.
 *
 * I'm experimenting with providing this only in pointer form, as I've more than
 * once needed to change from expanded to pointer form,
 */
struct HeapAny {

  /**
   * Set this if you need extra information for comparing items.
   */
  RefAny info;

  /**
   * The items in the heap. Note that items are always pointers.
   *
   * TODO After complaining about expanded forms, I want to use an expanded list
   * TODO here. I really like tying its life in here. Grr.
   */
  List<RefAny> items;

  /**
   * Set to non-null if you want automatic info destruction.
   */
  bool (*destroyInfo)(RefAny info);

  /**
   * Set to non-null if you want automatic item destruction.
   *
   * Called before destroyInfo.
   */
  void (*destroyItem)(RefAny info, RefAny item);

  /**
   * Says whether a is less than b.
   */
  bool (*less)(RefAny info, RefAny a, RefAny b);

};


/**
 * Since all heaps here are of pointers, it might be good enough form just to
 * say cnHeap(cnIndex) to mean a heap of _pointers_ to indices, for example.
 * It's just convention, and saying cnHeap(cnRef(cnIndex)) or even just
 * cnHeap(cnIndex*) could be a bother.
 */
template<typename Item>
struct Heap: HeapAny {};


/**
 * Inits the dims and values to empty lists.
 */
void cnGridInit(GridAny* grid);


/**
 * Inits the dims to the number of rows and columns specified, and preallocates
 * the space requested.
 */
bool cnGridInit2d(GridAny* grid, Count nrows, Count ncols);


/**
 * Inits the dims to a copy of the dims given, and preallocates the space
 * requested.
 */
bool cnGridInitNd(GridAny* grid, const List<Count>* dims);


/**
 * Clears out the heap contents, destroying the items if destroyItem is set.
 */
void cnHeapClear(HeapAny* heap);


/**
 * All the contents will be cleared out and/or init'd. You still need to set at
 * least the less function for things work, though.
 *
 * TODO An option for initial contents for O(n) heapify?
 */
HeapAny* cnHeapCreate(bool (*less)(RefAny info, RefAny a, RefAny b));


void cnHeapDestroy(HeapAny* heap);


RefAny cnHeapPeek(HeapAny* heap);


RefAny cnHeapPull(HeapAny* heap);


bool cnHeapPush(HeapAny* heap, RefAny item);


/**
 * Returns whether x is NaN.
 */
bool cnIsNaN(Float x);


/**
 * Sets the count to 0, but leaves space allocated.
 */
void cnListClear(ListAny* list);


/**
 * Disposes of and frees the list.
 */
void cnListDestroy(ListAny* list);


/**
 * Perform the action for each item. Variable i will be of type Type*. The loop
 * increment uses itemSize explicitly so that sizeof(Type) doesn't necessarily
 * have to match. That is, you can pick whatever pointer type you want.
 *
 * Terminate with cnEnd.
 */
#define cnListEachBegin(list, Type, i) { \
  Type* i; \
  Type* end = (Type*)cnListEnd(list); \
  for ( \
    i = (Type*)(list)->items; \
    i < end; \
    i = (Type*)(((char*)i) + (list)->itemSize) \
  )


/**
 * The address right after the end of the list. Useful for iteration, but don't
 * try to store anything here.
 */
void* cnListEnd(const ListAny* list);


/**
 * Adds one item to the list with uninitialized contents, suitable for
 * filling in after the fact.
 *
 * Returns a pointer to the beginning of the new space.
 */
void* cnListExpand(ListAny* list);


/**
 * Adds count items to the list with uninitialized contents, suitable for
 * filling in after the fact.
 *
 * Returns a pointer to the beginning of the new space.
 *
 * WARNING: If you try to add nothing, you'll get a NULL back, as there's no
 * space to use. However, it's still technically successful. That makes
 * usability awkward. TODO Consider changing the API here?
 */
void* cnListExpandMulti(ListAny* list, Count count);


/**
 * Assumes this list holds pointers and returns the pointer value instead of
 * the pointer to the pointer.
 */
void* cnListGetPointer(ListAny* list, Index index);


/**
 * Pushes a single item from the given address onto the list.
 *
 * Returns the destination pointer to the item just pushed, or NULL if failure.
 */
void* cnListPush(ListAny* list, const void* item);


/**
 * Pushes all of the from list onto the given list.
 *
 * Returns the destination pointer to the first item just pushed, or NULL if
 * failure.
 */
void* cnListPushAll(ListAny* list, const ListAny* from);


/**
 * Pushes count items onto the given list.
 *
 * Returns the destination pointer to the first item just pushed, or NULL if
 * failure.
 */
void* cnListPushMulti(ListAny* list, const void* items, Count count);


void cnListPut(ListAny* list, Index index, void* value);


/**
 * Does not reduce memory usage nor change the address of items. The count is
 * reduced by one and the item at the index is deleted from the array.
 */
void cnListRemove(ListAny* list, Index index);


/**
 * Shuffles the list.
 *
 * TODO Pass in a rand stream.
 *
 * TODO Simple unit test of this would be nice.
 */
void cnListShuffle(ListAny* list);


/**
 * Quiet NaN (not a number).
 */
Float cnNaN(void);


/**
 * Allocates the given number of bytes on the stack, if that's supported by the
 * platform. Otherwise allocates the memory on the heap.
 *
 * It could return null for failure in some cases. In others, stack overflow
 * might occur undetected. Use only for space expected to be small.
 *
 * TODO _malloca and/or _alloca on Windows?
 */
#define cnStackAlloc(byteCount) alloca(byteCount)


/**
 * Helper for cpp needs.
 */
#define cnStackAllocOf(Type, count) \
  reinterpret_cast<Type*>(alloca(count * sizeof(Type)))


/**
 * Frees the memory allocated by cnStackAlloc. Does nothing in most cases for
 * memory actually allocated on the stack.
 *
 * TODO _freea on Windows?
 */
#define cnStackFree(memory)


/**
 * Provides a usable string, including for the case of no data in the items
 * buffer. In this case, provides a static empty string. Therefore if the count
 * is 0, do not modify the returned string.
 */
char* cnStr(String* string);


/**
 * Changes content to an empty string and count to 1, if items are already
 * allocated. If not, just leaves it alone.
 */
void cnStringClear(String* string);


char cnStringGetChar(String* string, Index index);


bool cnStringPushChar(String* string, char c);


bool cnStringPushStr(String* string, const char* str);


}


#endif
