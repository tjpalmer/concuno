#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "mat.h"
#include "tree.h"


void cnLeafNodeInit(cnLeafNode* leaf);


cnBool cnLeafNodePropagateBindingBag(
  cnLeafNode* leaf, cnBindingBag* bindingBag,
  cnList(cnLeafBindingBag)* leafBindingBags
);


void cnRootNodeDispose(cnRootNode* root);


cnBool cnRootNodePropagateBindingBag(
  cnRootNode* root, cnBindingBag* bindingBag,
  cnList(cnLeafBindingBag)* leafBindingBags
);


void cnSplitNodeDispose(cnSplitNode* split);


cnBool cnSplitNodeInit(cnSplitNode* split, cnBool addLeaves);


cnBool cnSplitNodePropagateBindingBag(
  cnSplitNode* split, cnBindingBag* bindingBag,
  cnList(cnLeafBindingBag)* leafBindingBags
);


void cnVarNodeDispose(cnVarNode* var);


cnBool cnVarNodeInit(cnVarNode* var, cnBool addLeaf);


cnBool cnVarNodePropagateBindingBag(
  cnVarNode* var, cnBindingBag* bindingBag,
  cnList(cnLeafBindingBag)* leafBindingBags
);


void cnBindingBagDispose(cnBindingBag* bindingBag) {
  bindingBag->bag = NULL;
  cnListDispose(&bindingBag->bindings);
}


void cnBindingBagInit(
  cnBindingBag* bindingBag, cnBag* bag, cnCount entityCount
) {
  bindingBag->bag = bag;
  bindingBag->entityCount = entityCount;
  cnListInit(&bindingBag->bindings, entityCount * sizeof(void*));
}


cnBindingBagList* cnBindingBagListCreate(void) {
  cnBindingBagList* list = malloc(sizeof(cnBindingBagList));
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


cnBool cnBindingBagListPushBags(
  cnBindingBagList* bindingBags, const cnList(cnBag)* bags
) {
  // Preallocate all the space we need.
  cnBindingBag* bindingBag =
    cnListExpandMulti(&bindingBags->bindingBags, bags->count);
  if (!bindingBag) return cnFalse;
  // Now init each one.
  cnListEachBegin(bags, cnBag, bag) {
    cnBindingBagInit(bindingBag, bag, 0);
    // Have a fake empty binding. Note that this is abusive. Alternatives?
    bindingBag->bindings.count++;
    bindingBag++;
  } cnEnd;
  return cnTrue;
}


cnBool cnBindingValid(cnCount entityCount, cnEntity* entities) {
  cnEntity* entity = entities;
  cnEntity* end = entities + entityCount;
  for (; entity < end; entity++) {
    if (!*entity) {
      // Bogus/dummy binding.
      return cnFalse;
    }
  }
  return cnTrue;
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


cnBool cnGroupLeafBindingBags(
  cnList(cnLeafBindingBagGroup)* leafBindingBagGroups,
  cnList(cnLeafBindingBag)* leafBindingBags
) {
  cnLeafBindingBagGroup* group;
  cnBool result = cnFalse;

  // See if we already have space in the output list. This is needed only for
  // the first bag, but it's convenient just to leave in place for now.
  if (leafBindingBagGroups->count) {
    if (leafBindingBagGroups->count != leafBindingBags->count) {
      // TODO If not matching, we could at least check to see if the leaf ids
      // TODO in the groups match a subset of the leaves.
      cnFailTo(
        DONE, "Group count %ld != leaf count %ld.",
        leafBindingBagGroups->count, leafBindingBags->count
      );
    }
  } else {
    // Add empty binding bag groups. Should only apply for the first bag, at
    // most.
    if (!cnListExpandMulti(leafBindingBagGroups, leafBindingBags->count)) {
      cnFailTo(DONE, "No leaf binding bag groups.");
    }
    // Init each group.
    group = leafBindingBagGroups->items;
    cnListEachBegin(leafBindingBags, cnLeafBindingBag, leafBindingBag) {
      cnListInit(&group->bindingBags, sizeof(cnBindingBag));
      group->leaf = leafBindingBag->leaf;
      group++;
    } cnEnd;
  }

  // Add the latest bindings to the groups.
  group = leafBindingBagGroups->items;
  cnListEachBegin(leafBindingBags, cnLeafBindingBag, leafBindingBag) {
    // Sanity check. TODO Only needs done for the first bag!?!
    if (leafBindingBag->leaf != group->leaf) {
      cnFailTo(DONE, "Wrong leaf for binding bag group.");
    }
    // Push on the binding bag if it's not empty.
    if (leafBindingBag->bindingBag.bindings.count) {
      if (!cnListPush(
        &group->bindingBags, &leafBindingBag->bindingBag
      )) cnFailTo(DONE, "No space for binding bag.");
    }
    // On to the next leaf/group.
    group++;
  } cnEnd;

  // Clear out the leaf bindings, but don't dispose of their lists, since we
  // stole those for the group.
  cnListClear(leafBindingBags);

  // Winned.
  result = cnTrue;

  DONE:
  return result;
}


void cnLeafBindingBagDispose(cnLeafBindingBag* leafBindingBag) {
  cnBindingBagDispose(&leafBindingBag->bindingBag);
  leafBindingBag->leaf = NULL;
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
  cnLeafNode* leaf = malloc(sizeof(cnLeafNode));
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
}


cnBool cnLeafNodePropagateBindingBag(
  cnLeafNode* leaf, cnBindingBag* bindingBag,
  cnList(cnLeafBindingBag)* leafBindingBags
) {
  cnBool result = cnFalse;
  cnLeafBindingBag* binding = cnListExpand(leafBindingBags);

  if (!binding) cnFailTo(DONE, "No leaf binding bag.");
  binding->leaf = leaf;
  cnBindingBagInit(
    &binding->bindingBag, bindingBag->bag, bindingBag->entityCount
  );
  if (bindingBag->bindings.count) {
    if (!cnListPushAll(&binding->bindingBag.bindings, &bindingBag->bindings)) {
      // We couldn't really make the binding, so hide it.
      leafBindingBags->count--;
      cnFailTo(DONE, "No bindings for bag.");
    }
  }

  // Winned!
  result = cnTrue;

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
    assert(cnFalse);
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
    assert(cnFalse);
    return 0; // to avoid warnings.
  }
}


cnBool cnNodeLeaves(cnNode* node, cnList(cnLeafNode*)* leaves) {
  cnCount count = cnNodeKidCount(node);
  cnNode** kid = cnNodeKids(node);
  cnNode** end = kid + count;
  for (; kid < end; kid++) {
    if ((*kid)->type == cnNodeTypeLeaf) {
      if (!cnListPush(leaves, kid)) {
        return cnFalse;
      }
    } else {
      if (!cnNodeLeaves(*kid, leaves)) {
        return cnFalse;
      }
    }
  }
  return cnTrue;
}


cnBool cnNodePropagateBindingBag(
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
    return cnFalse;
  }
}


