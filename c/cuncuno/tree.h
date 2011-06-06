#ifndef cuncuno_tree_h
#define cuncuno_tree_h


#include "entity.h"


typedef struct cnBinding {
  cnList(void*) entities;
} cnBinding;


typedef struct cnBindingBag {

  cnBag* bag;

  cnList(cnBinding) bindings;

} cnBindingBag;


typedef struct cnBindingBagList {

  cnList(cnBindingBag) bindingBags;

  /**
   * Reference counting because we'd like to share binding lists across lots of
   * trees, some of which will be kept, and some thrown away. In the end, that
   * makes it hard to track when to dispose of bindings.
   */
  cnCount refCount;

} cnBindingBagList;


typedef enum {
  cnNodeTypeNone, cnNodeTypeRoot, cnNodeTypeLeaf, cnNodeTypeSplit,
  cnNodeTypeVar
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
  cnList(cnEntityFunction)* entityFunctions;

  cnIndex nextId;

} cnRootNode;


typedef enum {

  cnSplitYes,

  cnSplitNo,

  cnSplitErr,

  cnSplitCount,

} cnSplitIndex;


/**
 * A node for making decisions about where to propagate bindings.
 */
typedef struct cnSplitNode {

  cnNode node;

  cnNode* kids[cnSplitCount];

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


void cnBindingBagInit(cnBindingBag* bindingBag, cnBag* bag);


/**
 * Creates a new binding bag list, with a ref count of 1.
 *
 * TODO Generalize ref-counted types?
 */
cnBindingBagList* cnBindingBagListCreate(void);


/**
 * Decrements refCount and disposes if at zero. If disposed, it disposes of the
 * bindingBags. It then frees the list itself, and sets the pointer to null.
 *
 * If the list pointer is null, then this function does nothing.
 *
 * TODO Generalize ref-counted types?
 */
void cnBindingBagListDrop(cnBindingBagList** list);


/**
 * Push on the bags as new empty bindings on the given list.
 */
cnBool cnBindingBagListPushBags(
  cnBindingBagList* bindingBags, const cnList(cnBag)* bags
);


cnLeafNode* cnLeafNodeCreate(void);


/**
 * Disposes of the given node, not freeing it. However, all its descendants are
 * both disposed of and freed.
 *
 * If node is null, this function does nothing.
 */
void cnNodeDispose(cnNode* node);
// TODO void cnNodeDrop(cnNode* node); instead or also?


/**
 * Returns an array of pointers to kids.
 */
cnNode** cnNodeKids(cnNode* node);


cnCount cnNodeKidCount(cnNode* node);


/**
 * You don't usually need to call this directly.
 */
void cnNodeInit(cnNode* node, cnNodeType type);


/**
 * Stores all leaves under this node in the given list.
 */
void cnNodeLeaves(cnNode* node, cnList(cnLeafNode*)* leaves);


/**
 * Assigns the given bindings to this node, then propagates down.
 */
cnBool cnNodePropagate(cnNode* node, cnBindingBagList* bindingBags);


/**
 * Replaces the kid at the index, if any, with the new kid, disposing of and
 * freeing the old one if any.
 */
void cnNodePutKid(cnNode* parent, cnIndex k, cnNode* kid);


/**
 * Replaces the given old kid with the new kid, disposing of the old.
 */
void cnNodeReplaceKid(cnNode* oldKid, cnNode* newKid);


/**
 * Returns the node at the top of this tree if a root node and null otherwise.
 */
cnRootNode* cnNodeRoot(cnNode* node);


/**
 * Returns the number of var nodes in the tree above the given node.
 */
cnCount cnNodeVarDepth(cnNode* node);


/**
 * Specify whether to add a leaf. That failing is the only reason root node
 * init would fail.
 */
cnBool cnRootNodeInit(cnRootNode* root, cnBool addLeaf);


cnVarNode* cnVarNodeCreate(cnBool addLeaf);


#endif
