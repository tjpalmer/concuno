#include <stdlib.h>


#include "stackiter-learner.h"


int main(int argc, char** argv) {
  cnList bags;
  cnBindingBagList* bindingBags;
  cnEntityFunction *differenceFunction, *entityFunction;
  cnLearner learner;
  cnList entityFunctions;
  cnRootNode* learnedTree;
  cnSchema schema;
  stState* state;
  cnList states;
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
  printf("%ld true of %ld samples\n", trueCount, bags.count);
  // TODO Print mean number of items in chosen states?

  // Set up schema.
  if (!stSchemaInit(&schema)) {
    printf("Failed to init schema.\n");
    goto DISPOSE_SAMPLES;
  }

  // Set up entity functions.
  // TODO Extract this setup, and make it easier to do.
  cnListInit(&entityFunctions, sizeof(cnEntityFunction));
  if (!(entityFunction = cnListExpand(&entityFunctions))) {
    printf("Failed to expand functions.\n");
    goto DISPOSE_SCHEMA;
  }
  // TODO Look up the type by name.
  itemType = cnListGet(&schema.types, 1);
  // TODO Look up the property by name.
  if (!cnEntityFunctionInitProperty(
    entityFunction, cnListGet(&itemType->properties, 0)
  )) {
    printf("Failed to init function.\n");
    goto DISPOSE_FUNCTIONS;
  }
  // DifferenceLocation
  if (!(differenceFunction = cnListExpand(&entityFunctions))) {
    printf("Failed to expand functions.\n");
    goto DISPOSE_FUNCTIONS;
  }
  if (!cnEntityFunctionInitDifference(differenceFunction, entityFunction)) {
    printf("Failed to init difference.\n");
    goto DISPOSE_FUNCTIONS;
  }
  //printf("Function named %s.\n", cnStr(&differenceFunction->name));

  // Set up the tree.
  if (!cnRootNodeInit(&stubTree, cnTrue)) {
    printf("Failed to init tree.\n");
    goto DISPOSE_FUNCTIONS;
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
  learnedTree = cnLearnerLearn(&learner, &stubTree);
  if (!learnedTree) {
    printf("No learned tree.\n");
    goto DISPOSE_LEARNER;
  }

  status = EXIT_SUCCESS;

  // Dispose of the learned tree.
  cnNodeDispose(&learnedTree->node);
  free(learnedTree);

  DISPOSE_LEARNER:
  cnLearnerDispose(&learner);

  DROP_BINDING_BAGS:
  // Could actually do this earlier, but we'll effectively hang onto this until
  // the end anyway, so no big deal.
  cnBindingBagListDrop(&bindingBags);

  DISPOSE_TREE:
  cnNodeDispose(&stubTree.node);

  DISPOSE_FUNCTIONS:
  cnListEachBegin(&entityFunctions, cnEntityFunction, function) {
    cnEntityFunctionDispose(function);
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
