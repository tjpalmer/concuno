#ifndef cuncuno_tree_h
#define cuncuno_tree_h


#include "entity.h"


typedef struct cnBinding {
  cnList entities;
} cnBinding;


typedef struct cnBindingBag {

  cnBag* bag;

  cnList bindings;

} cnBindingBag;


typedef struct cnBindingBagList {

  cnList bindingBags;

  /**
   * Reference counting because we'd like to share binding lists across lots of
   * trees, some of which will be kept, and some thrown away. In the end, that
   * makes it hard to track when to dispose of bindings.
   */
  cnCount refCount;

} cnBindingBagList;


typedef enum {
  cnNodeTypeNone, cnNodeTypeRoot, cnNodeTypeVar, cnNodeTypeSplit, cnNodeTypeLeaf
} cnNodeType;


struct cnNode;
typedef struct cnNode cnNode;


/**
 * Abstract node base type.
 */
struct cnNode {

  cnNodeType type;

  cnIndex id;

  cnNode* parent;

  cnBindingBagList* bindingBagList;

};


/**
 * A leaf node (with no kids).
 */
typedef struct cnLeafNode {

  cnNode node;

  cnFloat probability;

} cnLeafNode;


/**
 * The node at the top of the tree.
 */
typedef struct cnRootNode {

  cnNode node;

  cnNode* kid;

  /**
   * To be managed separately from the tree itself.
   */
  cnList* entityFunctions;

  cnIndex nextId;

} cnRootNode;


/**
 * A node for making decisions about where to propagate bindings.
 */
typedef struct cnSplitNode {

  cnNode node;

  cnNode* yes;

  cnNode* no;

  cnNode* err;

} cnSplitNode;


/**
 * A variable node for binding to entities.
 */
typedef struct cnVarNode {

  cnNode node;

  cnNode* kid;

} cnVarNode;


/**
 * Disposes of the entity list but not the entities themselves.
 */
void cnBindingDispose(cnBinding* binding);


/**
 * Disposes of the bindings but not the bag.
 */
void cnBindingBagDispose(cnBindingBag* bindingBag);


/**
 * Creates a new binding bag list, with a ref count of 1.
 *
 * TODO Generalize ref-counted types?
 */
cnBindingBagList* cnBindingBagListCreate(void);

/**
 * Decrements refCount and disposes if at zero. If disposed, it disposes of the
 * bindingBags. It then frees the list itself.
 *
 * If the list pointer is null, then this function does nothing.
 *
 * TODO Generalize ref-counted types?
 */
void cnBindingBagListDrop(cnBindingBagList* list);


/**
 * You don't usually need to call this directly.
 */
void cnNodeDispose(cnNode* node);


/**
 * You don't usually need to call this directly.
 */
void cnNodeInit(cnNode* node, cnNodeType type);


void cnRootNodeDispose(cnRootNode* node);


void cnRootNodeInit(cnRootNode* node);


#endif