cnBool cnNodePropagateBindingBags(
  cnNode* node, cnList(cnBindingBag)* bindingBags,
  cnList(cnLeafBindingBagGroup)* leafBindingBagGroups
) {
  cnList(cnLeafBindingBag) leafBindingBags;
  cnBool result = cnFalse;

  // Init.
  cnListInit(&leafBindingBags, sizeof(cnLeafBindingBag));

  // Propagate for each bag.
  cnListEachBegin(bindingBags, cnBindingBag, bindingBag) {
    // Propagate.
    if (!cnNodePropagateBindingBag(node, bindingBag, &leafBindingBags)) {
      cnFailTo(DONE, "No propagate.");
    }
    // Group.
    if (!cnGroupLeafBindingBags(leafBindingBagGroups, &leafBindingBags)) {
      cnFailTo(DONE, "No grouping.");
    }
  } cnEnd;

  // Winned!
  result = cnTrue;

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


void cnRootNodeDispose(cnRootNode* root) {
  // Dispose of the kid.
  if (root->kid) {
    cnNodeDrop(root->kid);
  }
  // TODO Anything else special?
  cnRootNodeInit(root, cnFalse);
}


cnBool cnRootNodeInit(cnRootNode* root, cnBool addLeaf) {
  cnNodeInit(&root->node, cnNodeTypeRoot);
  root->node.id = 0;
  root->kid = NULL;
  root->nextId = 1;
  if (addLeaf) {
    cnLeafNode* leaf = cnLeafNodeCreate();
    if (!leaf) {
      return cnFalse;
    }
    cnNodePutKid(&root->node, 0, &leaf->node);
  }
  return cnTrue;
}


cnBool cnRootNodePropagateBindingBag(
  cnRootNode* root, cnBindingBag* bindingBag,
  cnList(cnLeafBindingBag)* leafBindingBags
) {
  if (root->kid) {
    return cnNodePropagateBindingBag(root->kid, bindingBag, leafBindingBags);
  }
  return cnTrue;
}


cnSplitNode* cnSplitNodeCreate(cnBool addLeaves) {
  cnSplitNode* split = malloc(sizeof(cnSplitNode));
  if (!split) return cnFalse;
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
  cnSplitNodeInit(split, cnFalse);
}


cnBool cnSplitNodeInit(cnSplitNode* split, cnBool addLeaves) {
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
        return cnFalse;
      }
      cnNodePutKid(&split->node, k, &leaf->node);
    }
  }
  return cnTrue;
}


