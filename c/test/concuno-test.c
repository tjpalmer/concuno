#include <concuno.h>
#include <stdio.h>
#include <stdlib.h>


// TODO Split stuff out.

// TODO Make a better test harness, too.


void testBinomial(void);


void testHeap(void);


void testMultinomial(void);


void testPermutations(void);


void testPropagate(void);


void testRadians(void);


void testReframe(void);


void testUnitRand(void);


void testVariance(void);


int main(void) {
  switch ('f') {
  case 'b':
    testBinomial();
    break;
  case 'f':
    testReframe();
    break;
  case 'h':
    testHeap();
    break;
  case 'm':
    testMultinomial();
    break;
  case 'p':
    testPermutations();
    break;
  case 'r':
    testPropagate();
    break;
  case 's':
    testRadians();
    break;
  case 'u':
    testUnitRand();
    break;
  case 'v':
    testVariance();
    break;
  default:
    printf("No test requested.\n");
  }

  return EXIT_SUCCESS;
}


void testBinomial(void) {
  cnBinomial binomial = NULL;
  cnCount countPerSample = 1000;
  cnIndex i;
  cnFloat prob = 0.3;
  cnCount successTotal = 0;
  cnCount sampleTotal = 1000000;
  cnRandom random = NULL;

  // Init.
  if (!(random = cnRandomCreate())) cnErrTo(DONE, "No random.");
  if (!(binomial = cnBinomialCreate(random, countPerSample, prob))) {
    cnErrTo(DONE, "No random.");
  }

  // Sample.
  printf("Binomial (%ld, %.3lf) samples:", countPerSample, prob);
  for (i = 0; i < sampleTotal; i++) {
    cnCount successCount = cnBinomialSample(binomial);
    //printf(" %ld", successCount);
    successTotal += successCount;
  }

  // Report stats.
  printf(
    " (%lg of %ld*%lg total)\n",
    successTotal / (cnFloat)(sampleTotal * countPerSample),
    countPerSample, (cnFloat)sampleTotal
  );

  DONE:
  cnBinomialDestroy(binomial);
  cnRandomDestroy(random);
}


void testHeap_destroyItem(cnRefAny unused, cnRefAny item) {
  free(item);
}

cnBool testHeap_greater(cnRefAny unused, cnRefAny a, cnRefAny b) {
  cnFloat indexA = *(cnRef(cnFloat))a;
  cnFloat indexB = *(cnRef(cnFloat))b;
  // Make it a max heap for kicks.
  return indexA > indexB;
}

#define testHeap_COUNT 10

void testHeap(void) {
  cnHeap(cnIndex) heap = cnHeapCreate(testHeap_greater);
  cnIndex i;
  if (!heap) cnErrTo(DONE, "No heap.");

  // Load the heap.
  heap->destroyItem = testHeap_destroyItem;
  for (i = 0; i < testHeap_COUNT; i++) {
    cnFloat* number = malloc(sizeof(cnFloat));
    if (!number) cnErrTo(DONE, "No float %ld.", i);
    *number = cnUnitRand();
    if (!cnHeapPush(heap, number)) cnErrTo(DONE, "No push %ld.", i);
  }

  // Drain the heap partially.
  printf("Pulling half: ");
  while (heap->items.count > testHeap_COUNT / 2) {
    cnFloat* number = cnHeapPull(heap);
    printf(" %lf", *number);
    free(number);
  }
  printf("\n");

  // Load the heap again.
  heap->destroyItem = testHeap_destroyItem;
  for (i = 0; i < testHeap_COUNT / 2; i++) {
    cnFloat* number = malloc(sizeof(cnFloat));
    if (!number) cnErrTo(DONE, "No 2nd float %ld.", i);
    *number = cnUnitRand();
    if (!cnHeapPush(heap, number)) cnErrTo(DONE, "No 2nd push %ld.", i);
  }

  // Drain the heap completely.
  printf("Pulling all: ");
  while (heap->items.count) {
    cnFloat* number = cnHeapPull(heap);
    printf(" %lf", *number);
    free(number);
  }
  printf("\n");

  DONE:
  cnHeapDestroy(heap);
}


#define testMultinomial_CLASS_COUNT 4

