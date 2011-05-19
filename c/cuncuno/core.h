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
typedef struct cnString {
  char* buffer;
  cnCount reservedCount;
} cnString;


void cnListClean(cnList* list);


void* cnListGet(cnList* list, cnIndex index);


void cnListInit(cnList* list, cnCount itemSize);


cnBool cnListPush(cnList* list, void* item);


cnBool cnStringInit(cnString* string);


cnBool cnStringClean(cnString* string);


#endif