cnPointBag* cnSplitNodePointBag(
  cnSplitNode* split, cnBindingBag* bindingBag, cnPointBag* pointBag
) {
  cnEntity* args = NULL;
  cnBool makeOwnPointBag = !pointBag;
  void* values;

  // Init first.
  if (makeOwnPointBag) {
    if (!(pointBag = malloc(sizeof(cnPointBag)))) {
      cnFailTo(FAIL, "No point bag.");
    }
  } else if (pointBag->pointMatrix) {
    // Failing to DONE on purpose here, so we don't free their data!
    cnFailTo(DONE, "Point bag has %ld points already!", pointBag->pointCount);
  }
  pointBag->bag = bindingBag->bag;
  pointBag->valueCount = split->function->outCount;
  pointBag->valueSize = split->function->outType->size;
  pointBag->pointMatrix = NULL;
  // Null (dummy bindings) will yield NaN as needed.
  // TODO What about for non-float outputs???
  pointBag->pointCount = bindingBag->bindings.count;

  // Prepare space for points.
  if (!(pointBag->pointMatrix = malloc(
    pointBag->pointCount * pointBag->valueCount * pointBag->valueSize
  ))) cnFailTo(FAIL, "No point matrix.");
  // Put args on the stack.
  if (!(args = cnStackAlloc(split->function->inCount * sizeof(void*)))) {
    cnFailTo(FAIL, "No args.");
  }

  // Calculate the points.
  values = pointBag->pointMatrix;
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
    values = ((char*)values) + pointBag->valueCount * pointBag->valueSize;
  } cnEnd;

  // We winned.
  goto DONE;

  FAIL:
  if (pointBag) {
    // We won't be needing this anymore.
    free(pointBag->pointMatrix);
    if (makeOwnPointBag) {
      // Free it if we made it.
      free(pointBag);
    } else {
      // Clean it up.
      pointBag->pointCount = 0;
      pointBag->pointMatrix = NULL;
    }
  }

  DONE:
  cnStackFree(args);
  return pointBag;
}


typedef struct cnSplitNodePointBags_Binding {
  cnBinding binding;
  cnIndex index;
  cnSplitNode* split;
} cnSplitNodePointBags_Binding;

