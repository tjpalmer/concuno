#include <stdio.h>
#include "tree.h"


void cnRootNodeDispose(cnRootNode* node);


void cnLeafNodeInit(cnLeafNode* leaf);


void cnBindingDispose(cnBinding* binding) {
  // Just dispose of the list. The entities themselves live on.
  cnListDispose(&binding->entities);
}


void cnBindingBagDispose(cnBindingBag* bindingBag) {
  bindingBag->bag = NULL;
  cnListEachBegin(&bindingBag->bindings, cnBinding, binding) {
    cnBindingDispose(binding);
  } cnEnd;
  cnListDispose(&bindingBag->bindings);
}


void cnBindingBagInit(cnBindingBag* bindingBag, cnBag* bag) {
  bindingBag->bag = bag;
  cnListInit(&bindingBag->bindings, sizeof(cnBinding));
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
  cnBindingBagList* bindingBags, const cnList* bags
) {
  // Preallocate all the space we need.
  cnBindingBag* bindingBag =
    cnListExpandMulti(&bindingBags->bindingBags, bags->count);
  if (!bindingBag) return cnFalse;
  // Now init each one.
  cnListEachBegin(bags, cnBag, bag) {
    cnBindingBagInit(bindingBag, bag);
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
  cnBindingBagListDrop(&node->bindingBagList);
  // TODO Dispose of child nodes.
  switch (node->type) {
  case cnNodeTypeLeaf:
    cnLeafNodeDispose((cnLeafNode*)node);
    break;
  case cnNodeTypeRoot:
    cnRootNodeDispose((cnRootNode*)node);
    break;
  default:
    printf("I don't handle type %u yet.\n", node->type);
    break;
  }
}


void cnLeafNodeInit(cnLeafNode* leaf) {
  cnNodeInit(&leaf->node, cnNodeTypeLeaf);
  leaf->probability = 0;
}


cnBool cnNodePropagate(cnNode* node, cnBindingBagList* bindingBags) {
  // TODO Do something based on node type.
  return cnTrue;
}


void cnNodeInit(cnNode* node, cnNodeType type) {
  node->bindingBagList = NULL;
  node->id = -1;
  node->parent = NULL;
  node->type = type;
}


void cnRootNodeDispose(cnRootNode* node) {
  // Dispose of the kid.
  if (node->kid) {
    // TODO Unified drop?
    cnNodeDispose(node->kid);
    free(node->kid);
  }
  // TODO Anything else special?
  cnRootNodeInit(node, cnFalse);
}


cnBool cnRootNodeInit(cnRootNode* node, cnBool addLeaf) {
  cnNodeInit(&node->node, cnNodeTypeRoot);
  node->node.id = 0;
  node->kid = NULL;
  node->entityFunctions = NULL;
  node->nextId = 1;
  if (addLeaf) {
    node->kid = &cnLeafNodeCreate()->node;
  }
  return cnTrue;
}
