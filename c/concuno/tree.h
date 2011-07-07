#ifndef concuno_tree_h
#define concuno_tree_h


#include "entity.h"


/**
 * An arbitrary length array of entity pointers.
 */
typedef void** cnBinding;


typedef struct cnBindingBag {

  cnBag* bag;

  /**
   * Actually, this is stored in compact form of equal numbers of entities per
   * binding.
   */
  cnList(cnBinding) bindings;

  cnCount entityCount;

} cnBindingBag;


/**
 * Leaves (and whatever else) store the bag lists, and trees get cloned, so
 * these are reference counted for convenient management.
 *
 * TODO Change to reprop every time and stop storing? Then would point to which
 * TODO leaf and called cnLeafBindingBags?
 */
typedef struct cnBindingBagList {

  cnList(cnBindingBag) bindingBags;

  /**
   * Reference counting because we'd like to share binding lists across lots of
   * trees, some of which will be kept, and some thrown away. In the end, that
   * makes it hard to track when to dispose of bindings.
   */
  cnCount refCount;

} cnBindingBagList;


/**
 * A set of points for a bag, in some topology, given by some entity function.
 * For discrete topologies, the term "point" is abusive, but I still like it
 * better than other options considered.
 *
 * Each point is presumed to be an array (all of the same length) of
 * homogeneous values, therefore representable as a matrix. Most commonly, each
 * point is a point in n-dimensional Euclidean space, but each could also be a
 * rotation angle, a quaternion, a category ID, or something else.
 */
typedef struct cnPointBag {

  cnBag* bag;

  /**
   * The total number of points in the bag.
   */
  cnCount pointCount;

  /**
   * One point at a time.
   */
  void* pointMatrix;
  // Or cnGridAny points; ??

  /**
   * Number of homogeneous values per point.
   */
  cnCount valueCount;

  /**
   * The size of each value.
   */
  cnCount valueSize;

} cnPointBag;


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

  /**
   * We need to find these guys easily across clones, hence the ids to go with.
   */
  cnIndex id;

  cnNode* parent;

  /**
   * TODO Change just to returning bindings instead of storing them?
   */
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
   * These are referenced inside the tree because they are necessary for
   * querying in addition to learning.
   *
   * TODO On the other hand, really this all gets abstracted down into the
   * TODO models in the split nodes. Do these really belong here or better just
   * TODO in the learner?
   *
   * To be managed separately from the tree itself. No disposal here.
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

  /**
   * Not owned by this node.
   */
  cnEntityFunction* function;

  cnIndex* varIndices;

  /**
   * Owned by this node and dropped with this node if not null.
   */
  cnPredicate* predicate;

} cnSplitNode;


/**
 * A variable node for binding to entities.
 */
typedef struct cnVarNode {

  cnNode node;

  cnNode* kid;

} cnVarNode;


/**
 * Disposes of the bindings but not the entities nor the bag.
 */
void cnBindingBagDispose(cnBindingBag* bindingBag);


/**
 * Creates binding bags where each binding references entityCount entities.
 */
void cnBindingBagInit(
  cnBindingBag* bindingBag, cnBag* bag, cnCount entityCount
);


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


/**
 * Determines if the binding represented by the entities is valid (has no null
 * entity pointers).
 *
 * TODO If entities were a sized 2D grid, we could avoid the count here.
 */
cnBool cnBindingValid(cnCount entityCount, void** entities);


cnLeafNode* cnLeafNodeCreate(void);


/**
 * Disposes of the given node, not freeing it. However, all its descendants are
 * both disposed of and freed.
 *
 * If node is null, this function does nothing.
 *
 * TODO Rename to cnTreeDispose to clarify deep nature of disposal?
 */
void cnNodeDispose(cnNode* node);


/**
 * Disposes of and frees the given node and all its descendants.
 *
 * TODO Get rid of init/dispose on nodes from public interface?
 *
 * TODO Rename to cnTreeDrop to clarify deep nature of the drop?
 */
void cnNodeDrop(cnNode* node);


/**
 * Searches depth first under the given node to find the node with the given
 * id, returning a pointer to that node if found or else null.
 */
cnNode* cnNodeFindById(cnNode* node, cnIndex id);


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
cnBool cnNodeLeaves(cnNode* node, cnList(cnLeafNode*)* leaves);


/**
 * Assigns the given bindings to this node, then propagates down.
 *
 * If null bindingBags, repropagate the currently stored ones, if any.
 *
 * TODO Change just to returning bindings instead of storing them?
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


/**
 * For top-level convenience, propagates empty bindings made from the given
 * bags.
 *
 * TODO Change just to returning bindings instead of storing them?
 */
cnBool cnRootNodePropagateBags(cnRootNode* root, cnList(cnBag)* bags);


cnSplitNode* cnSplitNodeCreate(cnBool addLeaves);


/**
 * Fills the list of value bags with values according to the bindings and the
 * function at this node.
 *
 * TODO Guaranteed to be in the same order as the binding bags and bindings.
 */
cnBool cnSplitNodePointBags(cnSplitNode* split, cnList(cnPointBag)* pointBags);


/**
 * Copies this node and all under, except the bindings. (TODO Keep the models?)
 * Original references to bindings are kept to avoid excessive data copying.
 */
cnNode* cnTreeCopy(cnNode* node);


cnVarNode* cnVarNodeCreate(cnBool addLeaf);


#endif
