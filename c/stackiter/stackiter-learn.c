#include <stdlib.h>


#include "stackiter-learn.h"


int main(int argc, char** argv) {
  cnList(cnBag) bags;
  cnEntityFunction* entityFunction;
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
  if (!(entityFunction = cnPushPropertyFunction(
    &entityFunctions, cnListGet(&itemType->properties, 0)
  ))) {
    printf("Failed to push color function.\n");
    goto DROP_FUNCTIONS;
  }
  // DifferenceColor
  if (!cnPushDifferenceFunction(&entityFunctions, entityFunction)) {
    printf("Failed to push difference color function.\n");
    goto DROP_FUNCTIONS;
  }
  // Location.
  // TODO Look up the property by name.
  if (!(entityFunction = cnPushPropertyFunction(
    &entityFunctions, cnListGet(&itemType->properties, 1)
  ))) {
    printf("Failed to push location function.\n");
    goto DROP_FUNCTIONS;
  }
  // DifferenceLocation
  if (!cnPushDifferenceFunction(&entityFunctions, entityFunction)) {
    printf("Failed to push difference location function.\n");
    goto DROP_FUNCTIONS;
  }
  // DistanceLocation
  if (!cnPushDistanceFunction(&entityFunctions, entityFunction)) {
    printf("Failed to push distance location function.\n");
    goto DROP_FUNCTIONS;
  }
  // Velocity.
  // TODO Look up the property by name.
  if (!(entityFunction = cnPushPropertyFunction(
    &entityFunctions, cnListGet(&itemType->properties, 2)
  ))) {
    printf("Failed to push velocity function.\n");
    goto DROP_FUNCTIONS;
  }

  // Set up the tree.
  if (!cnRootNodeInit(&stubTree, cnTrue)) {
    printf("Failed to init tree.\n");
    goto DROP_FUNCTIONS;
  }
  stubTree.entityFunctions = &entityFunctions;

  // Propagate empty binding bags.
  if (!cnRootNodePropagateBags(&stubTree, &bags)) {
    printf("Failed to propagate bags in stub tree.\n");
    goto DISPOSE_TREE;
  }

  // Learn a tree.
  cnLearnerInit(&learner);
  // TODO If no stored bindings, we'll need to pass them in here.
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

  DISPOSE_TREE:
  cnNodeDispose(&stubTree.node);

  DROP_FUNCTIONS:
  cnListEachBegin(&entityFunctions, cnEntityFunction*, function) {
    cnEntityFunctionDrop(*function);
  } cnEnd;
  cnListDispose(&entityFunctions);

  // DISPOSE_SCHEMA:
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
