#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "mat.h"
#include "tree.h"


void cnLeafNodeInit(cnLeafNode* leaf);


void cnRootNodeDispose(cnRootNode* root);


cnBool cnRootNodePropagate(cnRootNode* root);


void cnSplitNodeDispose(cnSplitNode* split);


cnBool cnSplitNodeInit(cnSplitNode* split, cnBool addLeaves);


cnBool cnSplitNodePropagate(cnSplitNode* split);


void cnVarNodeDispose(cnVarNode* var);


cnBool cnVarNodeInit(cnVarNode* var, cnBool addLeaf);


cnBool cnVarNodePropagate(cnVarNode* var);


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


cnBool cnBindingValid(cnCount entityCount, void** entities) {
  void** entity = entities;
  void** end = entities + entityCount;
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


void cnLeafNodeDispose(cnLeafNode* node) {
  cnLeafNodeInit(node);
}


cnLeafNode* cnLeafNodeCreate(void) {
  cnLeafNode* leaf = malloc(sizeof(cnLeafNode));
  if (leaf) {
    cnLeafNodeInit(leaf);
  }
  return leaf;
}


void cnNodeDispose(cnNode* node) {
  if (!node) {
    return;
  }
  // Drop bindings while reference still here.
  cnBindingBagListDrop(&node->bindingBagList);
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


void cnNodeAttachDeep(cnRootNode* root, cnNode* node) {
  cnNode** kid = cnNodeKids(node);
  cnNode** end = kid + cnNodeKidCount(node);
  node->id = root->nextId;
  root->nextId++;
  for (; kid < end; kid++) {
    cnNodeAttachDeep(root, *kid);
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
  node->bindingBagList = NULL;
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


cnBool cnNodePropagate(cnNode* node, cnBindingBagList* bindingBags) {
  // TODO Or just push them onto any existing?
  // TODO Change just to returning bindings instead of storing them?
  if (!bindingBags) {
    bindingBags = node->bindingBagList;
    if (!bindingBags) {
      // Still nothing. Force nothing on down?
      return cnFalse;
    }
  } else {
    if (node->bindingBagList) {
      cnBindingBagListDrop(&node->bindingBagList);
    }
    node->bindingBagList = bindingBags;
    bindingBags->refCount++;
  }
  // Sub-propagate.
  switch (node->type) {
  case cnNodeTypeLeaf:
    // Nothing more to do for leaf nodes.
    return cnTrue;
  case cnNodeTypeSplit:
    return cnSplitNodePropagate((cnSplitNode*)node);
  case cnNodeTypeRoot:
    return cnRootNodePropagate((cnRootNode*)node);
  case cnNodeTypeVar:
    // TODO Indicate whether the incomings are new in case we cached outgoings?
    return cnVarNodePropagate((cnVarNode*)node);
  default:
    printf("I don't handle type %u for prop.\n", node->type);
    return cnFalse;
  }
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


cnBool cnRootNodePropagate(cnRootNode* root) {
  if (root->kid) {
    return cnNodePropagate(root->kid, root->node.bindingBagList);
  }
  return cnTrue;
}


cnBool cnRootNodePropagateBags(cnRootNode* root, cnList(cnBag)* bags) {
  cnBool result = cnFalse;
  cnBindingBagList* bindingBags;
  if (!(bindingBags = cnBindingBagListCreate())) {
    printf("Failed to create bindings.\n");
    goto DONE;
  }
  if (!cnBindingBagListPushBags(bindingBags, bags)) {
    printf("Failed to push bindings.\n");
    goto DROP_BINDING_BAGS;
  }
  result = cnNodePropagate(&root->node, bindingBags);

  DROP_BINDING_BAGS:
  cnBindingBagListDrop(&bindingBags);

  DONE:
  return result;
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


cnBool cnSplitNodePropagate(cnSplitNode* split) {
  // TODO Split to proper kids.
  cnSplitIndex splitIndex;
  cnBindingBag* bindingBag;
  cnBindingBagList* kidBagLists[cnSplitCount];
  cnList(cnPointBag) pointBags;
  cnNode* yes = split->kids[cnSplitYes];
  cnNode* no = split->kids[cnSplitNo];
  cnNode* err = split->kids[cnSplitErr];
  cnCount counts[cnSplitCount];
  for (splitIndex = 0; splitIndex < cnSplitCount; splitIndex++) {
    counts[splitIndex] = 0;
    kidBagLists[splitIndex] = NULL;
  }

  // See if we have what we need for splitting.
  if (!(yes && no && err)) {
    printf("No kids for splitting!\n");
    return cnFalse;
  }
  if (!split->predicate) {
    printf("No predicate, so splitting only to err.\n");
    return cnNodePropagate(err, split->node.bindingBagList);
  }

  // We have a predicate and kids.
  // We can actually do something (if we can allocate space and so on).
  cnListInit(&pointBags, sizeof(cnPointBag));
  if (!cnSplitNodePointBags(split, &pointBags)) {
    printf("Failed to get point bags.\n");
    cnListDispose(&pointBags);
    return cnFalse;
  }

  // Got the data. Set up the kids' bags.
  for (splitIndex = 0; splitIndex < cnSplitCount; splitIndex++) {
    kidBagLists[splitIndex] = cnBindingBagListCreate();
    if (!kidBagLists[splitIndex]) {
      // One failed, just drop them all. Nulls are ignored.
      for (splitIndex = 0; splitIndex < cnSplitCount; splitIndex++) {
        cnBindingBagListDrop(kidBagLists + splitIndex);
      }
      return cnFalse;
    }
  }

  // Got the data. Try the split.
  bindingBag = split->node.bindingBagList->bindingBags.items;
  cnListEachBegin(&pointBags, cnPointBag, pointBag) {
    cnBindingBag* bagsOut[cnSplitCount];
    cnBinding bindingIn = bindingBag->bindings.items;
    cnFloat* point = pointBag->pointMatrix;
    cnFloat* pointsEnd = point + pointBag->pointCount * pointBag->valueCount;
    // Null out potential bags until we know we need them.
    for (splitIndex = 0; splitIndex < cnSplitCount; splitIndex++) {
      bagsOut[splitIndex] = NULL;
    }
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
      if (!allGood) {
        splitIndex = cnSplitErr;
      } else {
        // TODO Deal with error cases.
        splitIndex = split->predicate->evaluate(split->predicate, point) ?
          cnSplitYes : cnSplitNo;
      }
      if (!bagsOut[splitIndex]) {
        bagsOut[splitIndex] =
          cnListExpand(&kidBagLists[splitIndex]->bindingBags);
        if (!bagsOut[splitIndex]) {
          // TODO What kind of cleanup?
          printf("That bag list totally didn't get allocated.\n");
          // Let it crash for now. TODO Error out properly.
        }
        cnBindingBagInit(
          bagsOut[splitIndex], pointBag->bag, bindingBag->entityCount
        );
      }
      cnListPush(&bagsOut[splitIndex]->bindings, bindingIn);
      counts[splitIndex]++;
    }
    bindingBag++;
  } cnEnd;

  // Report. TODO Build binding bags for prop.
  printf("Split counts: ");
  for (splitIndex = 0; splitIndex < cnSplitCount; splitIndex++) {
    if (!cnNodePropagate(split->kids[splitIndex], kidBagLists[splitIndex])) {
      printf("Split kid prop failed. Now what?\n");
      // TODO Real cleanup.
    }
    if (splitIndex) printf(", ");
    printf(
      "%ld bags with %ld bindings",
      kidBagLists[splitIndex]->bindingBags.count, counts[splitIndex]
    );
    //    if (splitIndex == cnSplitNo) {
    //      cnListEachBegin(
    //        &kidBagLists[splitIndex]->bindingBags, cnBindingBag, bindingBag
    //      ) {
    //        if (bindingBag->bindings.count == 1) {
    //          cnFloat* out = cnStackAlloc(split->function->outCount * sizeof(cnFloat));
    //          split->function->get(split->function, bindingBag->bindings.items, out);
    //          printf(" [");
    //          cnVectorPrint(stdout, split->function->outCount, out);
    //          printf("]");
    //        }
    //      } cnEnd;
    //    }
    // The kid has it. Drop it here.
    cnBindingBagListDrop(&kidBagLists[splitIndex]);
  }
  printf("\n");

  // Clean up.
  cnListEachBegin(&pointBags, cnPointBag, pointBag) {
    free(pointBag->pointMatrix);
  } cnEnd;
  cnListDispose(&pointBags);
  return cnTrue;
}


cnBool cnSplitNodePointBags(cnSplitNode* split, cnList(cnPointBag)* pointBags) {
  void** args = NULL;
  cnList(cnBindingBag)* bindingBags = &split->node.bindingBagList->bindingBags;
  cnCount validBindingsCount = 0;
  cnPointBag* pointBag;
  cnPointBag* pointBagsEnd;

  // Init first for safety.
  if (pointBags->count) {
    printf("Start with empty pointBags, not %ld!\n", pointBags->count);
    return cnFalse;
  }
  if (!cnListExpandMulti(pointBags, bindingBags->count)) {
    return cnFalse;
  }
  pointBagsEnd = cnListEnd(pointBags);
  pointBag = pointBags->items;
  cnListEachBegin(bindingBags, cnBindingBag, bindingBag) {
    // TODO Standard init function?
    pointBag->bag = bindingBag->bag;
    pointBag->valueCount = split->function->outCount;
    pointBag->valueSize = split->function->outType->size;
    pointBag->pointMatrix = NULL;
    // Null (dummy bindings) will yield NaN as needed.
    // TODO What about for non-float outputs???
    pointBag->pointCount = bindingBag->bindings.count;
    validBindingsCount += pointBag->pointCount;
    // Next bag.
    pointBag++;
  } cnEnd;
  printf("Need to build %ld values.\n", validBindingsCount);

  // Now build the values.
  args = malloc(split->function->inCount * sizeof(void*));
  pointBag = pointBags->items;
  cnListEachBegin(bindingBags, cnBindingBag, bindingBag) {
    void* values;
    // First allocate.
    pointBag->pointMatrix = malloc(
      pointBag->pointCount * pointBag->valueCount * pointBag->valueSize
    );
    if (!pointBag->pointMatrix) {
      printf("No value matrix.\n");
      goto FAIL;
    }
    // Now fill.
    values = pointBag->pointMatrix;
    cnListEachBegin(&bindingBag->bindings, void*, entities) {
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
    // Next bag.
    pointBag++;
  } cnEnd;

  // It all worked.
  free(args);
  return cnTrue;

  FAIL:
  free(args);
  cnListEachBegin(pointBags, cnPointBag, pointBag) {
    free(pointBag->pointMatrix);
  } cnEnd;
  cnListDispose(pointBags);
  return cnFalse;
}


cnNode* cnTreeCopy(cnNode* node) {
  cnNode* copy;
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
    printf("No copy for type %u.\n", node->type);
    return NULL;
  }
  copy = malloc(nodeSize);
  memcpy(copy, node, nodeSize);
  if (copy->bindingBagList) {
    copy->bindingBagList->refCount++;
  }
  // Recursively copy.
  kid = cnNodeKids(copy);
  end = kid + cnNodeKidCount(copy);
  for (; kid < end; kid++) {
    *kid = cnTreeCopy(*kid);
    (*kid)->parent = copy;
  }
  // TODO Handle any custom parts!?!
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
  cnLeafNode* leafA = *(void**)a;
  cnLeafNode* leafB = *(void**)b;
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
  cnList(cnLeafNode*) leaves;
  cnBool result = cnFalse;

  // First propagate bags, and gather leaves.
  // TODO Bindings out of leaves, then extra function to get the lists.
  if (!cnRootNodePropagateBags(root, bags)) cnFailTo(DONE, "No propagate.");
  cnListInit(&leaves, sizeof(cnLeafNode*));
  if (!cnNodeLeaves(&root->node, &leaves)) {
    cnFailTo(DONE, "Failed to gather leaves.");
  }

  // Prepare space for counts at one go for efficiency.
  cnListClear(counts);
  if (!cnListExpandMulti(counts, leaves.count)) {
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
    leaves.items, leaves.count, leaves.itemSize,
    cnNodeMaxLeafCounts_compareLeafProbsDown
  );

  // Loop through leaves from max prob to min, count bags and marking them used
  // along the way.
  cnListEachBegin(&leaves, cnLeafNode*, leaf) {
    cnBindingBagList* bindingBags = (*leaf)->node.bindingBagList;
    // Init the count.
    cnLeafCount* count = cnListGet(counts, leaf - (cnLeafNode**)leaves.items);
    count->leaf = *leaf;
    count->negCount = 0;
    count->posCount = 0;
    // Loop through bags, if any.
    if (!bindingBags) continue;
    cnListEachBegin(&bindingBags->bindingBags, cnBindingBag, bindingBag) {
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
  cnListDispose(&leaves);
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


cnBool cnVarNodePropagate(cnVarNode* var) {
  cnCount bindingsOutCount = 0;
  cnBindingBagList* bindingBagsIn = var->node.bindingBagList;
  cnBindingBagList* bindingBagsOut = cnBindingBagListCreate();
  if (!bindingBagsOut) {
    return cnFalse;
  }
  // Preallocate all the bindingBag space we need.
  cnBindingBag* bindingBagOut = cnListExpandMulti(
    &bindingBagsOut->bindingBags, bindingBagsIn->bindingBags.count
  );
  if (!bindingBagOut) return cnFalse;
  cnListEachBegin(&bindingBagsIn->bindingBags, cnBindingBag, bindingBagIn) {
    cnIndex b;
    cnBindingBagInit(
      bindingBagOut, bindingBagIn->bag, bindingBagIn->entityCount + 1
    );
    // Find each binding to expand.
    // Use custom looping because of our abusive 2D-ish array.
    for (b = 0; b < bindingBagIn->bindings.count; b++) {
      void** entitiesIn = cnListGet(&bindingBagIn->bindings, b);
      cnBool anyLeft = cnFalse;
      // Find the entities to add on.
      cnListEachBegin(&bindingBagIn->bag->entities, void*, entityOut) {
        cnBool found = cnFalse;
        void** entityIn = entitiesIn;
        void** entitiesInEnd = entityIn + bindingBagIn->entityCount;
        for (; entityIn < entitiesInEnd; entityIn++) {
          if (*entityIn == *entityOut) {
            // Already used.
            found = cnTrue;
            break;
          }
        }
        if (!found) {
          anyLeft = cnTrue;
          bindingsOutCount++;
          // Didn't find it. Push a new binding with the new entity.
          void** entitiesOut = cnListExpand(&bindingBagOut->bindings);
          // For the zero length arrays, I'm not sure if memcpy from null is
          // okay, so check that first.
          if (entitiesIn) {
            memcpy(
              entitiesOut, entitiesIn,
              bindingBagIn->entityCount * sizeof(void*)
            );
          }
          entitiesOut[bindingBagIn->entityCount] = *entityOut;
        }
      } cnEnd;
      if (!anyLeft) {
        bindingsOutCount++;
        // Push a dummy binding for later errors since no entities remained.
        // TODO Is it worth extracting a function to avoid this bit of
        // TODO duplication?
        void** entitiesOut = cnListExpand(&bindingBagOut->bindings);
        // For the zero length arrays, I'm not sure if memcpy from null is
        // okay, so check that first.
        if (entitiesIn) {
          memcpy(
            entitiesOut, entitiesIn, bindingBagIn->entityCount * sizeof(void*)
          );
        }
        // Here be the dummy!
        entitiesOut[bindingBagIn->entityCount] = NULL;
      }
    }
    bindingBagOut++;
  } cnEnd;
  //printf("bindingsOutCount: %ld\n", bindingsOutCount);
  if (var->kid) {
    // TODO Also record it here in this var as bindingBagsOut?
    // TODO If not, only calculate when there's a kid.
    cnNodePropagate(var->kid, bindingBagsOut);
  }
  cnBindingBagListDrop(&bindingBagsOut);
  return cnTrue;
}
