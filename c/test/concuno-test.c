#include <concuno.h>
#include <stdio.h>
#include <stdlib.h>


// TODO Split stuff out.

// TODO Make a better test harness, too.


void testMultinomialSample(void);


void testPermutations(void);


void testPropagate(void);


void testUnitRand(void);


int main(void) {
  switch (3) {
  case 1:
    testMultinomialSample();
    break;
  case 2:
    testPermutations();
    break;
  case 3:
    testPropagate();
    break;
  case 4:
    testUnitRand();
    break;
  default:
    printf("No test requested.\n");
  }

  return EXIT_SUCCESS;
}


void testMultinomialSample(void) {
  cnIndex i;
  cnIndex j;
  cnCount out[4];
  cnFloat p[] = {0.2, 0.1, 0.4, 0.3};
  printf("testMultinomialSample:\n");
  for (j = 0; j < 4; j++) {
    printf("%lg ", p[j]);
  }
  printf("\n");
  for (i = 0; i < 10; i++) {
    cnMultinomialSample(4, out, 1000, p);
    for (j = 0; j < 4; j++) {
      printf("%ld ", out[j]);
    }
    printf("\n");
  }
}


cnBool testPermutations_handle(
  void *data, cnCount count, cnIndex *permutation
) {
  cnIndex* end = permutation + count;
  for (; permutation < end; permutation++) {
    printf("%ld ", *permutation);
  }
  printf("\n");
  return cnTrue;
}

void testPermutations(void) {
  cnCount count = 2;
  cnCount options = 4;
  printf("Permutations of %ld, taken %ld at a time:\n", options, count);
  cnPermutations(options, count, testPermutations_handle, NULL);
  printf("\n");
}


void testPropagate_CharsDiffGet(
  cnEntityFunction* function, cnEntity* ins, void* outs
) {
  char a = *(char*)(ins[0]);
  char b = *(char*)(ins[1]);
  if (a < 'A' || b < 'A') {
    // Give a NaN for non-letters, just for kicks.
    // TODO Non-float error convention!
    *(cnFloat*)outs = cnNaN();
  } else {
    *(cnFloat*)outs = a - b;
  }
}

cnBool testPropagate_EqualEvaluate(cnPredicate* predicate, void* in) {
  return !*(cnFloat*)in;
}

void testPropagate(void) {
  cnBag bag;
  char* c;
  char* data = "Hello!";
  cnEntityFunction* entityFunction = NULL;
  cnIndex i;
  cnList(cnLeafBindingBag) leafBindingBags;
  cnSplitNode* split;
  cnVarNode* vars[2];
  cnRootNode tree;
  cnType* type = NULL;

  // Init stuff.
  cnBagInit(&bag);
  cnListInit(&leafBindingBags, sizeof(cnLeafBindingBag));
  if (!cnRootNodeInit(&tree, cnFalse)) cnFailTo(DONE, "No tree.");
  // TODO Float required because of NaN convention. Fix this!
  if (!(type = cnTypeCreate("Float", sizeof(cnFloat)))) {
    cnFailTo(DONE, "No type.");
  }
  if (!(entityFunction = cnEntityFunctionCreate("CharsDiff", 2, 1))) {
    cnFailTo(DONE, "No function.");
  }
  entityFunction->get = testPropagate_CharsDiffGet;
  entityFunction->outType = type;

  // Add var nodes.
  // Create and add nodes one at a time, so destruction will be automatic.
  if (!(vars[0] = cnVarNodeCreate(cnFalse))) cnFailTo(DONE, "No var[0].");
  cnNodePutKid(&tree.node, 0, &vars[0]->node);
  if (!(vars[1] = cnVarNodeCreate(cnFalse))) cnFailTo(DONE, "No var[1].");
  cnNodePutKid(&vars[0]->node, 0, &vars[1]->node);

  // Add split node.
  if (!(split = cnSplitNodeCreate(cnTrue))) cnFailTo(DONE, "No split.");
  cnNodePutKid(&vars[1]->node, 0, &split->node);
  split->function = entityFunction;
  // Var indices.
  if (!(
    split->varIndices = malloc(entityFunction->inCount * sizeof(cnIndex))
  )) cnFailTo(DONE, "No var indices.");
  for (i = 0; i < entityFunction->inCount; i++) split->varIndices[i] = i;
  // Predicate.
  if (!(split->predicate = malloc(sizeof(cnPredicate)))) {
    cnFailTo(DONE, "No predicate.");
  }
  split->predicate->copy = NULL;
  split->predicate->dispose = NULL;
  split->predicate->data = NULL;
  split->predicate->evaluate = testPropagate_EqualEvaluate;

  // Prepare bogus data.
  for (c = data; *c; c++) {
    cnEntity entity = c;
    cnListPush(&bag.entities, &entity);
  }

  // Propagate.
  if (!cnTreePropagateBag(&tree, &bag, &leafBindingBags)) {
    cnFailTo(DONE, "No propagate.");
  }

  // And see what bindings we have.
  cnListEachBegin(&leafBindingBags, cnLeafBindingBag, leafBindingBag) {
    printf("Bindings at leaf with id %ld:\n", leafBindingBag->leaf->node.id);
    cnListEachBegin(&leafBindingBag->bindingBag.bindings, cnEntity, entities) {
      cnIndex e;
      printf("  ");
      for (e = 0; e < leafBindingBag->bindingBag.entityCount; e++) {
        char* c = entities[e];
        printf("%c", *c);
      }
      printf("\n");
    } cnEnd;
  } cnEnd;

  DONE:
  cnEntityFunctionDrop(entityFunction);
  cnTypeDrop(type);
  cnNodeDispose(&tree.node);
  cnListEachBegin(&leafBindingBags, cnLeafBindingBag, leafBindingBag) {
    cnLeafBindingBagDispose(leafBindingBag);
  } cnEnd;
  cnListDispose(&leafBindingBags);
  cnBagDispose(&bag);
}


void testUnitRand(void) {
  cnIndex i;
  // TODO Assert stuff, calculate statistics, and so on, instead of printing.
  printf("testUnitRand:\n");
  for (i = 0; i < 10; i++) {
    printf("%lg ", cnUnitRand());
  }
  printf("\n");
}
