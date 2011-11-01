#include <assert.h>
#include <string.h>

#include "io.h"
#include "mat.h"
#include "tree.h"

using namespace std;


namespace concuno {


void cnLeafNodeInit(LeafNode* leaf);


bool cnLeafNodePropagateBindingBag(
  LeafNode* leaf, BindingBag* bindingBag,
  List<LeafBindingBag>* leafBindingBags
);


void cnRootNodeDispose(RootNode* root);


bool cnRootNodePropagateBindingBag(
  RootNode* root, BindingBag* bindingBag,
  List<LeafBindingBag>* leafBindingBags
);


void cnSplitNodeDispose(SplitNode* split);


bool cnSplitNodeInit(SplitNode* split, bool addLeaves);


bool cnSplitNodePropagateBindingBag(
  SplitNode* split, BindingBag* bindingBag,
  List<LeafBindingBag>* leafBindingBags
);


void cnVarNodeDispose(VarNode* var);


bool cnVarNodeInit(VarNode* var, bool addLeaf);


bool cnVarNodePropagateBindingBag(
  VarNode* var, BindingBag* bindingBag,
  List<LeafBindingBag>* leafBindingBags
);


void cnBindingBagDispose(BindingBag* bindingBag) {
  bindingBag->bag = NULL;
}


BindingBag::BindingBag(): bag(0), bindings(0), entityCount(0) {}


BindingBag::BindingBag(Bag* $bag, Count $entityCount):
    bag($bag), bindings($entityCount), entityCount($entityCount) {}


void BindingBag::init(Bag* bag, Count entityCount) {
  this->bag = bag;
  this->bindings.init(entityCount);
  this->entityCount = entityCount;
}


BindingBagList* cnBindingBagListCreate(void) {
  BindingBagList* list = new BindingBagList;
  if (list) {
    list->refCount = 1;
  }
  return list;
}


void cnBindingBagListDrop(BindingBagList** list) {
  BindingBagList* direct = *list;
  if (!direct) {
    return;
  }
  direct->refCount--;
  if (direct->refCount < 1) {
    if (direct->refCount < 0) {
      printf("Negative refCount: %ld\n", direct->refCount);
    }
    cnListEachBegin(&direct->bindingBags, BindingBag, bindingBag) {
      bindingBag->~BindingBag();
    } cnEnd;
    delete direct;
    *list = NULL;
  }
}


bool cnBindingBagListPushBags(
  BindingBagList* bindingBags, const List<Bag>* bags
) {
  // Preallocate all the space we need.
  BindingBag* bindingBag = reinterpret_cast<BindingBag*>(
    cnListExpandMulti(&bindingBags->bindingBags, bags->count)
  );
  if (!bindingBag) return false;
  // Now init each one.
  cnListEachBegin(bags, Bag, bag) {
    // TODO Are such inits guaranteed to work?
    bindingBag->init(bag, 0);
    // Have a fake empty binding. Note that this is abusive. Alternatives?
    bindingBag->bindings.count++;
    bindingBag++;
  } cnEnd;
  return true;
}


bool cnBindingValid(Count entityCount, Entity* entities) {
  Entity* entity = entities;
  Entity* end = entities + entityCount;
  for (; entity < end; entity++) {
    if (!*entity) {
      // Bogus/dummy binding.
      return false;
    }
  }
  return true;
}


Float cnCountsLogMetric(List<LeafCount>* counts) {
  Float score = 0;
  cnListEachBegin(counts, LeafCount, count) {
    // Add to the metric.
    if (count->posCount) {
      score += count->posCount * log(count->leaf->probability);
    }
    if (count->negCount) {
      score += count->negCount * log(1 - count->leaf->probability);
    }
  } cnEnd;
  return score;
}


bool cnGroupLeafBindingBags(
  List<LeafBindingBagGroup>* leafBindingBagGroups,
  List<LeafBindingBag>* leafBindingBags
) {
  LeafBindingBagGroup* group;
  bool result = false;

  // See if we already have space in the output list. This is needed only for
  // the first bag, but it's convenient just to leave in place for now.
  if (leafBindingBagGroups->count) {
    if (leafBindingBagGroups->count != leafBindingBags->count) {
      // TODO If not matching, we could at least check to see if the leaf ids
      // TODO in the groups match a subset of the leaves.
      cnErrTo(
        DONE, "Group count %ld != leaf count %ld.",
        leafBindingBagGroups->count, leafBindingBags->count
      );
    }
  } else {
    // Add empty binding bag groups. Should only apply for the first bag, at
    // most.
    if (!cnListExpandMulti(leafBindingBagGroups, leafBindingBags->count)) {
      cnErrTo(DONE, "No leaf binding bag groups.");
    }
    // Init each group.
    group =
      reinterpret_cast<LeafBindingBagGroup*>(leafBindingBagGroups->items);
    cnListEachBegin(leafBindingBags, LeafBindingBag, leafBindingBag) {
      new(group) LeafBindingBagGroup();
      group->leaf = leafBindingBag->leaf;
      group++;
    } cnEnd;
  }

  // Add the latest bindings to the groups.
  group = reinterpret_cast<LeafBindingBagGroup*>(leafBindingBagGroups->items);
  cnListEachBegin(leafBindingBags, LeafBindingBag, leafBindingBag) {
    // Sanity check. TODO Only needs done for the first bag!?!
    if (leafBindingBag->leaf != group->leaf) {
      cnErrTo(DONE, "Wrong leaf for binding bag group.");
    }
    // Push on the binding bag if it's not empty.
    if (leafBindingBag->bindingBag.bindings.count) {
      if (!cnListPush(
        &group->bindingBags, &leafBindingBag->bindingBag
      )) cnErrTo(DONE, "No space for binding bag.");
    }
    // On to the next leaf/group.
    group++;
  } cnEnd;

  // Clear out the leaf bindings, but don't dispose of their lists, since we
  // stole those for the group.
  cnListClear(leafBindingBags);

  // Winned.
  result = true;

  DONE:
  return result;
}


LeafBindingBag::LeafBindingBag(): leaf(NULL) {}


LeafBindingBagGroup::LeafBindingBagGroup(): leaf(NULL) {}


void cnLeafBindingBagGroupListLimits(
  List<LeafBindingBagGroup>* groups, Bag** begin, Bag** end
) {
  Bag* bags = NULL;
  Bag* bagsEnd = NULL;

  // Search the list.
  cnListEachBegin(groups, LeafBindingBagGroup, group) {
    cnListEachBegin(&group->bindingBags, BindingBag, bindingBag) {
      if (bindingBag->bag < bags || !bags) {
        bags = bindingBag->bag;
      }
      if (bindingBag->bag > bagsEnd || !bagsEnd) {
        bagsEnd = bindingBag->bag;
      }
    } cnEnd;
  } cnEnd;
  bagsEnd++;

  // Copy to outputs, as requested.
  if (begin) *begin = bags;
  if (end) *end = bagsEnd;
}


void cnLeafBindingBagGroupListDispose(List<LeafBindingBagGroup>* groups) {
  cnListEachBegin(groups, LeafBindingBagGroup, group) {
    // TODO Put this in ~LeafBindingBagGroup?
    cnListEachBegin(&group->bindingBags, BindingBag, bindingBag) {
      // TODO Automate in some form?
      bindingBag->~BindingBag();
    } cnEnd;
    group->~LeafBindingBagGroup();
  } cnEnd;
}


LeafNode* cnLeafNodeCreate(void) {
  LeafNode* leaf = cnAlloc(LeafNode, 1);
  if (leaf) {
    cnLeafNodeInit(leaf);
  }
  return leaf;
}


void cnLeafNodeDispose(LeafNode* node) {
  cnLeafNodeInit(node);
}


void cnNodeDispose(Node* node) {
  if (!node) {
    return;
  }
  // Disposes of child nodes in sub-dispose.
  switch (node->type) {
  case cnNodeTypeLeaf:
    cnLeafNodeDispose((LeafNode*)node);
    break;
  case cnNodeTypeSplit:
    cnSplitNodeDispose((SplitNode*)node);
    break;
  case cnNodeTypeRoot:
    cnRootNodeDispose((RootNode*)node);
    break;
  case cnNodeTypeVar:
    cnVarNodeDispose((VarNode*)node);
    break;
  default:
    printf("I don't handle type %u.\n", node->type);
    break;
  }
  // Base node disposal.
  cnNodeInit(node, node->type);
}


void cnNodeDrop(Node* node) {
  cnNodeDispose(node);
  free(node);
}


void cnLeafNodeInit(LeafNode* leaf) {
  cnNodeInit(&leaf->node, cnNodeTypeLeaf);
  leaf->probability = 0;
  leaf->strength = 0;
}


bool cnLeafNodePropagateBindingBag(
  LeafNode* leaf, BindingBag* bindingBag,
  List<LeafBindingBag>* leafBindingBags
) {
  bool result = false;
  LeafBindingBag* binding =
    reinterpret_cast<LeafBindingBag*>(cnListExpand(leafBindingBags));

  if (!binding) cnErrTo(DONE, "No leaf binding bag.");
  binding->leaf = leaf;
  binding->bindingBag.init(bindingBag->bag, bindingBag->entityCount);
  if (bindingBag->bindings.count) {
    if (!cnListPushAll(&binding->bindingBag.bindings, &bindingBag->bindings)) {
      // We couldn't really make the binding, so hide it.
      leafBindingBags->count--;
      cnErrTo(DONE, "No bindings for bag.");
    }
  }

  // Winned!
  result = true;

  DONE:
  return result;
}


void cnNodeAttachDeep(RootNode* root, Node* node) {
  Node** kid = cnNodeKids(node);
  Node** end = kid + cnNodeKidCount(node);
  node->id = root->nextId;
  root->nextId++;
  for (; kid < end; kid++) {
    if (*kid) cnNodeAttachDeep(root, *kid);
  }
}


Node* cnNodeFindById(Node* node, Index id) {
  if (node->id == id) {
    return node;
  } else {
    Node* found = NULL;
    Node** kid = cnNodeKids(node);
    Node** end = kid + cnNodeKidCount(node);
    for (; kid < end; kid++) {
      if ((found = cnNodeFindById(*kid, id))) {
        return found;
      }
    }
    return NULL;
  }
}


void cnNodeInit(Node* node, NodeType type) {
  node->id = -1;
  node->parent = NULL;
  node->type = type;
}


Node** cnNodeKids(Node* node) {
  switch (node->type) {
  case cnNodeTypeLeaf:
    return NULL;
  case cnNodeTypeRoot:
    return &((RootNode*)node)->kid;
  case cnNodeTypeSplit:
    return ((SplitNode*)node)->kids;
  case cnNodeTypeVar:
    return &((VarNode*)node)->kid;
  default:
    assert(false);
    return NULL; // to avoid warnings.
  }
}


Count cnNodeKidCount(Node* node) {
  switch (node->type) {
  case cnNodeTypeLeaf:
    return 0;
  case cnNodeTypeRoot:
  case cnNodeTypeVar:
    return 1;
  case cnNodeTypeSplit:
    return 3;
  default:
    assert(false);
    return 0; // to avoid warnings.
  }
}


bool cnNodeLeaves(Node* node, List<LeafNode*>* leaves) {
  Count count = cnNodeKidCount(node);
  Node** kid = cnNodeKids(node);
  Node** end = kid + count;
  for (; kid < end; kid++) {
    if ((*kid)->type == cnNodeTypeLeaf) {
      if (!cnListPush(leaves, kid)) {
        return false;
      }
    } else {
      if (!cnNodeLeaves(*kid, leaves)) {
        return false;
      }
    }
  }
  return true;
}


bool cnNodePropagateBindingBag(
  Node* node, BindingBag* bindingBag,
  List<LeafBindingBag>* leafBindingBags
) {
  // Sub-propagate. TODO Function pointers of some form instead of this?
  switch (node->type) {
  case cnNodeTypeLeaf:
    return cnLeafNodePropagateBindingBag(
      (LeafNode*)node, bindingBag, leafBindingBags
    );
  case cnNodeTypeSplit:
    return cnSplitNodePropagateBindingBag(
      (SplitNode*)node, bindingBag, leafBindingBags
    );
  case cnNodeTypeRoot:
    return cnRootNodePropagateBindingBag(
      (RootNode*)node, bindingBag, leafBindingBags
    );
  case cnNodeTypeVar:
    return cnVarNodePropagateBindingBag(
      (VarNode*)node, bindingBag, leafBindingBags
    );
  default:
    printf("I don't handle type %u for prop.\n", node->type);
    return false;
  }
}


void cnNodePropagateBindingBags(
  Node* node, List<BindingBag>* bindingBags,
  List<LeafBindingBagGroup>* leafBindingBagGroups
) {
  List<LeafBindingBag> leafBindingBags;

  // Propagate for each bag.
  cnListEachBegin(bindingBags, BindingBag, bindingBag) {
    // Propagate.
    if (!cnNodePropagateBindingBag(node, bindingBag, &leafBindingBags)) {
      throw Error("No propagate.");
    }
    // Group.
    if (!cnGroupLeafBindingBags(leafBindingBagGroups, &leafBindingBags)) {
      throw Error("No grouping.");
    }
  } cnEnd;
}


void cnNodePutKid(Node* parent, Index k, Node* kid) {
  Node** kids = cnNodeKids(parent);
  Node* old = kids[k];
  RootNode* root;
  if (old) {
    cnNodeDrop(old);
  }
  kids[k] = kid;
  kid->parent = parent;
  root = cnNodeRoot(parent);
  if (root) {
    cnNodeAttachDeep(root, kid);
  }
}


void cnNodeReplaceKid(Node* oldKid, Node* newKid) {
  Count k;
  Count kidCount = cnNodeKidCount(oldKid->parent);
  Node** kid = cnNodeKids(oldKid->parent);
  for (k = 0; k < kidCount; k++, kid++) {
    if (*kid == oldKid) {
      cnNodePutKid(oldKid->parent, k, newKid);
      break;
    }
  }
}


RootNode* cnNodeRoot(Node* node) {
  while (node->parent) {
    node = node->parent;
  }
  if (node->type == cnNodeTypeRoot) {
    return (RootNode*)node;
  }
  return NULL;
}


Count cnNodeVarDepth(Node* node) {
  Count depth = 0;
  while (node->parent) {
    node = node->parent;
    depth += node->type == cnNodeTypeVar;
  }
  return depth;
}


void cnPointBagDispose(PointBag* pointBag) {
  if (pointBag) {
    // TODO Split out point matrix init?
    free(pointBag->bindingPointIndices);
    free(pointBag->pointMatrix.points);
    cnPointBagInit(pointBag);
  }
}


void cnPointBagInit(PointBag* pointBag) {
  pointBag->bag = NULL;
  pointBag->bindingPointIndices = NULL;
  // TODO Split out point matrix dispose?
  pointBag->pointMatrix.valueCount = 0;
  pointBag->pointMatrix.valueSize = 0;
  pointBag->pointMatrix.points = NULL;
  pointBag->pointMatrix.pointCount = 0;
}


void cnRootNodeDispose(RootNode* root) {
  // Dispose of the kid.
  if (root->kid) {
    cnNodeDrop(root->kid);
  }
  // TODO Anything else special?
  cnRootNodeInit(root, false);
}


bool cnRootNodeInit(RootNode* root, bool addLeaf) {
  cnNodeInit(&root->node, cnNodeTypeRoot);
  root->node.id = 0;
  root->kid = NULL;
  root->nextId = 1;
  if (addLeaf) {
    LeafNode* leaf = cnLeafNodeCreate();
    if (!leaf) {
      return false;
    }
    cnNodePutKid(&root->node, 0, &leaf->node);
  }
  return true;
}


bool cnRootNodePropagateBindingBag(
  RootNode* root, BindingBag* bindingBag,
  List<LeafBindingBag>* leafBindingBags
) {
  if (root->kid) {
    return cnNodePropagateBindingBag(root->kid, bindingBag, leafBindingBags);
  }
  return true;
}


SplitNode* cnSplitNodeCreate(bool addLeaves) {
  SplitNode* split = cnAlloc(SplitNode, 1);
  if (!split) return NULL;
  if (!cnSplitNodeInit(split, addLeaves)) {
    free(split);
    return NULL;
  }
  return split;
}


void cnSplitNodeDispose(SplitNode* split) {
  // Dispose of the kids.
  Node** kid = split->kids;
  Node** end = split->kids + cnSplitCount;
  for (; kid < end; kid++) {
    cnNodeDrop(*kid);
  }
  if (split->predicate) {
    cnPredicateDrop(split->predicate);
    split->predicate = NULL;
  }
  free(split->varIndices);
  // TODO Anything else special?
  cnSplitNodeInit(split, false);
}


bool cnSplitNodeInit(SplitNode* split, bool addLeaves) {
  Node** kid;
  Node** end = split->kids + cnSplitCount;
  cnNodeInit(&split->node, cnNodeTypeSplit);
  // Init to null for convenience and safety.
  split->function = NULL;
  split->predicate = NULL;
  split->varIndices = NULL;
  for (kid = split->kids; kid < end; kid++) {
    *kid = NULL;
  }
  if (addLeaves) {
    Index k;
    for (kid = split->kids, k = 0; kid < end; kid++, k++) {
      LeafNode* leaf = cnLeafNodeCreate();
      if (!leaf) {
        cnNodeDispose(&split->node);
        return false;
      }
      cnNodePutKid(&split->node, k, &leaf->node);
    }
  }
  return true;
}


typedef struct cnSplitNodePointBag_Binding {

  /**
   * The actual thing we're sorting on.
   */
  Binding binding;

  /**
   * For reference to the original order.
   */
  Index index;

  /**
   * For access to arity and var indices.
   */
  SplitNode* split;

} cnSplitNodePointBag_Binding;

int cnSplitNodePointBag_compareBindings(const void* a, const void* b) {
  cnSplitNodePointBag_Binding* bindingA =
    reinterpret_cast<cnSplitNodePointBag_Binding*>(const_cast<void*>(a));
  cnSplitNodePointBag_Binding* bindingB =
    reinterpret_cast<cnSplitNodePointBag_Binding*>(const_cast<void*>(b));
  Index i;
  // Compare just on those indices we care about.
  for (i = 0; i < bindingA->split->function->inCount; i++) {
    Entity entityA = bindingA->binding[bindingA->split->varIndices[i]];
    Entity entityB = bindingB->binding[bindingB->split->varIndices[i]];
    if (entityA != entityB) return entityA > entityB ? 1 : -1;
  }
  // No differences found.
  return 0;
}

PointBag* cnSplitNodePointBag(
  SplitNode* split, BindingBag* bindingBag, PointBag* pointBag
) {
  Entity* args = NULL;
  bool makeOwnPointBag = !pointBag;
  List<cnSplitNodePointBag_Binding> splitBindings;
  void* values;
  Count varDepth;

  // Init first.
  varDepth = cnNodeVarDepth(&split->node);
  BindingBag uniqueBag(0, varDepth);
  // Make a point bag id needed, and init either way.
  if (makeOwnPointBag) {
    if (!(pointBag = cnAlloc(PointBag, 1))) {
      cnErrTo(FAIL, "No point bag.");
    }
  } else if (pointBag->pointMatrix.points) {
    // Failing to DONE on purpose here, so we don't free their data!
    cnErrTo(DONE,
      "Point bag has %ld points already!", pointBag->pointMatrix.pointCount
    );
  }
  cnPointBagInit(pointBag);
  pointBag->bag = bindingBag->bag;
  pointBag->pointMatrix.valueCount = split->function->outCount;
  pointBag->pointMatrix.valueSize = split->function->outType->size;

  // See if we have potential duplicate points. If so, combine them for
  // efficiency. This can speed things up a lot for deep areas of trees with
  // little pruning above.
  //
  // Purposely avoid the no-bindings case. It makes for some awkwardness below
  // and clearly won't be too slow, either.
  //
  // TODO Check instead whether the var indices use past the first n vars.
  // TODO Still need to keep a list of all bindings corresponding to each point.
  if (varDepth > split->function->inCount && bindingBag->bindings.count) {
    Index b;
    Index p;
    cnSplitNodePointBag_Binding* mostRecentlyKept;
    cnSplitNodePointBag_Binding* splitBinding;

    // Prepare a list of split bindings. Needed for sorting.
    if (!cnListExpandMulti(&splitBindings, bindingBag->bindings.count)) {
      cnErrTo(FAIL, "No split bindings.");
    }
    b = 0;
    splitBinding =
      reinterpret_cast<cnSplitNodePointBag_Binding*>(splitBindings.items);
    cnListEachBegin(&bindingBag->bindings, Entity, binding) {
      splitBinding->binding = binding;
      splitBinding->index = b++;
      splitBinding->split = split;
      splitBinding++;
    } cnEnd;

    // Track binding point indices, now that we know how many uniques we have.
    if (!(
      pointBag->bindingPointIndices = cnAlloc(Index, splitBindings.count)
    )) cnErrTo(FAIL, "No binding point indices.");

    // Sort them, and keep only uniques.
    // TODO Restore original order somehow (saving index for each)?
    // TODO Make my own sort (and for lists), allowing helper data:
    // TODO cnListSort(&bindings, split, cnSplitNodePointBag_compareBindings);
    qsort(
      splitBindings.items, splitBindings.count, splitBindings.itemSize,
      cnSplitNodePointBag_compareBindings
    );

    // Keep only the uniques.
    // TODO Generic 'cnUniques' function?
    uniqueBag.bag = bindingBag->bag;
    // Track the indices of each binding and of the most recent point.
    p = -1;
    // Remember the most recently kept split binding for easy comparison.
    mostRecentlyKept = NULL;
    cnListEachBegin(
      &splitBindings, cnSplitNodePointBag_Binding, splitBinding
    ) {
      // If it's first or different, keep it.
      if (
        !mostRecentlyKept ||
        cnSplitNodePointBag_compareBindings(splitBinding, mostRecentlyKept)
      ) {
        if (!cnListPush(&uniqueBag.bindings, splitBinding->binding)) {
          cnErrTo(FAIL, "No unique binding.");
        }
        // Increment our point index and remember this point for comparison.
        p++;
        mostRecentlyKept = splitBinding;
      }
      // Use the original binding index, not the sorted one.
      pointBag->bindingPointIndices[splitBinding->index] = p;
    } cnEnd;

    // Look at the one with uniques only.
    bindingBag = &uniqueBag;
  }

  // Prepare space for points.
  // Null (dummy bindings) will yield NaN as needed, so every binding yields a
  // point.
  // TODO What about for non-float outputs???
  pointBag->pointMatrix.pointCount = bindingBag->bindings.count;
  if (!(pointBag->pointMatrix.points = reinterpret_cast<Float*>(malloc(
    pointBag->pointMatrix.pointCount *
    pointBag->pointMatrix.valueCount * pointBag->pointMatrix.valueSize
  )))) cnErrTo(FAIL, "No point matrix.");
  // Put args on the stack.
  if (!(args = cnStackAllocOf(void*, split->function->inCount))) {
    cnErrTo(FAIL, "No args.");
  }

  // Calculate the points.
  values = pointBag->pointMatrix.points;
  cnListEachBegin(&bindingBag->bindings, Entity, entities) {
    // Gather the arguments.
    Index a;
    for (a = 0; a < split->function->inCount; a++) {
      args[a] = entities[split->varIndices[a]];
    }
    // Call the function.
    // TODO Check for errors once we provide such things.
    split->function->get(split->function, args, values);
    // Move to the next vector.
    values =
      ((char*)values) +
      pointBag->pointMatrix.valueCount * pointBag->pointMatrix.valueSize;
  } cnEnd;

  // We winned.
  goto DONE;

  FAIL:
  // We won't be needing this anymore.
  cnPointBagDispose(pointBag);
  if (makeOwnPointBag) {
    // Free it if we made it.
    free(pointBag);
  }
  pointBag = NULL;

  DONE:
  cnStackFree(args);
  return pointBag;
}


bool cnSplitNodePointBags(
  SplitNode* split,
  List<BindingBag>* bindingBags,
  List<PointBag>* pointBags
) {
  PointBag* pointBag;
  bool result = false;
  Count validBindingsCount = 0;

  // Init first for safety.
  if (pointBags->count) {
    cnErrTo(FAIL, "Start with empty pointBags, not %ld.", pointBags->count);
  }
  if (!cnListExpandMulti(pointBags, bindingBags->count)) {
    cnErrTo(FAIL, "No point bags.");
  }
  pointBag = reinterpret_cast<PointBag*>(pointBags->items);
  cnListEachBegin(bindingBags, BindingBag, bindingBag) {
    // Clear out each point bag for later filling.
    cnPointBagInit(pointBag);
    // Null (dummy bindings) will yield NaN as needed.
    // TODO What about for non-float outputs???
    // Next bag.
    pointBag++;
  } cnEnd;

  // Now build the values.
  // TODO Actually, make an array of all the args and eliminate the duplicates!
  // TODO Dupes can come from bindings where only the non-args are unique.
  pointBag = reinterpret_cast<PointBag*>(pointBags->items);
  cnListEachBegin(bindingBags, BindingBag, bindingBag) {
    if (!cnSplitNodePointBag(split, bindingBag, pointBag)) {
      cnErrTo(FAIL, "No point bag.");
    }
    validBindingsCount += pointBag->pointMatrix.pointCount;
    pointBag++;
  } cnEnd;
  printf("Points built: %ld\n", validBindingsCount);

  // It all worked.
  result = true;
  goto DONE;

  FAIL:
  cnListEachBegin(pointBags, PointBag, pointBag) {
    free(pointBag->pointMatrix.points);
  } cnEnd;
  cnListClear(pointBags);

  DONE:
  return result;
}


bool cnSplitNodePropagateBindingBag(
  SplitNode* split, BindingBag* bindingBag,
  List<LeafBindingBag>* leafBindingBags
) {
  bool allToErr = false;
  Index b;
  BindingBag bagsOut[cnSplitCount];
  Index p;
  Float* point;
  PointBag* pointBag = NULL;
  Float* pointsEnd;
  BindingBag** pointBindingBagOuts = NULL;
  bool result = false;
  Node* yes = split->kids[cnSplitYes];
  Node* no = split->kids[cnSplitNo];
  Node* err = split->kids[cnSplitErr];

  // Init first.
  for (
    Index splitIndex = 0;
    splitIndex < static_cast<Index>(cnSplitCount);
    splitIndex++
  ) {
    (bagsOut + splitIndex)->init(bindingBag->bag, bindingBag->entityCount);
  }

  // Check error cases.
  if (!(yes && no && err)) {
    result = true;
    goto DONE;
  }
  if (!(split->function && split->predicate)) {
    // No way to say where to go, so go to error node.
    allToErr = true;
    goto PROPAGATE;
  }

  // Gather up points.
  if (!(pointBag = cnSplitNodePointBag(split, bindingBag, NULL))) {
    cnErrTo(DONE, "No point bag.");
  }

  // Prepare space for point assessment. This makes easier the case where we
  // have multiple bindings for a single point, and it shouldn't be horribly
  // expensive for the common case.
  if (!(
    pointBindingBagOuts =
      cnAlloc(BindingBag*, pointBag->pointMatrix.pointCount)
  )) cnErrTo(DONE, "No point binding bag assignments.");

  // Go through the points.
  p = 0;
  point = pointBag->pointMatrix.points;
  pointsEnd = point +
    pointBag->pointMatrix.pointCount * pointBag->pointMatrix.valueCount;
  for (; point < pointsEnd; point += pointBag->pointMatrix.valueCount) {
    // Check for error.
    bool allGood = true;
    Float* value = point;
    Float* pointEnd = point + split->function->outCount;
    // TODO Let the function tell us instead of explicitly checking bad?
    for (value = point; value < pointEnd; value++) {
      if (cnIsNaN(*value)) {
        allGood = false;
        break;
      }
    }

    // Choose the bag this point goes to.
    SplitIndex splitIndex;
    if (!allGood) {
      splitIndex = cnSplitErr;
    } else {
      splitIndex = split->predicate->evaluate(split->predicate, point) ?
        cnSplitYes : cnSplitNo;
      // I hack -1 (turned unsigned) into this for errors at this point.
      if (splitIndex >= cnSplitCount) cnErrTo(DONE, "Bad evaluate.");
    }

    // Remember this choice for later, when we go through the bindings.
    pointBindingBagOuts[p++] = bagsOut + splitIndex;
  }

  // Now actually assign the bindings themselves.
  b = 0;
  cnListEachBegin(&bindingBag->bindings, Binding, bindingIn) {
    // The point index either comes from the mapping or is just the the binding
    // index itself.
    Index p = pointBag->bindingPointIndices ?
      pointBag->bindingPointIndices[b] : b;
    if (!cnListPush(&pointBindingBagOuts[p]->bindings, bindingIn)) {
      cnErrTo(DONE, "No pushed binding.");
    }
    b++;
  } cnEnd;

  PROPAGATE:
  // Now that we have the bindings split, push them down.
  for (
    Index splitIndex = 0;
    splitIndex < static_cast<Index>(cnSplitCount);
    splitIndex++
  ) {
    // Use the original instead of an empty if going to allToErr.
    BindingBag* bagOut =
      allToErr && splitIndex == static_cast<Index>(cnSplitErr) ?
        bindingBag : bagsOut + splitIndex;
    if (!cnNodePropagateBindingBag(
      split->kids[splitIndex], bagOut, leafBindingBags
    )) cnErrTo(DONE, "Split kid prop failed. Now what?\n");
  }

  // Winned!
  result = true;

  DONE:
  free(pointBindingBagOuts);
  if (pointBag) {
    cnPointBagDispose(pointBag);
    free(pointBag);
  }
  for (
    Index splitIndex = 0;
    splitIndex < static_cast<Index>(cnSplitCount);
    splitIndex++
  ) {
    (bagsOut + splitIndex)->~BindingBag();
  }
  return result;
}


bool cnTreeCopy_handleSplit(SplitNode* source, SplitNode* copy) {
  if (source->varIndices) {
    // TODO Assert source->function?
    copy->varIndices = cnAlloc(Index, source->function->inCount);
    if (!copy->varIndices) return false;
    if (copy->varIndices) {
      memcpy(
        copy->varIndices, source->varIndices,
        source->function->inCount * sizeof(Index)
      );
    }
  }
  if (source->predicate) {
    copy->predicate = cnPredicateCopy(source->predicate);
    if (!copy->predicate) {
      // Failed!
      if (copy->varIndices) {
        // Clean up this, too.
        free(copy->varIndices);
      }
      return false;
    }
  }
  return true;
}

Node* cnTreeCopy(Node* node) {
  bool anyFailed = false;
  Node* copy = NULL;
  Node** end;
  Node** kid;
  // TODO Extract node size to separate function?
  Count nodeSize;
  switch (node->type) {
  case cnNodeTypeLeaf:
    nodeSize = sizeof(LeafNode);
    break;
  case cnNodeTypeRoot:
    nodeSize = sizeof(RootNode);
    break;
  case cnNodeTypeSplit:
    nodeSize = sizeof(SplitNode);
    break;
  case cnNodeTypeVar:
    nodeSize = sizeof(VarNode);
    break;
  default:
    cnErrTo(DONE, "No copy for type %u.", node->type);
  }
  copy = reinterpret_cast<Node*>(malloc(nodeSize));
  if (!copy) cnErrTo(DONE, "Failed to allocate copy.");
  memcpy(copy, node, nodeSize);
  // Recursively copy.
  kid = cnNodeKids(copy);
  end = kid + cnNodeKidCount(copy);
  for (; kid < end; kid++) {
    *kid = anyFailed ? NULL : cnTreeCopy(*kid);
    if (!*kid) {
      anyFailed = true;
      continue;
    }
    (*kid)->parent = copy;
  }
  if (node->type == cnNodeTypeSplit) {
    // Custom handling for split nodes.
    anyFailed = !cnTreeCopy_handleSplit(
      reinterpret_cast<SplitNode*>(node), reinterpret_cast<SplitNode*>(copy)
    );
  }
  if (anyFailed) {
    printf("Failed to copy kids!\n");
    // Kids should be either copies or null at this point.
    cnNodeDrop(copy);
    copy = NULL;
  }
  // TODO Any other custom parts!?!
  DONE:
  return copy;
}


Float cnTreeLogMetric(RootNode* root, List<Bag>* bags) {
  List<LeafCount> counts;
  // Count positives and negatives in each leaf.
  if (!cnTreeMaxLeafCounts(root, &counts, bags)) throw Error("No counts.");
  // Calculate the score.
  return cnCountsLogMetric(&counts);
}


int cnTreeMaxLeafCounts_compareLeafProbsDown(const void* a, const void* b) {
  LeafNode* leafA = ((LeafBindingBagGroup*)a)->leaf;
  LeafNode* leafB = ((LeafBindingBagGroup*)b)->leaf;
  return
    // Smaller is higher here for downward sort.
    leafA->probability < leafB->probability ? 1 :
    leafA->probability == leafB->probability ? 0 :
    -1;
}

bool cnTreeMaxLeafCounts(
  RootNode* root, List<LeafCount>* counts, List<Bag>* bags
) {
  List<LeafBindingBagGroup> groups;
  bool result = false;

  // First propagate bags, and gather leaves.
  // TODO Bindings out of leaves, then extra function to get the lists.
  cnTreePropagateBags(root, bags, &groups);

  // Prepare space to track which bags have been used up already.
  // TODO Could be bit-efficient, since bools, but don't stress it.
  vector<bool> bagsUsed;
  bagsUsed.resize(bags->count, false);

  // Prepare space for counts at one go for efficiency.
  cnListClear(counts);
  if (!cnListExpandMulti(counts, groups.count)) {
    cnErrTo(DONE, "No space for counts.");
  }

  // Sort the leaves down by probability. Not too many leaves, so no worries.
  qsort(
    groups.items, groups.count, groups.itemSize,
    cnTreeMaxLeafCounts_compareLeafProbsDown
  );

  // Loop through leaves from max prob to min, count bags and marking them used
  // along the way.
  cnListEachBegin(&groups, LeafBindingBagGroup, group) {
    // Init the count.
    // TODO Loop providing item and index?
    LeafCount* count = reinterpret_cast<LeafCount*>(
      cnListGet(counts, group - (LeafBindingBagGroup*)groups.items)
    );
    count->leaf = group->leaf;
    count->negCount = 0;
    count->posCount = 0;
    // Loop through bags, if any.
    cnListEachBegin(&group->bindingBags, BindingBag, bindingBag) {
      // TODO This subtraction only works if bags is an array of Bag and not
      // TODO Bag*, because we'd otherwise have no reference point. Should I
      // TODO consider explicit ids/indices at some point?
      Index bagIndex = bindingBag->bag - (Bag*)bags->items;
      if (bagsUsed[bagIndex]) continue;
      // Add to the proper count.
      if (bindingBag->bag->label) {
        count->posCount++;
      } else {
        count->negCount++;
      }
      bagsUsed[bagIndex] = true;
    } cnEnd;
  } cnEnd;
  // All done!
  result = true;

  DONE:
  cnLeafBindingBagGroupListDispose(&groups);
  return result;
}


/**
 * For retaining original index.
 */
struct cnTreeMaxLeafBags_IndexedGroup {
  LeafBindingBagGroup* group;
  Index index;
};

int cnTreeMaxLeafBags_compareLeafProbsDown(const void* a, const void* b) {
  LeafNode* leafA = ((cnTreeMaxLeafBags_IndexedGroup*)a)->group->leaf;
  LeafNode* leafB = ((cnTreeMaxLeafBags_IndexedGroup*)b)->group->leaf;
  return
    // Smaller is higher here for downward sort.
    leafA->probability < leafB->probability ? 1 :
    leafA->probability == leafB->probability ? 0 :
    -1;
}

bool cnTreeMaxLeafBags(
  List<LeafBindingBagGroup>* groupsIn,
  List<List<Index> >* groupsMaxOut
) {
  Index b;
  Count bagCount;
  Bag* bags = NULL;
  Bag* bagsEnd = NULL;
  Index g;
  cnTreeMaxLeafBags_IndexedGroup* indexedGroupsIn = NULL;
  bool result = false;

  // TODO Can I generify this enough to unify max bags and max counts somewhat?
  // TODO That is, with callbacks in the later loops?
  vector<bool> bagsUsed;

  // Prepare the groups out.
  if (groupsMaxOut->count) {
    // Just clear the old out for convience. Could error. TODO Document!
    cnListEachBegin(groupsMaxOut, List<Index>, indices) {
      indices->dispose();
    } cnEnd;
    cnListClear(groupsMaxOut);
  }
  // Allocate space right away.
  if (!cnListExpandMulti(groupsMaxOut, groupsIn->count)) {
    cnErrTo(FAIL, "No groups out.");
  }
  // And init for safety.
  cnListEachBegin(groupsMaxOut, List<Index>, indices) {
    indices->init();
  } cnEnd;

  // Make a list of groups with their indices.
  if (!(
    indexedGroupsIn = cnAlloc(cnTreeMaxLeafBags_IndexedGroup, groupsIn->count)
  )) cnErrTo(FAIL, "No indexed groups.");
  g = 0;
  cnListEachBegin(groupsIn, LeafBindingBagGroup, groupIn) {
    indexedGroupsIn[g].group = groupIn;
    indexedGroupsIn[g].index = g;
    g++;
  } cnEnd;

  // Sort the leaves down by probability.
  qsort(
    indexedGroupsIn, groupsIn->count, sizeof(cnTreeMaxLeafBags_IndexedGroup),
    cnTreeMaxLeafBags_compareLeafProbsDown
  );

  // Init which bags used. First, we need to find how many and where they start.
  cnLeafBindingBagGroupListLimits(groupsIn, &bags, &bagsEnd);
  bagCount = bagsEnd - bags;
  bagsUsed.resize(bagCount, false);

  // Loop through leaves from max prob to min, count bags and marking them used
  // along the way.
  for (g = 0; g < groupsIn->count; g++) {
    // Group in and out, where out retains original order.
    cnTreeMaxLeafBags_IndexedGroup* groupIn = indexedGroupsIn + g;
    List<Index>* indices = reinterpret_cast<List<Index>*>(
      cnListGet(groupsMaxOut, groupIn->index)
    );

    // Loop through bags.
    b = 0;
    cnListEachBegin(&groupIn->group->bindingBags, BindingBag, bindingBag) {
      // TODO This subtraction only works if bags is an array of Bag and not
      // TODO Bag*, because we'd otherwise have no reference point. Should I
      // TODO consider explicit ids/indices at some point?
      Index bagIndex = bindingBag->bag - bags;
      if (!bagsUsed[bagIndex]) {
        if (!cnListPush(indices, &b)) {
          cnErrTo(FAIL, "No binding bag ref.")
        }
        bagsUsed[bagIndex] = true;
      }
      b++;
    } cnEnd;
  }

  // All done!
  result = true;
  goto DONE;

  FAIL:
  // Clear these out on fail.
  cnListEachBegin(groupsMaxOut, List<Index>, indices) {
    indices->dispose();
  } cnEnd;
  cnListClear(groupsMaxOut);

  DONE:
  free(indexedGroupsIn);
  return result;
}


bool cnTreePropagateBag(
  RootNode* tree, Bag* bag, List<LeafBindingBag>* leafBindingBags
) {
  // Init an empty binding bag (with an abusive 0-sized item), and propagate it.
  BindingBag bindingBag(bag, 0);
  bindingBag.bindings.count = 1;
  return cnNodePropagateBindingBag(&tree->node, &bindingBag, leafBindingBags);
}


void cnTreePropagateBags(
  RootNode* tree, List<Bag>* bags,
  List<LeafBindingBagGroup>* leafBindingBagGroups
) {
  // Propagate for each bag.
  List<LeafBindingBag> leafBindingBags;
  cnListEachBegin(bags, Bag, bag) {
    // Propagate.
    if (!cnTreePropagateBag(tree, bag, &leafBindingBags)) {
      throw Error("No propagate.");
    }
    // Group.
    if (!cnGroupLeafBindingBags(leafBindingBagGroups, &leafBindingBags)) {
      throw Error("No grouping.");
    }
  } cnEnd;
}

bool cnTreeWrite_leaf(LeafNode* leaf, FILE* file, String* indent);
bool cnTreeWrite_root(RootNode* root, FILE* file, String* indent);
bool cnTreeWrite_split(SplitNode* split, FILE* file, String* indent);
bool cnTreeWrite_var(VarNode* var, FILE* file, String* indent);

bool cnTreeWrite_any(Node* node, FILE* file, String* indent) {
  Count kidCount;
  bool result = false;

  fprintf(file, "{\n");
  if (!cnIndent(indent)) cnErrTo(DONE, "No indent.");

  // Handle each node type.
  switch (node->type) {
  case cnNodeTypeLeaf:
    if (!cnTreeWrite_leaf((LeafNode*)node, file, indent)) {
      cnErrTo(DONE, "No leaf.");
    }
    break;
  case cnNodeTypeRoot:
    if (!cnTreeWrite_root((RootNode*)node, file, indent)) {
      cnErrTo(DONE, "No root.");
    }
    break;
  case cnNodeTypeSplit:
    if (!cnTreeWrite_split((SplitNode*)node, file, indent)) {
      cnErrTo(DONE, "No split.");
    }
    break;
  case cnNodeTypeVar:
    if (!cnTreeWrite_var((VarNode*)node, file, indent)) {
      cnErrTo(DONE, "No var.");
    }
    break;
  default:
    cnErrTo(DONE, "No such type: %u", node->type);
  }

  // Handle the kids, if any.
  kidCount = cnNodeKidCount(node);
  if (kidCount) {
    Index k;
    Node** kids = cnNodeKids(node);
    // TODO Check errors.
    // Wouldn't it be great if JSON didn't require quotation marks on keys?
    fprintf(file, "%s\"kids\": [", cnStr(indent));
    for (k = 0; k < kidCount; k++) {
      if (!cnTreeWrite_any(kids[k], file, indent)) {
        cnErrTo(CLOSE, "No kid write.");
      }
      if (k < kidCount - 1) {
        // Stupid no trailing JSON commas.
        fprintf(file, ", ");
      }
    }
    fprintf(file, "]\n");
  }

  // Winned.
  result = true;

  CLOSE:
  // Need to dedent if indented.
  cnDedent(indent);
  // Close the object.
  fprintf(file, "%s}", cnStr(indent));

  DONE:
  return result;
}

bool cnTreeWrite_leaf(LeafNode* leaf, FILE* file, String* indent) {
  // TODO Check error states?
  fprintf(file, "%s\"type\": \"Leaf\",\n", cnStr(indent));
  fprintf(file, "%s\"probability\": %lg,\n", cnStr(indent), leaf->probability);
  fprintf(file, "%s\"strength\": %lg\n", cnStr(indent), leaf->strength);
  return true;
}

bool cnTreeWrite_root(RootNode* root, FILE* file, String* indent) {
  // TODO Check error states?
  fprintf(file, "%s\"type\": \"Root\",\n", cnStr(indent));
  return true;
}

bool cnTreeWrite_split(SplitNode* split, FILE* file, String* indent) {
  bool result = false;

  // TODO Check error states?
  fprintf(file, "%s\"type\": \"Split\",\n", cnStr(indent));
  if (split->function) {
    Count arity = split->function->inCount;
    Index i;

    // Function name.
    // TODO Escape strings (like function name)!!
    fprintf(
      file, "%s\"function\": \"%s\",\n",
      cnStr(indent), split->function->name.c_str()
    );

    // Var indices.
    fprintf(file, "%s\"vars\": [", cnStr(indent));
    for (i = 0; i < arity; i++) {
      if (i) fprintf(file, ", ");
      fprintf(file, "%ld", split->varIndices[i]);
    }
    fprintf(file, "],\n");

    fprintf(file, "%s\"predicate\": ", cnStr(indent));
    if (!cnPredicateWrite(split->predicate, file, indent)) {
      cnErrTo(DONE, "No predicate write.");
    }
    fprintf(file, ",\n");
  }

  // Winned!
  result = true;

  DONE:
  return result;
}

bool cnTreeWrite_var(VarNode* var, FILE* file, String* indent) {
  // TODO Check error states?
  fprintf(file, "%s\"type\": \"Var\",\n", cnStr(indent));
  return true;
}

bool cnTreeWrite(RootNode* tree, FILE* file) {
  String indent;
  bool result = false;
  // TODO In future stream abstraction, store indent level directly.
  result = cnTreeWrite_any(&tree->node, file, &indent);
  return result;
}


VarNode* cnVarNodeCreate(bool addLeaf) {
  VarNode* var = cnAlloc(VarNode, 1);
  if (!var) return NULL;
  if (!cnVarNodeInit(var, addLeaf)) {
    free(var);
    return NULL;
  }
  return var;
}


void cnVarNodeDispose(VarNode* var) {
  // Dispose of the kid.
  if (var->kid) {
    cnNodeDrop(var->kid);
  }
  // TODO Anything else special?
  cnVarNodeInit(var, false);
}


bool cnVarNodeInit(VarNode* var, bool addLeaf) {
  cnNodeInit(&var->node, cnNodeTypeVar);
  var->kid = NULL;
  if (addLeaf) {
    LeafNode* leaf = cnLeafNodeCreate();
    if (!leaf) {
      return false;
    }
    cnNodePutKid(&var->node, 0, &leaf->node);
  }
  return true;
}


bool cnVarNodePropagate_pushBinding(
  Entity* entitiesIn, BindingBag* bindingBagIn,
  Entity entityOut, BindingBag* bindingBagOut, Count* bindingsOutCount
) {
  Entity* entitiesOut =
    reinterpret_cast<Entity*>(cnListExpand(&bindingBagOut->bindings));
  if (!entitiesOut) return false;
  (*bindingsOutCount)++;
  // For the zero length arrays, I'm not sure if memcpy from null is
  // okay, so check that first.
  if (entitiesIn) {
    memcpy(
      entitiesOut, entitiesIn, bindingBagIn->entityCount * sizeof(void*)
    );
  }
  // Put the new one on.
  entitiesOut[bindingBagIn->entityCount] = entityOut;
  return true;
}


bool cnVarNodePropagateBindingBag(
  VarNode* var, BindingBag* bindingBag,
  List<LeafBindingBag>* leafBindingBags
) {
  Index b;
  BindingBag bindingBagOut(bindingBag->bag, bindingBag->entityCount + 1);
  Count bindingsOutCount = 0;
  bool result = false;

  // Do we have anything to do?
  if (!var->kid) goto DONE;

  // Find each binding to expand.
  // Use custom looping because of our abusive 2D-ish array.
  // TODO Normal looping really won't work here???
  for (b = 0; b < bindingBag->bindings.count; b++) {
    Entity* entitiesIn =
      reinterpret_cast<Entity*>(cnListGet(&bindingBag->bindings, b));
    bool anyLeft = false;
    // Figure out if we have constrained options.
    List<Entity>* entitiesOut = reinterpret_cast<List<Entity>*>(
      cnListGet(&bindingBag->bag->participantOptions, bindingBag->entityCount)
    );
    if (!(entitiesOut && entitiesOut->count)) {
      // No constraints after all.
      entitiesOut = bindingBag->bag->entities;
    }
    // Find the entities to add on.
    cnListEachBegin(entitiesOut, Entity, entityOut) {
      bool found = false;
      Entity* entityIn = entitiesIn;
      Entity* entitiesInEnd = entityIn + bindingBag->entityCount;
      // TODO Loop okay since usually few vars?
      for (; entityIn < entitiesInEnd; entityIn++) {
        if (*entityIn == *entityOut) {
          // Already used.
          found = true;
          break;
        }
      }
      if (!found) {
        // Didn't find it. Push a new binding with the new entity.
        anyLeft = true;
        if (!cnVarNodePropagate_pushBinding(
          entitiesIn, bindingBag,
          *entityOut, &bindingBagOut, &bindingsOutCount
        )) {
          // TODO Fail out!
          printf("Failed to push binding!\n");
        }
      }
    } cnEnd;
    if (!anyLeft) {
      // Push a dummy binding for later errors since no entities remained.
      if (!cnVarNodePropagate_pushBinding(
        entitiesIn, bindingBag, NULL, &bindingBagOut, &bindingsOutCount
      )) {
        // TODO Fail out!
        printf("Failed to push dummy binding!\n");
      }
    }
  }

  if (!cnNodePropagateBindingBag(var->kid, &bindingBagOut, leafBindingBags)) {
    cnErrTo(DONE, "No propagate.");
  }

  // Winned!
  result = true;

  DONE:
  return result;
}


}
