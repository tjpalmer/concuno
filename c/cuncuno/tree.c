#include <stdio.h>
#include "tree.h"


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


void cnNodeDispose(cnNode* node) {
  // TODO Dispose of child nodes?
  cnBindingBagListDrop(node->bindingBagList);
  cnNodeInit(node, node->type);
}


void cnNodeInit(cnNode* node, cnNodeType type) {
  node->bindingBagList = NULL;
  node->id = -1;
  node->parent = NULL;
  node->type = type;
}


void cnRootNodeDispose(cnRootNode* node) {
  cnNodeDispose(&node->node);
  // TODO Anything special?
  cnRootNodeInit(node);
}


void cnRootNodeInit(cnRootNode* node) {
  cnNodeInit(&node->node, cnNodeTypeRoot);
  node->node.id = 0;
  node->kid = NULL;
  node->nextId = 1;
}
