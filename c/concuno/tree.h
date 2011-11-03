#ifndef concuno_tree_h
#define concuno_tree_h


#include "entity.h"


namespace concuno {


/**
 * An arbitrary length array of entity pointers.
 */
typedef Entity* Binding;


struct BindingBag {

  /**
   * Nulls things out, assumes zero-length binding vectors, and so on.
   */
  BindingBag();

  /**
   * Creates binding bags where each binding references entityCount entities.
   */
  BindingBag(Bag* bag, Count entityCount);

  void init(Bag* bag, Count entityCount);

  Bag* bag;

  /**
   * Actually, this is stored in compact form of equal numbers of entities per
   * binding.
   */
  List<Entity> bindings;

  Count entityCount;

};


/**
 * Leaves (and whatever else) store the bag lists, and trees get cloned, so
 * these are reference counted for convenient management.
 *
 * TODO Change to reprop every time and stop storing? Then would point to which
 * TODO leaf and called cnLeafBindingBags?
 */
struct BindingBagList {

  List<BindingBag> bindingBags;

  /**
   * Reference counting because we'd like to share binding lists across lots of
   * trees, some of which will be kept, and some thrown away. In the end, that
   * makes it hard to track when to dispose of bindings.
   */
  Count refCount;

};


/**
 * Just a matrix of vectors, really.
 *
 * TODO Get grid working sometime?
 *
 * TODO Use this inside cnPointBag?
 */
struct PointMatrix {

  /**
   * The total number of points in the bag.
   */
  Count pointCount;

  /**
   * An array of pointCount points of valueCount values each.
   *
   * TODO Arbitrary topologies?
   */
  Float* points;
  // Or cnGridAny points; ??

  /**
   * It's easy to imagine KD trees on angles, quaternions, categories, or other.
   *
   * TODO Support other than Euclidean.
   */
  Topology topology;

  /**
   * Number of homogeneous values per point.
   */
  Count valueCount;

  /**
   * The size of each value.
   */
  Count valueSize;

};


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
struct PointBag {

  Bag* bag;

  /**
   * A mapping from bindings to points, in the same order as the bindings in the
   * bag. If null, the points are one-to-one in the same order as the bindings.
   *
   * Could instead do a list of list of bindings, since this assumes you know
   * what the original bindings are, but this is more efficient. Just don't use
   * this list unless you know the bindings (including how many there are).
   *
   * TODO Do go to the list of lists?
   */
  Index* bindingPointIndices;

  /**
   * One point at a time.
   */
  PointMatrix pointMatrix;

};


enum NodeType {
  cnNodeTypeNone, cnNodeTypeRoot, cnNodeTypeLeaf, cnNodeTypeSplit,
  cnNodeTypeVar
};


/**
 * Abstract node base type.
 */
struct Node {

  NodeType type;

  /**
   * We need to find these guys easily across clones, hence the ids to go with.
   */
  Index id;

  Node* parent;

};


/**
 * A leaf node (with no kids).
 */
struct LeafNode {

  Node node;

  /**
   * The probability of a binding at this node belonging to the target concept.
   */
  Float probability;

  /**
   * The strength of the probability, based on the amount of data, including any
   * prior. In max-per-bag mode, this is the number of bags for which this leaf
   * is max (again plus any prior).
   *
   * This value might often (always currently?) be an integer value, but it's
   * not necessarily limited to integers, in the general sense.
   *
   * TODO What would strength mean in independent bindings mode?
   */
  Float strength;

};


/**
 * The node at the top of the tree.
 */
struct RootNode {

  Node node;

  Node* kid;

  Index nextId;

};


typedef enum {

  cnSplitYes,

  cnSplitNo,

  cnSplitErr,

  cnSplitCount,

} SplitIndex;


/**
 * A node for making decisions about where to propagate bindings.
 */
struct SplitNode {

  Node node;

  Node* kids[cnSplitCount];

  /**
   * Not owned by this node.
   */
  EntityFunction* function;

  /**
   * Owned by this node and dropped with this node.
   */
  Index* varIndices;

