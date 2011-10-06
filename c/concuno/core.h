#ifndef concuno_core_h
#define concuno_core_h


// TODO Abstract out standard libraries?
#include <stdio.h>
#include <stdlib.h>


#ifdef __cplusplus
  #define cnCBegin extern "C" {
  #define cnCEnd }
#else
  #define cnCBegin
  #define cnCEnd
#endif


cnCBegin;


// Prettier in some contexts than the braces.
#define cnEnd }


typedef enum {cnFalse, cnTrue} cnBool;


typedef unsigned char cnByte;


typedef long cnInt;


typedef cnInt cnCount;


typedef double cnFloat;


typedef cnInt cnIndex;


/**
 * Pointer to an array of unknown count of anything.
 */
typedef void* cnArrAny;


/**
 * Pointer to an array of unknown count of a particular type.
 */
#define cnArr(Type) Type*


/**
 * Pointer to a single anything.
 */
typedef void* cnRefAny;


/**
 * Pointer to one thing of a particular type.
 */
#define cnRef(Type) Type*


typedef struct cnList {
  cnCount count;
  cnCount itemSize;
  void* items;
  cnCount reservedCount;
} cnListAny;


/**
 * Faux generics. Use this rather than cnListAny, when you can.
 */
#define cnList(Type) cnListAny


/**
 * Null-terminated UTF-8 string.
 */
typedef cnList(char) cnString;


/**
 * Stores an ND array of items.
 *
 * TODO It would actually be good to use this sometime. For now, I just hack
 * TODO Lists into submission.
 */
typedef struct cnGridAny {

  /**
   * The count for each dimension, and dims.count is the number of dimensions.
   *
   * It is safest not to modify these values directly, so that consistency with
   * the data can be maintained.
   *
   * TODO Matlab style or Numpy style?
   */
  cnList(cnCount) dims;

  /**
   * Value item storage. Note that values.count must equal the product of the
   * dimensions. There is some redundancy in this, but it's convenient enough.
   *
   * It is safest not to resize the values directly, so that consistency with
   * the dims can be maintained. Modifying the actual values should be
   * reasonably safe, however.
   */
  cnListAny values;

} cnGridAny;


/**
 * Faux generics. Use this rather than cnGridAny, when you can.
 */
#define cnGrid(Type) cnGridAny


#define cnFailTo(label, message, ...) { \
  printf(message "\n", ## __VA_ARGS__); \
  goto label; \
}


/**
 * A binary heap usable as a priority queue.
 *
 * I'm experimenting with providing this only in pointer form, as I've more than
 * once needed to change from expanded to pointer form,
 */
typedef struct cnHeapAny {

  /**
   * Set this if you need extra information for comparing items.
   */
  cnRefAny info;

  /**
   * The items in the heap. Note that items are always pointers.
   *
   * TODO After complaining about expanded forms, I want to use an expanded list
   * TODO here. I really like tying its life in here. Grr.
   */
  cnList(cnRefAny) items;

  /**
   * Set to non-null if you want automatic info destruction.
   */
  cnBool (*destroyInfo)(cnRefAny info);

  /**
   * Set to non-null if you want automatic item destruction.
   *
   * Called before destroyInfo.
   */
  void (*destroyItem)(cnRefAny info, cnRefAny item);

  /**
   * Says whether a is less than b.
   */
  cnBool (*less)(cnRefAny info, cnRefAny a, cnRefAny b);

}* cnHeapAny;


/**
 * Since all heaps here are of pointers, it might be good enough form just to
 * say cnHeap(cnIndex) to mean a heap of _pointers_ to indices, for example.
 * It's just convention, and saying cnHeap(cnRef(cnIndex)) or even just
 * cnHeap(cnIndex*) could be a bother.
 */
#define cnHeap(Type) cnHeapAny


/**
 * Inits the dims and values to empty lists.
 */
void cnGridInit(cnGridAny* grid);


/**
 * Inits the dims to the number of rows and columns specified, and preallocates
 * the space requested.
 */
cnBool cnGridInit2d(cnGridAny* grid, cnCount nrows, cnCount ncols);


/**
 * Inits the dims to a copy of the dims given, and preallocates the space
 * requested.
 */
