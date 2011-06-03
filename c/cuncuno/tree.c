#include <stdio.h>
#include "tree.h"


void cnRootNodeDispose(cnRootNode* node);


void cnBindingDispose(cnBinding* binding) {
  // Just dispose of the list. The entities themselves live on.
  cnListDispose(&binding->entities);
}


void cnBindingBagDispose(cnBindingBag* bindingBag) {
  cnListEachBegin(&bindingBag->bindings, cnBinding, binding) {
    cnBindingDispose(binding);
  } cnEnd;
  cnListDispose(&bindingBag->bindings);
}


cnBindingBagList* cnBindingBagListCreate(void) {
  cnBindingBagList* list = malloc(sizeof(cnBindingBagList));
  if (list) {
    cnListInit(&list->bindingBags, sizeof(cnBindingBag));
    list->refCount = 1;
  }
  return list;
}


void cnBindingBagListDrop(cnBindingBagList* list) {
  if (!list) {
    return;
  }
  list->refCount--;
  if (list->refCount < 1) {
    if (list->refCount < 0) {
      printf("Negative refCount: %ld\n", list->refCount);
    }
    cnListEachBegin(&list->bindingBags, cnBindingBag, bindingBag) {
      cnBindingBagDispose(bindingBag);
    } cnEnd;
    free(list);
  }
}


void cnLeafNodeDispose(cnLeafNode* node) {
  cnLeafNodeInit(node);
}


void cnLeafNodeInit(cnLeafNode* node) {
  cnNodeInit(&node->node, cnNodeTypeLeaf);
  node->probability = 0;
}


void cnNodeDispose(cnNode* node) {
  cnBindingBagListDrop(node->bindingBagList);
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
    // TODO Unified create?
    cnLeafNode* leaf = malloc(sizeof(cnLeafNode));
    if (!leaf) return cnFalse;
    cnLeafNodeInit(leaf);
    node->kid = &leaf->node;
  }
  return cnTrue;
}