  /**
   * Owned by this node and dropped with this node if not null.
   */
  Predicate* predicate;

};


/**
 * A variable node for binding to entities.
 */
struct VarNode {

  Node node;

  Node* kid;

};


/**
 * A way to track the bindings arriving at leaves.
 */
struct LeafBindingBag {

  LeafBindingBag();

  BindingBag bindingBag;

  LeafNode* leaf;

  // TODO Float probability; // For cases when leaves aren't needed?

};


/**
 * For binding bags grouped by leaf.
 */
struct LeafBindingBagGroup {

  LeafBindingBagGroup();

  ~LeafBindingBagGroup();

  List<BindingBag> bindingBags;

  LeafNode* leaf;

  // TODO Float probability; // For cases when leaves aren't needed?

};


/**
 * A simple way of tracking the number of positive and negative bags assigned to
 * each leaf, such that calculating the overall score is easier. We have a
 * variety of ways in which we want to produce such counts.
 *
 * TODO Better even just to state the probability at the leaf instead of a link
 * TODO to the leaf itself? The link is a double-edged sword.
 */
struct LeafCount {

  LeafNode* leaf;

  Count negCount;

  Count posCount;

  // TODO Float probability; // For cases when leaves aren't needed?

};


/**
 * Creates a new binding bag list, with a ref count of 1.
 *
 * TODO Generalize ref-counted types?
 */
BindingBagList* cnBindingBagListCreate(void);


/**
 * Decrements refCount and disposes if at zero. If disposed, it disposes of the
 * bindingBags. It then frees the list itself, and sets the pointer to null.
 *
 * If the list pointer is null, then this function does nothing.
 *
 * TODO Generalize ref-counted types?
 */
void cnBindingBagListDrop(BindingBagList** list);


/**
 * Push on the bags as new empty bindings on the given list.
 */
bool cnBindingBagListPushBags(
  BindingBagList* bindingBags, const List<Bag>* bags
);


/**
 * Determines if the binding represented by the entities is valid (has no null
 * entity pointers).
 *
 * TODO If entities were a sized 2D grid, we could avoid the count here.
 */
bool cnBindingValid(Count entityCount, Entity* entities);


/**
 * Calculate the score directly from a list of counts of max bags at leaves.
 */
Float cnCountsLogMetric(List<LeafCount>* counts);


/**
 * Gives the address of the first, and one past the last, of the bags referenced
 * in the group list.
 *
 * Utility assumes the bags are all in continguous memory order.
 */
void cnLeafBindingBagGroupListLimits(
  const List<LeafBindingBagGroup>& groups, Bag** begin, Bag** end
);


void cnLeafBindingBagGroupListDispose(List<LeafBindingBagGroup>* groupList);


LeafNode* cnLeafNodeCreate(void);


/**
 * Disposes of the given node, not freeing it. However, all its descendants are
 * both disposed of and freed.
 *
 * If node is null, this function does nothing.
 *
 * TODO Rename to cnTreeDispose to clarify deep nature of disposal?
 */
void cnNodeDispose(Node* node);


/**
 * Disposes of and frees the given node and all its descendants.
 *
 * TODO Get rid of init/dispose on nodes from public interface?
 *
 * TODO Rename to cnTreeDrop to clarify deep nature of the drop?
 */
void cnNodeDrop(Node* node);


/**
 * Searches depth first under the given node to find the node with the given
 * id, returning a pointer to that node if found or else null.
 */
Node* cnNodeFindById(Node* node, Index id);


/**
 * Returns an array of pointers to kids.
 */
Node** cnNodeKids(Node* node);


Count cnNodeKidCount(Node* node);


/**
 * You don't usually need to call this directly.
 */
void cnNodeInit(Node* node, NodeType type);


/**
 * Stores all leaves under this node in the given list.
 */
bool cnNodeLeaves(Node* node, List<LeafNode*>* leaves);


/**
 * Given a node and a bindingBag, fills the leaf binding bags with those
 * arriving at leaves.
 *
 * The leaf binding bags are not cleared out.
 */
bool cnNodePropagateBindingBag(
  Node* node, BindingBag* bindingBag,
  List<LeafBindingBag>* leafBindingBags
);


/**
 * Propagates multiple binding bags to the leaves, storing a leaf bindings group
 * for each leaf.
 */
void cnNodePropagateBindingBags(
  Node* node, List<BindingBag>* bindingBags,
  List<LeafBindingBagGroup>* leafBindingBagGroups
);


/**
 * Replaces the kid at the index, if any, with the new kid, disposing of and
 * freeing the old one if any.
 */
void cnNodePutKid(Node* parent, Index k, Node* kid);


/**
 * Replaces the given old kid with the new kid, disposing of the old.
 */
void cnNodeReplaceKid(Node* oldKid, Node* newKid);


/**
 * Returns the node at the top of this tree if a root node and null otherwise.
 */
RootNode* cnNodeRoot(Node* node);


/**
 * Returns the number of var nodes in the tree above the given node.
 */
Count cnNodeVarDepth(Node* node);


void cnPointBagDispose(PointBag* pointBag);


void cnPointBagInit(PointBag* pointBag);


/**
 * Specify whether to add a leaf. That failing is the only reason root node
 * init would fail.
 *
 * Even if it fails, it is safe to call dispose of the root after calling this
 * function.
 */
bool cnRootNodeInit(RootNode* root, bool addLeaf);


SplitNode* cnSplitNodeCreate(bool addLeaves);


/**
 * Fills the point bags with points according to the given bindings and the
 * function at this node.
 *
 * Provide a NULL pointBag to have one allocated automatically. Instead, if a
 * point bag is provided to this function, it should not have any points in it
 * yet, and the matrix pointer should be null.
 *
 * TODO Guaranteed to be in the same order as the bindings, except for the
 * TODO duplicates issue.
 */
PointBag* cnSplitNodePointBag(
  SplitNode* split, BindingBag* bindingBag, PointBag* pointBag
);


/**
 * Fills the list of value bags with values according to the bindings and the
 * function at this node.
 *
 * TODO Guaranteed to be in the same order as the binding bags and bindings,
 * TODO except for the duplicates issue.
 */
bool cnSplitNodePointBags(
  SplitNode* split,
  List<BindingBag>* bindingBags,
  List<PointBag>* pointBags
);


/**
 * Copies this node and all under, except the bindings. (TODO Keep the models?)
 * Original references to bindings are kept to avoid excessive data copying.
 */
Node* cnTreeCopy(Node* node);


Float cnTreeLogMetric(RootNode* root, List<Bag>* bags);


/**
 * Count the number of positive and negative bags assigned as the max for each
 * leaf. This means that the bag only counts for the leaf with max probability
 * for all the bindings for that bag.
 *
 * No guarantee is made on the order of the counts coming out.
 */
bool cnTreeMaxLeafCounts(
  RootNode* root, List<LeafCount>& counts, List<Bag>* bags
);


/**
 * Given the leaf bag groups (with or without bindings represented), fill the
 * outbound list with indices of the binding bags, but retaining only those
 * which are max for each leaf.
 */
void treeMaxLeafBags(
  const List<LeafBindingBagGroup>& groupsIn, List<List<Index> >& groupsMaxOut
);


/**
 * Writes the tree to the given file, in a format readable by machines and
 * people.
 */
bool cnTreeWrite(RootNode* tree, FILE* file);


/**
 * Propagates a bags to the leaves, storing a leaf binding bag for each leaf.
 */
bool cnTreePropagateBag(
  RootNode* tree, Bag* bag, List<LeafBindingBag>* leafBindingBags
);


/**
 * Propagates multiple bags to the leaves, storing a leaf bindings group for
 * each leaf.
 *
 * TODO Expose generic grouper function?
 */
void cnTreePropagateBags(
  RootNode* tree, List<Bag>* bags,
  List<LeafBindingBagGroup>* leafBindingBagGroups
);


VarNode* cnVarNodeCreate(bool addLeaf);


}


#endif
