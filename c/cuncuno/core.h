#ifndef cuncuno_core_h
#define cuncuno_core_h


#include <stdlib.h>


typedef enum {cnFalse, cnTrue} cnBool;


typedef int cnCount;


typedef double cnFloat;


typedef int cnIndex;


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


void cnListClean(cnList* list);


void* cnListGet(cnList* list, cnIndex index);


void cnListInit(cnList* list, cnCount itemSize);


/**
 * Pushes a single item from the given address onto the list.
 */
cnBool cnListPush(cnList* list, void* item);


void cnStringClean(cnString* string);


char cnStringGetChar(cnString* string, cnIndex index);


/**
 * Provides a pointer to an empty string, but not dynamically allocated.
 */
void cnStringInit(cnString* string);


cnBool cnStringPushChar(cnString* string, char c);


cnBool cnStringPushStr(cnString* string, char* str);


char* cnStringStr(cnString* string);


#endif
