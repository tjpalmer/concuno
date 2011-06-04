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
} cnList;


/**
 * Null-terminated UTF-8 string.
 */
typedef cnList cnString;


/**
 * Sets the count to 0, but leaves space allocated.
 */
void cnListClear(cnList* list);


void cnListDispose(cnList* list);


/**
 * Perform the action for each item. Variable i will be of type Type*.
 *
 * Terminate with cnEnd.
 */
#define cnListEachBegin(list, Type, i) { \
    Type *i; \
    Type *end; \
    for (i = (list)->items, end = cnListEnd(list); i < end; i++)


/**
 * The address right after the end of the list. Useful for iteration, but don't
 * try to store anything here.
 */
void* cnListEnd(const cnList* list);


/**
 * Adds one item to the list with uninitialized contents, suitable for
 * filling in after the fact.
 *
 * Returns a pointer to the beginning of the new space.
 */
void* cnListExpand(cnList* list);


/**
 * Adds count items to the list with uninitialized contents, suitable for
 * filling in after the fact.
 *
 * Returns a pointer to the beginning of the new space.
 */
void* cnListExpandMulti(cnList* list, cnCount count);


void* cnListGet(cnList* list, cnIndex index);


void cnListInit(cnList* list, cnCount itemSize);


/**
 * Pushes a single item from the given address onto the list.
 *
 * Returns the destination pointer to the item just pushed, or NULL if failure.
 */
void* cnListPush(cnList* list, void* item);


/**
 * Pushes all of the from list onto the given list.
 *
 * Returns the destination pointer to the first item just pushed, or NULL if
 * failure.
 */
void* cnListPushAll(cnList* list, cnList* from);


/**
 * Pushes count items onto the given list.
 *
 * Returns the destination pointer to the first item just pushed, or NULL if
 * failure.
 */
void* cnListPushMulti(cnList* list, void* items, cnCount count);


void cnListPut(cnList* list, cnIndex index, void* value);


/**
 * Does not reduce memory usage nor change the address of items. The count is
 * reduced by one and the item at the index is deleted from the array.
 */
void cnListRemove(cnList* list, cnIndex index);


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