int cnSplitNodePointBags_compareBindingBags(const void* a, const void* b) {
  cnSplitNodePointBags_Binding* bindingA = (void*)a;
  cnSplitNodePointBags_Binding* bindingB = (void*)b;
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

cnBool cnSplitNodePointBags(
  cnSplitNode* split,
  cnList(cnBindingBag)* bindingBags,
  cnList(cnPointBag)* pointBags
) {
  cnPointBag* pointBag;
  cnBool result = cnFalse;
  cnCount validBindingsCount = 0;

  // Init first for safety.
  if (pointBags->count) {
    cnFailTo(FAIL, "Start with empty pointBags, not %ld.", pointBags->count);
  }
  if (!cnListExpandMulti(pointBags, bindingBags->count)) {
    cnFailTo(FAIL, "No point bags.");
  }
  pointBag = pointBags->items;
  cnListEachBegin(bindingBags, cnBindingBag, bindingBag) {
    // Clear out each point bag for later filling.
    // TODO Standard init function?
    pointBag->pointCount = 0;
    pointBag->pointMatrix = NULL;
    // Null (dummy bindings) will yield NaN as needed.
    // TODO What about for non-float outputs???
    validBindingsCount += bindingBag->bindings.count;
    // Next bag.
    pointBag++;
  } cnEnd;
  printf("Need to build %ld values\n", validBindingsCount);

  // Now build the values.
  // TODO Actually, make an array of all the args and eliminate the duplicates!
  // TODO Dupes can come from bindings where only the non-args are unique.
  pointBag = pointBags->items;
  cnListEachBegin(bindingBags, cnBindingBag, bindingBag) {
    // TODO
    // TODO Avoid duplicates as follows:
    // TODO Build a list of cnSplitNodePointBags_Binding.
    // TODO Sort them.
    // TODO Check for duplicates as we loop.
    // TODO By storing an index, can I retain the original order?
    // TODO I don't want to introduce bias by sorting.
    // TODO
    if (!cnSplitNodePointBag(split, bindingBag, pointBag)) {
      cnFailTo(FAIL, "No point bag.");
    }
    pointBag++;
  } cnEnd;

  // It all worked.
  result = cnTrue;
  goto DONE;

  FAIL:
  cnListEachBegin(pointBags, cnPointBag, pointBag) {
    free(pointBag->pointMatrix);
  } cnEnd;
  cnListClear(pointBags);

  DONE:
  return result;
}


cnBool cnSplitNodePropagateBindingBag(
  cnSplitNode* split, cnBindingBag* bindingBag,
  cnList(cnLeafBindingBag)* leafBindingBags
) {
  cnBool allToErr = cnFalse;
  cnBindingBag bagsOut[cnSplitCount];
  cnBinding bindingIn;
  cnFloat* point;
  cnPointBag* pointBag = NULL;
  cnFloat* pointsEnd;
  cnBool result = cnFalse;
  cnSplitIndex splitIndex;
  cnNode* yes = split->kids[cnSplitYes];
  cnNode* no = split->kids[cnSplitNo];
  cnNode* err = split->kids[cnSplitErr];

  // Init first.
  for (splitIndex = 0; splitIndex < cnSplitCount; splitIndex++) {
    cnBindingBagInit(
      bagsOut + splitIndex, bindingBag->bag, bindingBag->entityCount
    );
  }

  // Check error cases.
  if (!(yes && no && err)) {
    result = cnTrue;
    goto DONE;
  }
  if (!(split->function && split->predicate)) {
    // No way to say where to go, so go to error node.
    allToErr = cnTrue;
    goto PROPAGATE;
  }

  // Gather up points.
  if (!(pointBag = cnSplitNodePointBag(split, bindingBag, NULL))) {
    cnFailTo(DONE, "No point bag.");
  }

  // Go through the points.
  bindingIn = bindingBag->bindings.items;
  point = pointBag->pointMatrix;
  pointsEnd = point + pointBag->pointCount * pointBag->valueCount;
  for (;
    point < pointsEnd;
    point += pointBag->valueCount, bindingIn += bindingBag->entityCount
  ) {
    // Check for error.
    cnBool allGood = cnTrue;
    cnFloat* value = point;
    cnFloat* pointEnd = point + split->function->outCount;
    // TODO Let the function tell us instead of explicitly checking bad?
    for (value = point; value < pointEnd; value++) {
      if (cnIsNaN(*value)) {
        allGood = cnFalse;
        break;
      }
    }
    // Choose the bag to go to.
    if (!allGood) {
      splitIndex = cnSplitErr;
    } else {
      splitIndex = split->predicate->evaluate(split->predicate, point) ?
        cnSplitYes : cnSplitNo;
      // I hack -1 (turned unsigned) into this for errors at this point.
      if (splitIndex >= cnSplitCount) cnFailTo(DONE, "Bad evaluate.");
    }
    if (!cnListPush(&bagsOut[splitIndex].bindings, bindingIn)) {
      cnFailTo(DONE, "No pushed binding.");
    }
  }

  PROPAGATE:
  // Now that we have the bindings split, push them down.
  for (splitIndex = 0; splitIndex < cnSplitCount; splitIndex++) {
    // Use the original instead of an empty if going to allToErr.
    cnBindingBag* bagOut = allToErr && splitIndex == cnSplitErr ?
      bindingBag : bagsOut + splitIndex;
    if (!cnNodePropagateBindingBag(
      split->kids[splitIndex], bagOut, leafBindingBags
    )) cnFailTo(DONE, "Split kid prop failed. Now what?\n");
  }

  // Winned!
  result = cnTrue;

  DONE:
  if (pointBag) {
    free(pointBag->pointMatrix);
    free(pointBag);
  }
  for (splitIndex = 0; splitIndex < cnSplitCount; splitIndex++) {
    cnBindingBagDispose(bagsOut + splitIndex);
  }
  return result;
}


cnBool cnTreeCopy_handleSplit(cnSplitNode* source, cnSplitNode* copy) {
  if (source->varIndices) {
    // TODO Assert source->function?
    copy->varIndices = malloc(source->function->inCount * sizeof(cnIndex));
    if (!copy->varIndices) return cnFalse;
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
      return cnFalse;
    }
  }
  return cnTrue;
}

