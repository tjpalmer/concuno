#ifndef cuncuno_core_h
#define cuncuno_core_h


#include <stdlib.h>


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


void* cnListGet(cnList* list, cnIndex index);


void cnListInit(cnList* list, cnCount itemSize);


/**
 * Pushes a single item from the given address onto the list.
 */
cnBool cnListPush(cnList* list, void* item);


void cnListPut(cnList* list, cnIndex index, void* value);


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
