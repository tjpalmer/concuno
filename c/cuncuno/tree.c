#include <assert.h>
#include <stdio.h>
#include "tree.h"


void cnLeafNodeInit(cnLeafNode* leaf);


void cnRootNodeDispose(cnRootNode* root);


void cnRootNodePropagate(cnRootNode* root);


cnBool cnVarNodeInit(cnVarNode* var, cnBool addLeaf);


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
  // Drop bindings while reference still here.
  cnBindingBagListDrop(&node->bindingBagList);
  // Disposes of child nodes in sub-dispose.
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
  // Base node disposal.
  cnNodeInit(node, node->type);
}


void cnLeafNodeInit(cnLeafNode* leaf) {
  cnNodeInit(&leaf->node, cnNodeTypeLeaf);
  leaf->probability = 0;
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


void cnNodeLeaves(cnNode* node, cnList* leaves) {
  cnCount count = cnNodeKidCount(node);
  cnNode **kid = cnNodeKids(node), **end = kid + count;
  for (; kid < end; kid++) {
    if ((*kid)->type == cnNodeTypeLeaf) {
      assert(cnListPush(leaves, kid));
    } else {
      cnNodeLeaves(*kid, leaves);
    }
  }
}


cnBool cnNodePropagate(cnNode* node, cnBindingBagList* bindingBags) {
  // TODO Or just push them onto any existing?
  node->bindingBagList = bindingBags;
  bindingBags->refCount++;
  switch (node->type) {
  case cnNodeTypeLeaf:
    // Nothing to do for leaf nodes.
    break;
  case cnNodeTypeRoot:
    cnRootNodePropagate((cnRootNode*)node);
    break;
  default:
    printf("I don't handle type %u yet.\n", node->type);
    break;
  }
  return cnTrue;
}


void cnNodePutKid(cnNode* parent, cnIndex k, cnNode* kid) {
  cnNode** kids = cnNodeKids(parent);
  cnNode* old = kids[k];
  cnRootNode* root;
  if (old) {
    // TODO Combined drop?
    cnNodeDispose(old);
    free(old);
  }
  kids[k] = kid;
  kid->parent = parent;
  root = cnNodeRoot(parent);
  if (root) {
    // TODO Abstract the nextId update process?
    kid->id = root->nextId;
    root->nextId++;
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
    // TODO Unified drop?
    cnNodeDispose(root->kid);
    free(root->kid);
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


void cnRootNodePropagate(cnRootNode* root) {
  if (root->kid) {
    cnNodePropagate(root->kid, root->node.bindingBagList);
  }
}


cnVarNode* cnVarNodeCreate(cnBool addLeaf) {
  cnVarNode* var = malloc(sizeof(cnVarNode));
  if (!var) return cnFalse;
  cnVarNodeInit(var, addLeaf);
  return var;
}


cnBool cnVarNodeInit(cnVarNode* var, cnBool addLeaf) {
  cnNodeInit(&var->node, cnNodeTypeVar);
  if (addLeaf) {
    cnLeafNode* leaf = cnLeafNodeCreate();
    if (!leaf) {
      return cnFalse;
    }
    cnNodePutKid(&var->node, 0, &leaf->node);
  }
  return cnTrue;
}