cnBool cnGridInitNd(cnGridAny* grid, const cnList(cnCount)* dims);


/**
 * Clears out the heap contents, destroying the items if destroyItem is set.
 */
void cnHeapClear(cnHeapAny heap);


/**
 * All the contents will be cleared out and/or init'd. You still need to set at
 * least the less function for things work, though.
 *
 * TODO An option for initial contents for O(n) heapify?
 */
cnHeapAny cnHeapCreate(cnBool (*less)(cnRefAny info, cnRefAny a, cnRefAny b));


void cnHeapDestroy(cnHeapAny heap);


cnRefAny cnHeapPeek(cnHeapAny heap);


cnRefAny cnHeapPull(cnHeapAny heap);


cnBool cnHeapPush(cnHeapAny heap, cnRefAny item);


/**
 * Returns whether x is NaN.
 */
cnBool cnIsNaN(cnFloat x);


/**
 * Sets the count to 0, but leaves space allocated.
 */
void cnListClear(cnListAny* list);


/**
 * Disposes of and frees the list.
 */
void cnListDestroy(cnListAny* list);


/**
 * Frees contents and resets the list to be usable as an empty list for items of
 * the same size as before.
 */
void cnListDispose(cnListAny* list);


/**
 * Perform the action for each item. Variable i will be of type Type*. The loop
 * increment uses itemSize explicitly so that sizeof(Type) doesn't necessarily
 * have to match. That is, you can pick whatever pointer type you want.
 *
 * Terminate with cnEnd.
 */
#define cnListEachBegin(list, Type, i) { \
  Type* i; \
  Type* end = cnListEnd(list); \
  for ( \
    i = (list)->items; i < end; i = (Type*)(((char*)i) + (list)->itemSize) \
  )


/**
 * The address right after the end of the list. Useful for iteration, but don't
 * try to store anything here.
 */
void* cnListEnd(const cnListAny* list);


/**
 * Adds one item to the list with uninitialized contents, suitable for
 * filling in after the fact.
 *
 * Returns a pointer to the beginning of the new space.
 */
void* cnListExpand(cnListAny* list);


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
void* cnListExpandMulti(cnListAny* list, cnCount count);


void* cnListGet(cnListAny* list, cnIndex index);


/**
 * Assumes this list holds pointers and returns the pointer value instead of
 * the pointer to the pointer.
 */
void* cnListGetPointer(cnListAny* list, cnIndex index);


void cnListInit(cnListAny* list, cnCount itemSize);


/**
 * Pushes a single item from the given address onto the list.
 *
 * Returns the destination pointer to the item just pushed, or NULL if failure.
 */
void* cnListPush(cnListAny* list, void* item);


/**
 * Pushes all of the from list onto the given list.
 *
 * Returns the destination pointer to the first item just pushed, or NULL if
 * failure.
 */
void* cnListPushAll(cnListAny* list, cnListAny* from);


/**
 * Pushes count items onto the given list.
 *
 * Returns the destination pointer to the first item just pushed, or NULL if
 * failure.
 */
void* cnListPushMulti(cnListAny* list, void* items, cnCount count);


void cnListPut(cnListAny* list, cnIndex index, void* value);


/**
 * Does not reduce memory usage nor change the address of items. The count is
 * reduced by one and the item at the index is deleted from the array.
 */
void cnListRemove(cnListAny* list, cnIndex index);


/**
 * Shuffles the list.
 *
 * TODO Pass in a rand stream.
 *
 * TODO Simple unit test of this would be nice.
 */
void cnListShuffle(cnListAny* list);


/**
 * Quiet NaN (not a number).
 */
cnFloat cnNaN(void);


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
char* cnStr(cnString* string);


/**
 * Changes content to an empty string and count to 1, if items are already
 * allocated. If not, just leaves it alone.
 */
void cnStringClear(cnString* string);


void cnStringDispose(cnString* string);


char cnStringGetChar(cnString* string, cnIndex index);


/**
 * Provides a pointer to an empty string, but not dynamically allocated.
 */
void cnStringInit(cnString* string);


cnBool cnStringPushChar(cnString* string, char c);


cnBool cnStringPushStr(cnString* string, char* str);


cnCEnd;


#endif
