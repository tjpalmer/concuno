#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "tree.h"


void cnLeafNodeInit(cnLeafNode* leaf);


void cnRootNodeDispose(cnRootNode* root);


cnBool cnRootNodePropagate(cnRootNode* root);


void cnSplitNodeDispose(cnSplitNode* split);


cnBool cnSplitNodeInit(cnSplitNode* split, cnBool addLeaves);


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
  cnNode **kid = cnNodeKids(node), **end = kid + count;
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
    // Nothing to do for leaf nodes.
    return cnTrue;
  case cnNodeTypeSplit:
    // TODO Handle these.
    printf("I don't handle splits yet for prop.\n");
    return cnTrue;
  case cnNodeTypeRoot:
    return cnRootNodePropagate((cnRootNode*)node);
  case cnNodeTypeVar:
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
  root->entityFunctions = NULL;
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
  // TODO Anything else special?
  cnSplitNodeInit(split, cnFalse);
}


cnBool cnSplitNodeInit(cnSplitNode* split, cnBool addLeaves) {
  cnNode** kid;
  cnNode** end = split->kids + cnSplitCount;
  cnNodeInit(&split->node, cnNodeTypeSplit);
  // Init to null for convenience and safety.
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
  printf("bindingsOutCount: %ld\n", bindingsOutCount);
  if (var->kid) {
    // TODO Also record it here in this var as bindingBagsOut?
    // TODO If not, only calculate when there's a kid.
    cnNodePropagate(var->kid, bindingBagsOut);
  }
  cnBindingBagListDrop(&bindingBagsOut);
  return cnTrue;
}
