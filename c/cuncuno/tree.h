#ifndef cuncuno_tree_h
#define cuncuno_tree_h


#include "entity.h"


typedef struct cnBindingBag {
  cnSample* sample;
} cnBindingBag;


typedef enum {
  cnNodeTypeRoot, cnNodeTypeVar, cnNodeTypeSplit, cnNodeTypeLeaf
} cnNodeType;


typedef struct cnNode {

  cnNodeType type;

  cnIndex id;

} cnNode;


#endif