void testMultinomial(void) {
  cnCount countPerSample = 1000;
  cnIndex i;
  cnIndex j;
  cnMultinomial multinomial = NULL;
  cnCount out[testMultinomial_CLASS_COUNT];
  cnFloat p[] = {0.2, 0.1, 0.4, 0.3};
  cnRandom random = NULL;
  cnCount sampleTotal = 100000;
  cnCount sum[testMultinomial_CLASS_COUNT];

  // Init.
  if (!(random = cnRandomCreate())) cnErrTo(DONE, "No random.");
  if (!(
    multinomial = cnMultinomialCreate(
      random, countPerSample, testMultinomial_CLASS_COUNT, p
    )
  )) cnErrTo(DONE, "No multinomial.");

  printf(
    "testMultinomialSample (%ld*%lg):\n", countPerSample, (cnFloat)sampleTotal
  );
  for (j = 0; j < testMultinomial_CLASS_COUNT; j++) {
    printf("%lg ", p[j]);
    sum[j] = 0;
  }
  printf("\n");

  for (i = 0; i < sampleTotal; i++) {
    cnMultinomialSample(multinomial, out);
    for (j = 0; j < testMultinomial_CLASS_COUNT; j++) {
      sum[j] += out[j];
      //printf("%ld ", out[j]);
    }
    //printf("\n");
  }

  for (j = 0; j < testMultinomial_CLASS_COUNT; j++) {
    printf("%lg ", sum[j] / (cnFloat)(countPerSample * sampleTotal));
  }

  DONE:
  cnMultinomialDestroy(multinomial);
  cnRandomDestroy(random);
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


void testPropagate_charsDiffGet(
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

cnBool testPropagate_equalEvaluate(cnPredicate* predicate, void* in) {
  return !*(cnFloat*)in;
}

cnBool testPropagate_write(
  cnPredicate* predicate, FILE* file, cnString* indent
) {
  cnBool result = cnFalse;

  // TODO Check error state?
  fprintf(file, "{");
  if (predicate->evaluate == testPropagate_equalEvaluate) {
    fprintf(file, "\"evaluate\": \"Equal\"");
  } else cnErrTo(DONE, "Unknown evaluate.");
  fprintf(file, "}");

  // Winned!
  result = cnTrue;

  DONE:
  return result;
}

void testPropagate(void) {
  cnBag bag;
  char* c;
  char* data = "Hello!";
  cnEntityFunction* entityFunction = NULL;
  cnIndex i;
  cnBool initOkay = cnTrue;
  cnList(cnLeafBindingBag) leafBindingBags;
  cnSplitNode* split;
  cnVarNode* vars[2];
  cnRootNode tree;
  cnType* type = NULL;

  // Init stuff.
  cnListInit(&leafBindingBags, sizeof(cnLeafBindingBag));
  initOkay &= cnBagInit(&bag, NULL);
  initOkay &= cnRootNodeInit(&tree, cnFalse);
  if (!initOkay) cnErrTo(DONE, "Init failed.");
  // TODO Float required because of NaN convention. Fix this!
  if (!(type = cnTypeCreate("Float", sizeof(cnFloat)))) {
    cnErrTo(DONE, "No type.");
  }
  if (!(entityFunction = cnEntityFunctionCreate("CharsDiff", 2, 1))) {
    cnErrTo(DONE, "No function.");
  }
  entityFunction->get = testPropagate_charsDiffGet;
  entityFunction->outType = type;

  // Add var nodes.
  // Create and add nodes one at a time, so destruction will be automatic.
  if (!(vars[0] = cnVarNodeCreate(cnFalse))) cnErrTo(DONE, "No var[0].");
  cnNodePutKid(&tree.node, 0, &vars[0]->node);
  if (!(vars[1] = cnVarNodeCreate(cnFalse))) cnErrTo(DONE, "No var[1].");
  cnNodePutKid(&vars[0]->node, 0, &vars[1]->node);

  // Add split node.
  if (!(split = cnSplitNodeCreate(cnTrue))) cnErrTo(DONE, "No split.");
  cnNodePutKid(&vars[1]->node, 0, &split->node);
  split->function = entityFunction;
  // Var indices.
  if (!(
    split->varIndices = malloc(entityFunction->inCount * sizeof(cnIndex))
  )) cnErrTo(DONE, "No var indices.");
  for (i = 0; i < entityFunction->inCount; i++) split->varIndices[i] = i;
  // Predicate.
  if (!(split->predicate = malloc(sizeof(cnPredicate)))) {
    cnErrTo(DONE, "No predicate.");
  }
  split->predicate->copy = NULL;
  split->predicate->dispose = NULL;
  split->predicate->info = NULL;
  split->predicate->evaluate = testPropagate_equalEvaluate;
  split->predicate->write = testPropagate_write;

  // Print the tree while we're at it.
  cnTreeWrite(&tree, stdout);
  printf("\n\n");

  // Prepare bogus data.
  for (c = data; *c; c++) {
    cnEntity entity = c;
    cnListPush(bag.entities, &entity);
  }

  // Propagate.
  if (!cnTreePropagateBag(&tree, &bag, &leafBindingBags)) {
    cnErrTo(DONE, "No propagate.");
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


void testReframe_get(cnEntityFunction* function, cnEntity* ins, void* outs) {
  cnFloat* in = *(cnFloat**)ins;
  cnFloat* inEnd = in + function->outCount;
  cnFloat* out = outs;
  for (; in < inEnd; in++, out++) {
    *out = *in;
  }
}

void testReframe_case(
  cnEntityFunction* reframe,
  cnFloat x0, cnFloat y0, cnFloat x1, cnFloat y1, cnFloat x2, cnFloat y2
) {
  // Set up data.
  double pointsData[][2] = {{x0, y0}, {x1, y1}, {x2, y2}};
  double* points[] = {&pointsData[0][0], &pointsData[1][0], &pointsData[2][0]};
  double result[2];
  // Reframe and check result.
  reframe->get(reframe, (void**)points, result);
  printf(
    "From (%g, %g) to (%g, %g) reframes (%g, %g) as: (%g, %g)\n",
    points[0][0], points[0][1],
    points[1][0], points[1][1],
    points[2][0], points[2][1],
    result[0], result[1]
  );
}

void testReframe(void) {
  cnEntityFunction* direct = NULL;
  cnEntityFunction* reframe = NULL;
  cnSchema schema;

  // Init.
  if (!cnSchemaInitDefault(&schema)) cnFailTo(DONE);
  if (!(direct = cnEntityFunctionCreate("Direct", 1, 2))) cnFailTo(DONE);
  direct->outType = schema.floatType;
  direct->get = testReframe_get;
  if (!(reframe = cnEntityFunctionCreateReframe(direct))) cnFailTo(DONE);

  // Test reframe.
  testReframe_case(reframe, 0.0, 0.0, 1.0, 1.0, 0.0, 1.0);

  DONE:
  cnSchemaDispose(&schema);
  cnEntityFunctionDrop(reframe);
  cnEntityFunctionDrop(direct);
}


void testRadians(void) {
  cnRadian a = cnPi - 0.01;
  cnRadian b = -cnPi + 0.01;
  printf(
    "Radian difference %lg - %lg is %lg, and reverse is %lg.\n",
    a, b, cnWrapRadians(a - b), cnWrapRadians(b - a)
  );
  a = -cnPi / 2;
  b = cnPi / 2;
  printf(
    "Radian difference %lg - %lg is %lg, and reverse is %lg.\n",
    a, b, cnWrapRadians(a - b), cnWrapRadians(b - a)
  );
  a = -cnPi / 2;
  b = -cnPi / 4;
  printf(
    "Radian difference %lg - %lg is %lg, and reverse is %lg.\n",
    a, b, cnWrapRadians(a - b), cnWrapRadians(b - a)
  );
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


void testVariance(void) {
  cnFloat a[] = {1.5, 3.2, -0.1, 4.1, 12.3};
  cnFloat b[] = {6.7, 1.8, 0.2, 4.5, 4.3};
  cnFloat variance = cnScalarVariance(5, 1, a);
  cnFloat covariance = cnScalarCovariance(5, 1, a, 1, b);
  printf("Variance: %lg\n", variance);
  printf("Covariance: %lg\n", covariance);
}
