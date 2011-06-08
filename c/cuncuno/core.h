#ifndef cuncuno_core_h
#define cuncuno_core_h


#include <stdlib.h>


// Prettier in some contexts than the braces.
#define cnEnd }


typedef enum {cnFalse, cnTrue} cnBool;


typedef unsigned char cnByte;


typedef long cnCount;


typedef double cnFloat;


typedef long cnIndex;


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
 * Sets the count to 0, but leaves space allocated.
 */
void cnListClear(cnListAny* list);


void cnListDispose(cnListAny* list);


/**
 * Perform the action for each item. Variable i will be of type Type*.
 *
 * Terminate with cnEnd.
 */
#define cnListEachBegin(list, Type, i) { \
    Type *i; \
    Type *end = cnListEnd(list); \
    for (i = (list)->items; i < end; i++)


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
 */
void* cnListExpandMulti(cnListAny* list, cnCount count);


void* cnListGet(cnListAny* list, cnIndex index);


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


#endif