cnNode* cnTreeCopy(cnNode* node) {
  cnBool anyFailed = cnFalse;
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
    cnFailTo(DONE, "No copy for type %u.", node->type);
  }
  copy = malloc(nodeSize);
  if (!copy) cnFailTo(DONE, "Failed to allocate copy.");
  memcpy(copy, node, nodeSize);
  // Recursively copy.
  kid = cnNodeKids(copy);
  end = kid + cnNodeKidCount(copy);
  for (; kid < end; kid++) {
    *kid = anyFailed ? NULL : cnTreeCopy(*kid);
    if (!*kid) {
      anyFailed = cnTrue;
      continue;
    }
    (*kid)->parent = copy;
  }
  if (node->type == cnNodeTypeSplit) {
    // Custom handling for split nodes.
    anyFailed = !cnTreeCopy_handleSplit((void*)node, (void*)copy);
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


cnFloat cnTreeLogMetric(cnRootNode* root, cnList(cnBag)* bags) {
  cnList(cnLeafCount) counts;
  cnFloat score = cnNaN();
  cnListInit(&counts, sizeof(cnLeafCount));

  // Count positives and negatives in each leaf.
  if (!cnTreeMaxLeafCounts(root, &counts, bags)) cnFailTo(DONE, "No counts.");
  // Calculate the score.
  score = cnCountsLogMetric(&counts);

  DONE:
  cnListDispose(&counts);
  return score;
}


int cnNodeMaxLeafCounts_compareLeafProbsDown(const void* a, const void* b) {
  cnLeafNode* leafA = ((cnLeafBindingBagGroup*)a)->leaf;
  cnLeafNode* leafB = ((cnLeafBindingBagGroup*)b)->leaf;
  return
    // Smaller is higher here for downward sort.
    leafA->probability < leafB->probability ? 1 :
    leafA->probability == leafB->probability ? 0 :
    -1;
}

cnBool cnTreeMaxLeafCounts(
  cnRootNode* root, cnList(cnLeafCount)* counts, cnList(cnBag)* bags
) {
  cnIndex b;
  cnBool* bagsUsed = NULL;
  cnList(cnLeafBindingBagGroup) groups;
  cnBool result = cnFalse;

  // First propagate bags, and gather leaves.
  // TODO Bindings out of leaves, then extra function to get the lists.
  cnListInit(&groups, sizeof(cnLeafBindingBagGroup));
  if (!cnTreePropagateBags(root, bags, &groups)) {
    cnFailTo(DONE, "No propagate.");
  }

  // Prepare space for counts at one go for efficiency.
  cnListClear(counts);
  if (!cnListExpandMulti(counts, groups.count)) {
    cnFailTo(DONE, "No space for counts.");
  }

  // Prepare space to track which bags have been used up already.
  // TODO Could be bit-efficient, since bools, but don't stress it.
  bagsUsed = malloc(bags->count * sizeof(cnBool));
  if (!bagsUsed) cnFailTo(DONE, "No used tracking.");
  // Clear it out manually, fearing bit representations.
  for (b = 0; b < bags->count; b++) bagsUsed[b] = cnFalse;

  // Sort the leaves down by probability. Not too many leaves, so no worries.
  qsort(
    groups.items, groups.count, groups.itemSize,
    cnNodeMaxLeafCounts_compareLeafProbsDown
  );

  // Loop through leaves from max prob to min, count bags and marking them used
  // along the way.
  cnListEachBegin(&groups, cnLeafBindingBagGroup, group) {
    // Init the count.
    // TODO Loop providing item and index?
    cnLeafCount* count =
      cnListGet(counts, group - (cnLeafBindingBagGroup*)groups.items);
    count->leaf = group->leaf;
    count->negCount = 0;
    count->posCount = 0;
    // Loop through bags, if any.
    cnListEachBegin(&group->bindingBags, cnBindingBag, bindingBag) {
      // TODO This subtraction only works if bags is an array of cnBag and not
      // TODO cnBag*, because we'd otherwise have no reference point. Should I
      // TODO consider explicit ids/indices at some point?
      cnIndex bagIndex = bindingBag->bag - (cnBag*)bags->items;
      if (bagsUsed[bagIndex]) continue;
      // Add to the proper count.
      if (bindingBag->bag->label) {
        count->posCount++;
      } else {
        count->negCount++;
      }
      bagsUsed[bagIndex] = cnTrue;
    } cnEnd;
  } cnEnd;
  // All done!
  result = cnTrue;

  DONE:
  free(bagsUsed);
  cnLeafBindingBagGroupListDispose(&groups);
  return result;
}


cnBool cnTreePropagateBag(
  cnRootNode* tree, cnBag* bag, cnList(cnLeafBindingBag)* leafBindingBags
) {
  cnBindingBag bindingBag;
  cnBool result;

  // Init an empty binding bag (with an abusive 0-sized item), and propagate it.
  cnBindingBagInit(&bindingBag, bag, 0);
  bindingBag.bindings.count = 1;
  result = cnNodePropagateBindingBag(&tree->node, &bindingBag, leafBindingBags);
  cnBindingBagDispose(&bindingBag);

  // That was easy.
  return result;
}


cnBool cnTreePropagateBags(
  cnRootNode* tree, cnList(cnBag)* bags,
  cnList(cnLeafBindingBagGroup)* leafBindingBagGroups
) {
  cnList(cnLeafBindingBag) leafBindingBags;
  cnBool result = cnFalse;

  // Init.
  cnListInit(&leafBindingBags, sizeof(cnLeafBindingBag));

  // Propagate for each bag.
  cnListEachBegin(bags, cnBag, bag) {
    // Propagate.
    if (!cnTreePropagateBag(tree, bag, &leafBindingBags)) {
      cnFailTo(DONE, "No propagate.");
    }
    // Group.
    if (!cnGroupLeafBindingBags(leafBindingBagGroups, &leafBindingBags)) {
      cnFailTo(DONE, "No grouping.");
    }
  } cnEnd;

  // Winned!
  result = cnTrue;

  DONE:
  cnListDispose(&leafBindingBags);
  return result;
}


cnVarNode* cnVarNodeCreate(cnBool addLeaf) {
  cnVarNode* var = malloc(sizeof(cnVarNode));
  if (!var) return cnFalse;
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
  cnVarNodeInit(var, cnFalse);
}


cnBool cnVarNodeInit(cnVarNode* var, cnBool addLeaf) {
  cnNodeInit(&var->node, cnNodeTypeVar);
  var->kid = NULL;
  if (addLeaf) {
    cnLeafNode* leaf = cnLeafNodeCreate();
    if (!leaf) {
      return cnFalse;
    }
    cnNodePutKid(&var->node, 0, &leaf->node);
  }
  return cnTrue;
}


cnBool cnVarNodePropagate_pushBinding(
  cnEntity* entitiesIn, cnBindingBag* bindingBagIn,
  cnEntity entityOut, cnBindingBag* bindingBagOut, cnCount* bindingsOutCount
) {
  cnEntity* entitiesOut = cnListExpand(&bindingBagOut->bindings);
  if (!entitiesOut) return cnFalse;
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
  return cnTrue;
}


cnBool cnVarNodePropagateBindingBag(
  cnVarNode* var, cnBindingBag* bindingBag,
  cnList(cnLeafBindingBag)* leafBindingBags
) {
  cnIndex b;
  cnBindingBag bindingBagOut;
  cnCount bindingsOutCount = 0;
  cnBool result = cnFalse;

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
    cnEntity* entitiesIn = cnListGet(&bindingBag->bindings, b);
    cnBool anyLeft = cnFalse;
    // Find the entities to add on.
    cnListEachBegin(&bindingBag->bag->entities, cnEntity, entityOut) {
      cnBool found = cnFalse;
      cnEntity* entityIn = entitiesIn;
      cnEntity* entitiesInEnd = entityIn + bindingBag->entityCount;
      // TODO Loop okay since usually few vars?
      for (; entityIn < entitiesInEnd; entityIn++) {
        if (*entityIn == *entityOut) {
          // Already used.
          found = cnTrue;
          break;
        }
      }
      if (!found) {
        // Didn't find it. Push a new binding with the new entity.
        anyLeft = cnTrue;
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
    cnFailTo(DONE, "No propagate.");
  }

  // Winned!
  result = cnTrue;

  DONE:
  cnBindingBagDispose(&bindingBagOut);
  return result;
}
