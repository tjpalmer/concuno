#include <assert.h>
#include <string.h>

#include "io.h"
#include "mat.h"
#include "tree.h"


namespace concuno {


void cnLeafNodeInit(cnLeafNode* leaf);


bool cnLeafNodePropagateBindingBag(
  cnLeafNode* leaf, cnBindingBag* bindingBag,
  cnList(cnLeafBindingBag)* leafBindingBags
);


void cnRootNodeDispose(cnRootNode* root);


bool cnRootNodePropagateBindingBag(
  cnRootNode* root, cnBindingBag* bindingBag,
  cnList(cnLeafBindingBag)* leafBindingBags
);


void cnSplitNodeDispose(cnSplitNode* split);


bool cnSplitNodeInit(cnSplitNode* split, bool addLeaves);


bool cnSplitNodePropagateBindingBag(
  cnSplitNode* split, cnBindingBag* bindingBag,
  cnList(cnLeafBindingBag)* leafBindingBags
);


void cnVarNodeDispose(cnVarNode* var);


bool cnVarNodeInit(cnVarNode* var, bool addLeaf);


bool cnVarNodePropagateBindingBag(
  cnVarNode* var, cnBindingBag* bindingBag,
  cnList(cnLeafBindingBag)* leafBindingBags
);


void cnBindingBagDispose(cnBindingBag* bindingBag) {
  bindingBag->bag = NULL;
  cnListDispose(&bindingBag->bindings);
}


void cnBindingBagInit(
  cnBindingBag* bindingBag, Bag* bag, cnCount entityCount
) {
  bindingBag->bag = bag;
  bindingBag->entityCount = entityCount;
  cnListInit(&bindingBag->bindings, entityCount * sizeof(void*));
}


cnBindingBagList* cnBindingBagListCreate(void) {
  cnBindingBagList* list = cnAlloc(cnBindingBagList, 1);
  if (list) {
    cnListInit(&list->bindingBags, sizeof(cnBindingBag));
    list->refCount = 1;
  }
  return list;
}


void cnBindingBagListDrop(cnBindingBagList** list) {
  cnBindingBagList* direct = *list;
  if (!direct) {
    return;
  }
  direct->refCount--;
  if (direct->refCount < 1) {
    if (direct->refCount < 0) {
      printf("Negative refCount: %ld\n", direct->refCount);
    }
    cnListEachBegin(&direct->bindingBags, cnBindingBag, bindingBag) {
      cnBindingBagDispose(bindingBag);
    } cnEnd;
    cnListDispose(&direct->bindingBags);
    free(direct);
    *list = NULL;
  }
}


bool cnBindingBagListPushBags(
  cnBindingBagList* bindingBags, const cnList(Bag)* bags
) {
  // Preallocate all the space we need.
  cnBindingBag* bindingBag = reinterpret_cast<cnBindingBag*>(
    cnListExpandMulti(&bindingBags->bindingBags, bags->count)
  );
  if (!bindingBag) return false;
  // Now init each one.
  cnListEachBegin(bags, Bag, bag) {
    cnBindingBagInit(bindingBag, bag, 0);
    // Have a fake empty binding. Note that this is abusive. Alternatives?
    bindingBag->bindings.count++;
    bindingBag++;
  } cnEnd;
  return true;
}


bool cnBindingValid(cnCount entityCount, cnEntity* entities) {
  cnEntity* entity = entities;
  cnEntity* end = entities + entityCount;
  for (; entity < end; entity++) {
    if (!*entity) {
      // Bogus/dummy binding.
      return false;
    }
  }
  return true;
}


cnFloat cnCountsLogMetric(cnList(cnLeafCount)* counts) {
  cnFloat score = 0;
  cnListEachBegin(counts, cnLeafCount, count) {
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
  cnList(cnLeafBindingBagGroup)* leafBindingBagGroups,
  cnList(cnLeafBindingBag)* leafBindingBags
) {
  cnLeafBindingBagGroup* group;
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
      reinterpret_cast<cnLeafBindingBagGroup*>(leafBindingBagGroups->items);
    cnListEachBegin(leafBindingBags, cnLeafBindingBag, leafBindingBag) {
      cnListInit(&group->bindingBags, sizeof(cnBindingBag));
      group->leaf = leafBindingBag->leaf;
      group++;
    } cnEnd;
  }

  // Add the latest bindings to the groups.
  group = reinterpret_cast<cnLeafBindingBagGroup*>(leafBindingBagGroups->items);
  cnListEachBegin(leafBindingBags, cnLeafBindingBag, leafBindingBag) {
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


void cnLeafBindingBagDispose(cnLeafBindingBag* leafBindingBag) {
  cnBindingBagDispose(&leafBindingBag->bindingBag);
  leafBindingBag->leaf = NULL;
}


void cnLeafBindingBagGroupListLimits(
  cnList(cnLeafBindingBagGroup)* groups, Bag** begin, Bag** end
) {
  Bag* bags = NULL;
  Bag* bagsEnd = NULL;

  // Search the list.
  cnListEachBegin(groups, cnLeafBindingBagGroup, group) {
    cnListEachBegin(&group->bindingBags, cnBindingBag, bindingBag) {
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


void cnLeafBindingBagGroupListDispose(
  cnList(cnLeafBindingBagGroup)* groups
) {
  cnListEachBegin(groups, cnLeafBindingBagGroup, group) {
    // TODO cnLeafBindingBagGroupDispose?
    cnListEachBegin(&group->bindingBags, cnBindingBag, bindingBag) {
      cnBindingBagDispose(bindingBag);
    } cnEnd;
    cnListDispose(&group->bindingBags);
  } cnEnd;
  cnListDispose(groups);
}


cnLeafNode* cnLeafNodeCreate(void) {
  cnLeafNode* leaf = cnAlloc(cnLeafNode, 1);
  if (leaf) {
    cnLeafNodeInit(leaf);
  }
  return leaf;
}


void cnLeafNodeDispose(cnLeafNode* node) {
  cnLeafNodeInit(node);
}


void cnNodeDispose(cnNode* node) {
  if (!node) {
    return;
  }
  // Disposes of child nodes in sub-dispose.
  switch (node->type) {
  case cnNodeTypeLeaf:
    cnLeafNodeDispose((cnLeafNode*)node);
    break;
  case cnNodeTypeSplit:
    cnSplitNodeDispose((cnSplitNode*)node);
    break;
  case cnNodeTypeRoot:
    cnRootNodeDispose((cnRootNode*)node);
    break;
  case cnNodeTypeVar:
    cnVarNodeDispose((cnVarNode*)node);
    break;
  default:
    printf("I don't handle type %u.\n", node->type);
    break;
  }
  // Base node disposal.
  cnNodeInit(node, node->type);
}


void cnNodeDrop(cnNode* node) {
  cnNodeDispose(node);
  free(node);
}


void cnLeafNodeInit(cnLeafNode* leaf) {
  cnNodeInit(&leaf->node, cnNodeTypeLeaf);
  leaf->probability = 0;
  leaf->strength = 0;
}


bool cnLeafNodePropagateBindingBag(
  cnLeafNode* leaf, cnBindingBag* bindingBag,
  cnList(cnLeafBindingBag)* leafBindingBags
) {
  bool result = false;
  cnLeafBindingBag* binding =
    reinterpret_cast<cnLeafBindingBag*>(cnListExpand(leafBindingBags));

  if (!binding) cnErrTo(DONE, "No leaf binding bag.");
  binding->leaf = leaf;
  cnBindingBagInit(
    &binding->bindingBag, bindingBag->bag, bindingBag->entityCount
  );
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


void cnNodeAttachDeep(cnRootNode* root, cnNode* node) {
  cnNode** kid = cnNodeKids(node);
  cnNode** end = kid + cnNodeKidCount(node);
  node->id = root->nextId;
  root->nextId++;
  for (; kid < end; kid++) {
    if (*kid) cnNodeAttachDeep(root, *kid);
  }
}


cnNode* cnNodeFindById(cnNode* node, cnIndex id) {
  if (node->id == id) {
    return node;
  } else {
    cnNode* found = NULL;
    cnNode** kid = cnNodeKids(node);
    cnNode** end = kid + cnNodeKidCount(node);
    for (; kid < end; kid++) {
      if ((found = cnNodeFindById(*kid, id))) {
        return found;
      }
    }
    return NULL;
  }
}


void cnNodeInit(cnNode* node, cnNodeType type) {
  node->id = -1;
  node->parent = NULL;
  node->type = type;
}


cnNode** cnNodeKids(cnNode* node) {
  switch (node->type) {
  case cnNodeTypeLeaf:
    return NULL;
  case cnNodeTypeRoot:
    return &((cnRootNode*)node)->kid;
  case cnNodeTypeSplit:
    return ((cnSplitNode*)node)->kids;
  case cnNodeTypeVar:
    return &((cnVarNode*)node)->kid;
  default:
    assert(false);
    return NULL; // to avoid warnings.
  }
}


cnCount cnNodeKidCount(cnNode* node) {
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


bool cnNodeLeaves(cnNode* node, cnList(cnLeafNode*)* leaves) {
  cnCount count = cnNodeKidCount(node);
  cnNode** kid = cnNodeKids(node);
  cnNode** end = kid + count;
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
  cnNode* node, cnBindingBag* bindingBag,
  cnList(cnLeafBindingBag)* leafBindingBags
) {
  // Sub-propagate. TODO Function pointers of some form instead of this?
  switch (node->type) {
  case cnNodeTypeLeaf:
    return cnLeafNodePropagateBindingBag(
      (cnLeafNode*)node, bindingBag, leafBindingBags
    );
  case cnNodeTypeSplit:
    return cnSplitNodePropagateBindingBag(
      (cnSplitNode*)node, bindingBag, leafBindingBags
    );
  case cnNodeTypeRoot:
    return cnRootNodePropagateBindingBag(
      (cnRootNode*)node, bindingBag, leafBindingBags
    );
  case cnNodeTypeVar:
    return cnVarNodePropagateBindingBag(
      (cnVarNode*)node, bindingBag, leafBindingBags
    );
  default:
    printf("I don't handle type %u for prop.\n", node->type);
    return false;
  }
}


bool cnNodePropagateBindingBags(
  cnNode* node, cnList(cnBindingBag)* bindingBags,
  cnList(cnLeafBindingBagGroup)* leafBindingBagGroups
) {
  cnList(cnLeafBindingBag) leafBindingBags;
  bool result = false;

  // Init.
  cnListInit(&leafBindingBags, sizeof(cnLeafBindingBag));

  // Propagate for each bag.
  cnListEachBegin(bindingBags, cnBindingBag, bindingBag) {
    // Propagate.
    if (!cnNodePropagateBindingBag(node, bindingBag, &leafBindingBags)) {
      cnErrTo(DONE, "No propagate.");
    }
    // Group.
    if (!cnGroupLeafBindingBags(leafBindingBagGroups, &leafBindingBags)) {
      cnErrTo(DONE, "No grouping.");
    }
  } cnEnd;

  // Winned!
  result = true;

  DONE:
  cnListDispose(&leafBindingBags);
  return result;
}


void cnNodePutKid(cnNode* parent, cnIndex k, cnNode* kid) {
  cnNode** kids = cnNodeKids(parent);
  cnNode* old = kids[k];
  cnRootNode* root;
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


void cnNodeReplaceKid(cnNode* oldKid, cnNode* newKid) {
  cnCount k;
  cnCount kidCount = cnNodeKidCount(oldKid->parent);
  cnNode** kid = cnNodeKids(oldKid->parent);
  for (k = 0; k < kidCount; k++, kid++) {
    if (*kid == oldKid) {
      cnNodePutKid(oldKid->parent, k, newKid);
      break;
    }
  }
}


cnRootNode* cnNodeRoot(cnNode* node) {
  while (node->parent) {
    node = node->parent;
  }
  if (node->type == cnNodeTypeRoot) {
    return (cnRootNode*)node;
  }
  return NULL;
}


cnCount cnNodeVarDepth(cnNode* node) {
  cnCount depth = 0;
  while (node->parent) {
    node = node->parent;
    depth += node->type == cnNodeTypeVar;
  }
  return depth;
}


void cnPointBagDispose(cnPointBag* pointBag) {
  if (pointBag) {
    // TODO Split out point matrix init?
    free(pointBag->bindingPointIndices);
    free(pointBag->pointMatrix.points);
    cnPointBagInit(pointBag);
  }
}


void cnPointBagInit(cnPointBag* pointBag) {
  pointBag->bag = NULL;
  pointBag->bindingPointIndices = NULL;
  // TODO Split out point matrix dispose?
  pointBag->pointMatrix.valueCount = 0;
  pointBag->pointMatrix.valueSize = 0;
  pointBag->pointMatrix.points = NULL;
  pointBag->pointMatrix.pointCount = 0;
}


void cnRootNodeDispose(cnRootNode* root) {
  // Dispose of the kid.
  if (root->kid) {
    cnNodeDrop(root->kid);
  }
  // TODO Anything else special?
  cnRootNodeInit(root, false);
}


bool cnRootNodeInit(cnRootNode* root, bool addLeaf) {
  cnNodeInit(&root->node, cnNodeTypeRoot);
  root->node.id = 0;
  root->kid = NULL;
  root->nextId = 1;
  if (addLeaf) {
    cnLeafNode* leaf = cnLeafNodeCreate();
    if (!leaf) {
      return false;
    }
    cnNodePutKid(&root->node, 0, &leaf->node);
  }
  return true;
}


bool cnRootNodePropagateBindingBag(
  cnRootNode* root, cnBindingBag* bindingBag,
  cnList(cnLeafBindingBag)* leafBindingBags
) {
  if (root->kid) {
    return cnNodePropagateBindingBag(root->kid, bindingBag, leafBindingBags);
  }
  return true;
}


cnSplitNode* cnSplitNodeCreate(bool addLeaves) {
  cnSplitNode* split = cnAlloc(cnSplitNode, 1);
  if (!split) return NULL;
  if (!cnSplitNodeInit(split, addLeaves)) {
    free(split);
    return NULL;
  }
  return split;
}


void cnSplitNodeDispose(cnSplitNode* split) {
  // Dispose of the kids.
  cnNode** kid = split->kids;
  cnNode** end = split->kids + cnSplitCount;
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


bool cnSplitNodeInit(cnSplitNode* split, bool addLeaves) {
  cnNode** kid;
  cnNode** end = split->kids + cnSplitCount;
  cnNodeInit(&split->node, cnNodeTypeSplit);
  // Init to null for convenience and safety.
  split->function = NULL;
  split->predicate = NULL;
  split->varIndices = NULL;
  for (kid = split->kids; kid < end; kid++) {
    *kid = NULL;
  }
  if (addLeaves) {
    cnIndex k;
    for (kid = split->kids, k = 0; kid < end; kid++, k++) {
      cnLeafNode* leaf = cnLeafNodeCreate();
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
  cnBinding binding;

  /**
   * For reference to the original order.
   */
  cnIndex index;

  /**
   * For access to arity and var indices.
   */
  cnSplitNode* split;

} cnSplitNodePointBag_Binding;

int cnSplitNodePointBag_compareBindings(const void* a, const void* b) {
  cnSplitNodePointBag_Binding* bindingA =
    reinterpret_cast<cnSplitNodePointBag_Binding*>(const_cast<void*>(a));
  cnSplitNodePointBag_Binding* bindingB =
    reinterpret_cast<cnSplitNodePointBag_Binding*>(const_cast<void*>(b));
  cnIndex i;
  // Compare just on those indices we care about.
  for (i = 0; i < bindingA->split->function->inCount; i++) {
    cnEntity entityA = bindingA->binding[bindingA->split->varIndices[i]];
    cnEntity entityB = bindingB->binding[bindingB->split->varIndices[i]];
    if (entityA != entityB) return entityA > entityB ? 1 : -1;
  }
  // No differences found.
  return 0;
}

cnPointBag* cnSplitNodePointBag(
  cnSplitNode* split, cnBindingBag* bindingBag, cnPointBag* pointBag
) {
  cnEntity* args = NULL;
  bool makeOwnPointBag = !pointBag;
  cnList(cnSplitNodePointBag_Binding) splitBindings;
  cnBindingBag uniqueBag;
  void* values;
  cnCount varDepth;

  // Init first.
  cnListInit(&splitBindings, sizeof(cnSplitNodePointBag_Binding));
  varDepth = cnNodeVarDepth(&split->node);
  cnBindingBagInit(&uniqueBag, NULL, varDepth);
  // Make a point bag id needed, and init either way.
  if (makeOwnPointBag) {
    if (!(pointBag = cnAlloc(cnPointBag, 1))) {
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
    cnIndex b;
    cnIndex p;
    cnSplitNodePointBag_Binding* mostRecentlyKept;
    cnSplitNodePointBag_Binding* splitBinding;

    // Prepare a list of split bindings. Needed for sorting.
    if (!cnListExpandMulti(&splitBindings, bindingBag->bindings.count)) {
      cnErrTo(FAIL, "No split bindings.");
    }
    b = 0;
    splitBinding =
      reinterpret_cast<cnSplitNodePointBag_Binding*>(splitBindings.items);
    cnListEachBegin(&bindingBag->bindings, cnEntity, binding) {
      splitBinding->binding = binding;
      splitBinding->index = b++;
      splitBinding->split = split;
      splitBinding++;
    } cnEnd;

    // Track binding point indices, now that we know how many uniques we have.
    if (!(
      pointBag->bindingPointIndices = cnAlloc(cnIndex, splitBindings.count)
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
  if (!(pointBag->pointMatrix.points = reinterpret_cast<cnFloat*>(malloc(
    pointBag->pointMatrix.pointCount *
    pointBag->pointMatrix.valueCount * pointBag->pointMatrix.valueSize
  )))) cnErrTo(FAIL, "No point matrix.");
  // Put args on the stack.
  if (!(args = cnStackAllocOf(void*, split->function->inCount))) {
    cnErrTo(FAIL, "No args.");
  }

  // Calculate the points.
  values = pointBag->pointMatrix.points;
  cnListEachBegin(&bindingBag->bindings, cnEntity, entities) {
    // Gather the arguments.
    cnIndex a;
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
  cnListDispose(&splitBindings);
  cnBindingBagDispose(&uniqueBag);
  cnStackFree(args);
  return pointBag;
}


bool cnSplitNodePointBags(
  cnSplitNode* split,
  cnList(cnBindingBag)* bindingBags,
  cnList(cnPointBag)* pointBags
) {
  cnPointBag* pointBag;
  bool result = false;
  cnCount validBindingsCount = 0;

  // Init first for safety.
  if (pointBags->count) {
    cnErrTo(FAIL, "Start with empty pointBags, not %ld.", pointBags->count);
  }
  if (!cnListExpandMulti(pointBags, bindingBags->count)) {
    cnErrTo(FAIL, "No point bags.");
  }
  pointBag = reinterpret_cast<cnPointBag*>(pointBags->items);
  cnListEachBegin(bindingBags, cnBindingBag, bindingBag) {
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
  pointBag = reinterpret_cast<cnPointBag*>(pointBags->items);
  cnListEachBegin(bindingBags, cnBindingBag, bindingBag) {
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
  cnListEachBegin(pointBags, cnPointBag, pointBag) {
    free(pointBag->pointMatrix.points);
  } cnEnd;
  cnListClear(pointBags);

  DONE:
  return result;
}


bool cnSplitNodePropagateBindingBag(
  cnSplitNode* split, cnBindingBag* bindingBag,
  cnList(cnLeafBindingBag)* leafBindingBags
) {
  bool allToErr = false;
  cnIndex b;
  cnBindingBag bagsOut[cnSplitCount];
  cnIndex p;
  cnFloat* point;
  cnPointBag* pointBag = NULL;
  cnFloat* pointsEnd;
  cnBindingBag** pointBindingBagOuts = NULL;
  bool result = false;
  cnNode* yes = split->kids[cnSplitYes];
  cnNode* no = split->kids[cnSplitNo];
  cnNode* err = split->kids[cnSplitErr];

  // Init first.
  for (
    cnIndex splitIndex = 0;
    splitIndex < static_cast<cnIndex>(cnSplitCount);
    splitIndex++
  ) {
    cnBindingBagInit(
      bagsOut + splitIndex, bindingBag->bag, bindingBag->entityCount
    );
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
      cnAlloc(cnBindingBag*, pointBag->pointMatrix.pointCount)
  )) cnErrTo(DONE, "No point binding bag assignments.");

  // Go through the points.
  p = 0;
  point = pointBag->pointMatrix.points;
  pointsEnd = point +
    pointBag->pointMatrix.pointCount * pointBag->pointMatrix.valueCount;
  for (; point < pointsEnd; point += pointBag->pointMatrix.valueCount) {
    // Check for error.
    bool allGood = true;
    cnFloat* value = point;
    cnFloat* pointEnd = point + split->function->outCount;
    // TODO Let the function tell us instead of explicitly checking bad?
    for (value = point; value < pointEnd; value++) {
      if (cnIsNaN(*value)) {
        allGood = false;
        break;
      }
    }

    // Choose the bag this point goes to.
    cnSplitIndex splitIndex;
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
  cnListEachBegin(&bindingBag->bindings, cnBinding, bindingIn) {
    // The point index either comes from the mapping or is just the the binding
    // index itself.
    cnIndex p = pointBag->bindingPointIndices ?
      pointBag->bindingPointIndices[b] : b;
    if (!cnListPush(&pointBindingBagOuts[p]->bindings, bindingIn)) {
      cnErrTo(DONE, "No pushed binding.");
    }
    b++;
  } cnEnd;

  PROPAGATE:
  // Now that we have the bindings split, push them down.
  for (
    cnIndex splitIndex = 0;
    splitIndex < static_cast<cnIndex>(cnSplitCount);
    splitIndex++
  ) {
    // Use the original instead of an empty if going to allToErr.
    cnBindingBag* bagOut =
      allToErr && splitIndex == static_cast<cnIndex>(cnSplitErr) ?
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
    cnIndex splitIndex = 0;
    splitIndex < static_cast<cnIndex>(cnSplitCount);
    splitIndex++
  ) {
    cnBindingBagDispose(bagsOut + splitIndex);
  }
  return result;
}


bool cnTreeCopy_handleSplit(cnSplitNode* source, cnSplitNode* copy) {
  if (source->varIndices) {
    // TODO Assert source->function?
    copy->varIndices = cnAlloc(cnIndex, source->function->inCount);
    if (!copy->varIndices) return false;
    if (copy->varIndices) {
      memcpy(
        copy->varIndices, source->varIndices,
        source->function->inCount * sizeof(cnIndex)
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

cnNode* cnTreeCopy(cnNode* node) {
  bool anyFailed = false;
  cnNode* copy = NULL;
  cnNode** end;
  cnNode** kid;
  // TODO Extract node size to separate function?
  cnCount nodeSize;
  switch (node->type) {
  case cnNodeTypeLeaf:
    nodeSize = sizeof(cnLeafNode);
    break;
  case cnNodeTypeRoot:
    nodeSize = sizeof(cnRootNode);
    break;
  case cnNodeTypeSplit:
    nodeSize = sizeof(cnSplitNode);
    break;
  case cnNodeTypeVar:
    nodeSize = sizeof(cnVarNode);
    break;
  default:
    cnErrTo(DONE, "No copy for type %u.", node->type);
  }
  copy = reinterpret_cast<cnNode*>(malloc(nodeSize));
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
      reinterpret_cast<cnSplitNode*>(node), reinterpret_cast<cnSplitNode*>(copy)
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


cnFloat cnTreeLogMetric(cnRootNode* root, cnList(Bag)* bags) {
  cnList(cnLeafCount) counts;
  cnFloat score = cnNaN();
  cnListInit(&counts, sizeof(cnLeafCount));

  // Count positives and negatives in each leaf.
  if (!cnTreeMaxLeafCounts(root, &counts, bags)) cnErrTo(DONE, "No counts.");
  // Calculate the score.
  score = cnCountsLogMetric(&counts);

  DONE:
  cnListDispose(&counts);
  return score;
}


int cnTreeMaxLeafCounts_compareLeafProbsDown(const void* a, const void* b) {
  cnLeafNode* leafA = ((cnLeafBindingBagGroup*)a)->leaf;
  cnLeafNode* leafB = ((cnLeafBindingBagGroup*)b)->leaf;
  return
    // Smaller is higher here for downward sort.
    leafA->probability < leafB->probability ? 1 :
    leafA->probability == leafB->probability ? 0 :
    -1;
}

bool cnTreeMaxLeafCounts(
  cnRootNode* root, cnList(cnLeafCount)* counts, cnList(Bag)* bags
) {
  cnIndex b;
  bool* bagsUsed = NULL;
  cnList(cnLeafBindingBagGroup) groups;
  bool result = false;

  // First propagate bags, and gather leaves.
  // TODO Bindings out of leaves, then extra function to get the lists.
  cnListInit(&groups, sizeof(cnLeafBindingBagGroup));
  if (!cnTreePropagateBags(root, bags, &groups)) {
    cnErrTo(DONE, "No propagate.");
  }

  // Prepare space for counts at one go for efficiency.
  cnListClear(counts);
  if (!cnListExpandMulti(counts, groups.count)) {
    cnErrTo(DONE, "No space for counts.");
  }

  // Prepare space to track which bags have been used up already.
  // TODO Could be bit-efficient, since bools, but don't stress it.
  bagsUsed = cnAlloc(bool, bags->count);
  if (!bagsUsed) cnErrTo(DONE, "No used tracking.");
  // Clear it out manually, fearing bit representations.
  for (b = 0; b < bags->count; b++) bagsUsed[b] = false;

  // Sort the leaves down by probability. Not too many leaves, so no worries.
  qsort(
    groups.items, groups.count, groups.itemSize,
    cnTreeMaxLeafCounts_compareLeafProbsDown
  );

  // Loop through leaves from max prob to min, count bags and marking them used
  // along the way.
  cnListEachBegin(&groups, cnLeafBindingBagGroup, group) {
    // Init the count.
    // TODO Loop providing item and index?
    cnLeafCount* count = reinterpret_cast<cnLeafCount*>(
      cnListGet(counts, group - (cnLeafBindingBagGroup*)groups.items)
    );
    count->leaf = group->leaf;
    count->negCount = 0;
    count->posCount = 0;
    // Loop through bags, if any.
    cnListEachBegin(&group->bindingBags, cnBindingBag, bindingBag) {
      // TODO This subtraction only works if bags is an array of cnBag and not
      // TODO cnBag*, because we'd otherwise have no reference point. Should I
      // TODO consider explicit ids/indices at some point?
      cnIndex bagIndex = bindingBag->bag - (Bag*)bags->items;
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
  free(bagsUsed);
  cnLeafBindingBagGroupListDispose(&groups);
  return result;
}


/**
 * For retaining original index.
 */
typedef struct cnTreeMaxLeafBags_IndexedGroup {
  cnLeafBindingBagGroup* group;
  cnIndex index;
} cnTreeMaxLeafBags_IndexedGroup;

int cnTreeMaxLeafBags_compareLeafProbsDown(const void* a, const void* b) {
  cnLeafNode* leafA = ((cnTreeMaxLeafBags_IndexedGroup*)a)->group->leaf;
  cnLeafNode* leafB = ((cnTreeMaxLeafBags_IndexedGroup*)b)->group->leaf;
  return
    // Smaller is higher here for downward sort.
    leafA->probability < leafB->probability ? 1 :
    leafA->probability == leafB->probability ? 0 :
    -1;
}

bool cnTreeMaxLeafBags(
  cnList(cnLeafBindingBagGroup)* groupsIn,
  cnList(cnList(cnIndex))* groupsMaxOut
) {
  cnIndex b;
  cnCount bagCount;
  Bag* bags = NULL;
  Bag* bagsEnd = NULL;
  bool* bagsUsed = NULL;
  cnIndex g;
  cnTreeMaxLeafBags_IndexedGroup* indexedGroupsIn = NULL;
  bool result = false;

  // TODO Can I generify this enough to unify max bags and max counts somewhat?
  // TODO That is, with callbacks in the later loops?

  // Prepare the groups out.
  if (groupsMaxOut->count) {
    // Just clear the old out for convience. Could error. TODO Document!
    cnListEachBegin(groupsMaxOut, cnList(cnIndex), indices) {
      cnListDispose(indices);
    } cnEnd;
    cnListClear(groupsMaxOut);
  }
  // Allocate space right away.
  if (!cnListExpandMulti(groupsMaxOut, groupsIn->count)) {
    cnErrTo(FAIL, "No groups out.");
  }
  // And init for safety.
  cnListEachBegin(groupsMaxOut, cnList(cnIndex), indices) {
    cnListInit(indices, sizeof(cnIndex));
  } cnEnd;

  // Make a list of groups with their indices.
  if (!(
    indexedGroupsIn = cnAlloc(cnTreeMaxLeafBags_IndexedGroup, groupsIn->count)
  )) cnErrTo(FAIL, "No indexed groups.");
  g = 0;
  cnListEachBegin(groupsIn, cnLeafBindingBagGroup, groupIn) {
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
  bagsUsed = cnAlloc(bool, bagCount);
  if (!bagsUsed) cnErrTo(FAIL, "No bags used array.");
  for (b = 0; b < bagCount; b++) bagsUsed[b] = false;

  // Loop through leaves from max prob to min, count bags and marking them used
  // along the way.
  for (g = 0; g < groupsIn->count; g++) {
    // Group in and out, where out retains original order.
    cnTreeMaxLeafBags_IndexedGroup* groupIn = indexedGroupsIn + g;
    cnList(cnIndex)* indices = reinterpret_cast<cnList(cnIndex)*>(
      cnListGet(groupsMaxOut, groupIn->index)
    );

    // Loop through bags.
    b = 0;
    cnListEachBegin(&groupIn->group->bindingBags, cnBindingBag, bindingBag) {
      // TODO This subtraction only works if bags is an array of cnBag and not
      // TODO cnBag*, because we'd otherwise have no reference point. Should I
      // TODO consider explicit ids/indices at some point?
      cnIndex bagIndex = bindingBag->bag - bags;
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
  cnListEachBegin(groupsMaxOut, cnList(cnIndex), indices) {
    cnListDispose(indices);
  } cnEnd;
  cnListClear(groupsMaxOut);

  DONE:
  free(bagsUsed);
  free(indexedGroupsIn);
  return result;
}


bool cnTreePropagateBag(
  cnRootNode* tree, Bag* bag, cnList(cnLeafBindingBag)* leafBindingBags
) {
  cnBindingBag bindingBag;
  bool result;

  // Init an empty binding bag (with an abusive 0-sized item), and propagate it.
  cnBindingBagInit(&bindingBag, bag, 0);
  bindingBag.bindings.count = 1;
  result = cnNodePropagateBindingBag(&tree->node, &bindingBag, leafBindingBags);
  cnBindingBagDispose(&bindingBag);

  // That was easy.
  return result;
}


bool cnTreePropagateBags(
  cnRootNode* tree, cnList(Bag)* bags,
  cnList(cnLeafBindingBagGroup)* leafBindingBagGroups
) {
  cnList(cnLeafBindingBag) leafBindingBags;
  bool result = false;

  // Init.
  cnListInit(&leafBindingBags, sizeof(cnLeafBindingBag));

  // Propagate for each bag.
  cnListEachBegin(bags, Bag, bag) {
    // Propagate.
    if (!cnTreePropagateBag(tree, bag, &leafBindingBags)) {
      cnErrTo(DONE, "No propagate.");
    }
    // Group.
    if (!cnGroupLeafBindingBags(leafBindingBagGroups, &leafBindingBags)) {
      cnErrTo(DONE, "No grouping.");
    }
  } cnEnd;

  // Winned!
  result = true;

  DONE:
  cnListDispose(&leafBindingBags);
  return result;
}

bool cnTreeWrite_leaf(cnLeafNode* leaf, FILE* file, cnString* indent);
bool cnTreeWrite_root(cnRootNode* root, FILE* file, cnString* indent);
bool cnTreeWrite_split(cnSplitNode* split, FILE* file, cnString* indent);
bool cnTreeWrite_var(cnVarNode* var, FILE* file, cnString* indent);

bool cnTreeWrite_any(cnNode* node, FILE* file, cnString* indent) {
  cnCount kidCount;
  bool result = false;

  fprintf(file, "{\n");
  if (!cnIndent(indent)) cnErrTo(DONE, "No indent.");

  // Handle each node type.
  switch (node->type) {
  case cnNodeTypeLeaf:
    if (!cnTreeWrite_leaf((cnLeafNode*)node, file, indent)) {
      cnErrTo(DONE, "No leaf.");
    }
    break;
  case cnNodeTypeRoot:
    if (!cnTreeWrite_root((cnRootNode*)node, file, indent)) {
      cnErrTo(DONE, "No root.");
    }
    break;
  case cnNodeTypeSplit:
    if (!cnTreeWrite_split((cnSplitNode*)node, file, indent)) {
      cnErrTo(DONE, "No split.");
    }
    break;
  case cnNodeTypeVar:
    if (!cnTreeWrite_var((cnVarNode*)node, file, indent)) {
      cnErrTo(DONE, "No var.");
    }
    break;
  default:
    cnErrTo(DONE, "No such type: %u", node->type);
  }

  // Handle the kids, if any.
  kidCount = cnNodeKidCount(node);
  if (kidCount) {
    cnIndex k;
    cnNode** kids = cnNodeKids(node);
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

bool cnTreeWrite_leaf(cnLeafNode* leaf, FILE* file, cnString* indent) {
  // TODO Check error states?
  fprintf(file, "%s\"type\": \"Leaf\",\n", cnStr(indent));
  fprintf(file, "%s\"probability\": %lg,\n", cnStr(indent), leaf->probability);
  fprintf(file, "%s\"strength\": %lg\n", cnStr(indent), leaf->strength);
  return true;
}

bool cnTreeWrite_root(cnRootNode* root, FILE* file, cnString* indent) {
  // TODO Check error states?
  fprintf(file, "%s\"type\": \"Root\",\n", cnStr(indent));
  return true;
}

bool cnTreeWrite_split(cnSplitNode* split, FILE* file, cnString* indent) {
  bool result = false;

  // TODO Check error states?
  fprintf(file, "%s\"type\": \"Split\",\n", cnStr(indent));
  if (split->function) {
    cnCount arity = split->function->inCount;
    cnIndex i;

    // Function name.
    // TODO Escape strings (like function name)!!
    fprintf(
      file, "%s\"function\": \"%s\",\n",
      cnStr(indent), cnStr(&split->function->name)
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

bool cnTreeWrite_var(cnVarNode* var, FILE* file, cnString* indent) {
  // TODO Check error states?
  fprintf(file, "%s\"type\": \"Var\",\n", cnStr(indent));
  return true;
}

bool cnTreeWrite(cnRootNode* tree, FILE* file) {
  cnString indent;
  bool result = false;
  cnStringInit(&indent);
  // TODO In future stream abstraction, store indent level directly.
  result = cnTreeWrite_any(&tree->node, file, &indent);
  cnStringDispose(&indent);
  return result;
}


cnVarNode* cnVarNodeCreate(bool addLeaf) {
  cnVarNode* var = cnAlloc(cnVarNode, 1);
  if (!var) return NULL;
  if (!cnVarNodeInit(var, addLeaf)) {
    free(var);
    return NULL;
  }
  return var;
}


void cnVarNodeDispose(cnVarNode* var) {
  // Dispose of the kid.
  if (var->kid) {
    cnNodeDrop(var->kid);
  }
  // TODO Anything else special?
  cnVarNodeInit(var, false);
}


bool cnVarNodeInit(cnVarNode* var, bool addLeaf) {
  cnNodeInit(&var->node, cnNodeTypeVar);
  var->kid = NULL;
  if (addLeaf) {
    cnLeafNode* leaf = cnLeafNodeCreate();
    if (!leaf) {
      return false;
    }
    cnNodePutKid(&var->node, 0, &leaf->node);
  }
  return true;
}


bool cnVarNodePropagate_pushBinding(
  cnEntity* entitiesIn, cnBindingBag* bindingBagIn,
  cnEntity entityOut, cnBindingBag* bindingBagOut, cnCount* bindingsOutCount
) {
  cnEntity* entitiesOut =
    reinterpret_cast<cnEntity*>(cnListExpand(&bindingBagOut->bindings));
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
  cnVarNode* var, cnBindingBag* bindingBag,
  cnList(cnLeafBindingBag)* leafBindingBags
) {
  cnIndex b;
  cnBindingBag bindingBagOut;
  cnCount bindingsOutCount = 0;
  bool result = false;

  // Init first.
  cnBindingBagInit(
    &bindingBagOut, bindingBag->bag, bindingBag->entityCount + 1
  );

  // Do we have anything to do?
  if (!var->kid) goto DONE;

  // Find each binding to expand.
  // Use custom looping because of our abusive 2D-ish array.
  // TODO Normal looping really won't work here???
  for (b = 0; b < bindingBag->bindings.count; b++) {
    cnEntity* entitiesIn =
      reinterpret_cast<cnEntity*>(cnListGet(&bindingBag->bindings, b));
    bool anyLeft = false;
    // Figure out if we have constrained options.
    cnList(cnEntity)* entitiesOut = reinterpret_cast<cnList(cnEntity)*>(
      cnListGet(&bindingBag->bag->participantOptions, bindingBag->entityCount)
    );
    if (!(entitiesOut && entitiesOut->count)) {
      // No constraints after all.
      entitiesOut = bindingBag->bag->entities;
    }
    // Find the entities to add on.
    cnListEachBegin(entitiesOut, cnEntity, entityOut) {
      bool found = false;
      cnEntity* entityIn = entitiesIn;
      cnEntity* entitiesInEnd = entityIn + bindingBag->entityCount;
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
  cnBindingBagDispose(&bindingBagOut);
  return result;
}


}