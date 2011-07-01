#include <stdlib.h>


#include "stackiter-learn.h"


int main(int argc, char** argv) {
  cnList(cnBag) bags;
  cnBindingBagList* bindingBags;
  cnEntityFunction *differenceFunction, *entityFunction;
  cnLearner learner;
  cnList(cnEntityFunction*) entityFunctions;
  cnRootNode* learnedTree;
  cnSchema schema;
  stState* state;
  cnList(stState) states;
  int status = EXIT_FAILURE;
  cnRootNode stubTree;
  cnCount trueCount;
  cnType* itemType;

  // Validate args.
  if (argc < 2) {
    printf("No data file specified.\n");
    goto DONE;
  }

  // Load file and show stats.
  cnListInit(&states, sizeof(stState));
  if (!stLoad(argv[1], &states)) {
    printf("Failed to load file.\n");
    goto DISPOSE_STATES;
  }
  printf("At end:\n");
  state = cnListGet(&states, states.count - 1);
  printf("%ld items\n", state->items.count);
  printf("%ld states\n", states.count);

  // Choose out the states we want to focus on.
  cnListInit(&bags, sizeof(cnBag));
  if (!stChooseDropWhereLandOnOther(&states, &bags)) {
    printf("Failed to choose samples.\n");
    goto DISPOSE_SAMPLES;
  }
  trueCount = 0;
  cnListEachBegin(&bags, cnBag, bag) {
    trueCount += bag->label;
  } cnEnd;
  // TODO Also print mean number of items in chosen states?
  printf("%ld true of %ld samples\n", trueCount, bags.count);
  // Shuffle bags, with controlled seed (and my own generator?).
  // TODO Shuffle here copies more than just single pointers.
  cnListShuffle(&bags);

  // Set up schema.
  if (!stSchemaInit(&schema)) {
    printf("Failed to init schema.\n");
    goto DISPOSE_SAMPLES;
  }

  // Set up entity functions.
  // TODO Extract this setup, and make it easier to do.
  cnListInit(&entityFunctions, sizeof(cnEntityFunction*));
  // TODO Look up the type by name.
  itemType = cnListGet(&schema.types, 1);
  // Color.
  if (!(entityFunction = cnEntityFunctionCreateProperty(
    cnListGet(&itemType->properties, 0)
  ))) {
    printf("Failed to create function.\n");
    goto DISPOSE_SCHEMA;
  }
  if (!cnListPush(&entityFunctions, &entityFunction)) {
    printf("Failed to expand functions.\n");
    goto DROP_FUNCTIONS;
  }
  // DifferenceColor
  if (!(differenceFunction = cnEntityFunctionCreateDifference(
    entityFunction
  ))) {
    printf("Failed to create difference.\n");
    goto DROP_FUNCTIONS;
  }
  if (!cnListPush(&entityFunctions, &differenceFunction)) {
    printf("Failed to expand functions.\n");
    goto DROP_FUNCTIONS;
  }
  // Location.
  // TODO Look up the property by name.
  if (!(entityFunction = cnEntityFunctionCreateProperty(
    cnListGet(&itemType->properties, 1)
  ))) {
    printf("Failed to create function.\n");
    goto DISPOSE_SCHEMA;
  }
  if (!cnListPush(&entityFunctions, &entityFunction)) {
    printf("Failed to expand functions.\n");
    goto DROP_FUNCTIONS;
  }
  // DifferenceLocation
  if (!(differenceFunction = cnEntityFunctionCreateDifference(
    entityFunction
  ))) {
    printf("Failed to create difference.\n");
    goto DROP_FUNCTIONS;
  }
  if (!cnListPush(&entityFunctions, &differenceFunction)) {
    printf("Failed to expand functions.\n");
    goto DROP_FUNCTIONS;
  }
  // DistanceLocation
  if (!(differenceFunction = cnEntityFunctionCreateDistance(entityFunction))) {
    printf("Failed to create difference.\n");
    goto DROP_FUNCTIONS;
  }
  if (!cnListPush(&entityFunctions, &differenceFunction)) {
    printf("Failed to expand functions.\n");
    goto DROP_FUNCTIONS;
  }
  //printf("Function named %s.\n", cnStr(&differenceFunction->name));
  // Velocity.
  if (cnTrue) {
    // TODO Look up the property by name.
    // TODO The realloc here causes the original Location pointer to be wrong.
    // TODO I need to make sure the pointers are stable in some fashion. Maybe
    // TODO make the list of pointers to individually allocated functions. More
    // TODO small chunks of memory, but we don't expect millions of functions or
    // TODO anything, although we might want to grow some during run time.
    if (!(entityFunction = cnEntityFunctionCreateProperty(
      cnListGet(&itemType->properties, 2)
    ))) {
      printf("Failed to create function.\n");
      goto DROP_FUNCTIONS;
    }
    if (!cnListPush(&entityFunctions, &entityFunction)) {
      printf("Failed to expand functions.\n");
      goto DROP_FUNCTIONS;
    }
  }

  // Set up the tree.
  if (!cnRootNodeInit(&stubTree, cnTrue)) {
    printf("Failed to init tree.\n");
    goto DROP_FUNCTIONS;
  }
  stubTree.entityFunctions = &entityFunctions;

  // Propagate empty binding bags.
  if (!(bindingBags = cnBindingBagListCreate())) {
    printf("Failed to create bindings.\n");
    goto DISPOSE_TREE;
  }
  if (!cnBindingBagListPushBags(bindingBags, &bags)) {
    printf("Failed to push bindings.\n");
    goto DROP_BINDING_BAGS;
  }
  if (!cnNodePropagate(&stubTree.node, bindingBags)) {
    printf("Failed to propagate bindings in stub tree.\n");
    goto DROP_BINDING_BAGS;
  }

  // Learn a tree.
  cnLearnerInit(&learner);
  learnedTree = cnLearnTree(&learner, &stubTree);
  if (!learnedTree) {
    printf("No learned tree.\n");
    goto DISPOSE_LEARNER;
  }

  status = EXIT_SUCCESS;

  // Dispose of the learned tree.
  cnNodeDrop(&learnedTree->node);

  DISPOSE_LEARNER:
  cnLearnerDispose(&learner);

  DROP_BINDING_BAGS:
  // Could actually do this earlier, but we'll effectively hang onto this until
  // the end anyway, so no big deal.
  cnBindingBagListDrop(&bindingBags);

  DISPOSE_TREE:
  cnNodeDispose(&stubTree.node);

  DROP_FUNCTIONS:
  cnListEachBegin(&entityFunctions, cnEntityFunction*, function) {
    cnEntityFunctionDrop(*function);
  } cnEnd;
  cnListDispose(&entityFunctions);

  DISPOSE_SCHEMA:
  cnSchemaDispose(&schema);

  DISPOSE_SAMPLES:
  cnListEachBegin(&bags, cnBag, bag) {
    cnBagDispose(bag);
  } cnEnd;
  cnListDispose(&bags);

  DISPOSE_STATES:
  cnListEachBegin(&states, stState, state) {
    stStateDispose(state);
  } cnEnd;
  cnListDispose(&states);

  DONE:
  return status;

}
