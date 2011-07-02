#include <stdlib.h>

#include "stackiter-learn.h"


void stDisposeSchemaAndEntityFunctions(
  cnSchema* schema, cnList(cnEntityFunction*)* functions
);


cnBool stInitSchemaAndEntityFunctions(
  cnSchema* schema, cnList(cnEntityFunction*)* functions
);


int main(int argc, char** argv) {
  cnList(cnBag) bags;
  cnLearner learner;
  cnList(cnEntityFunction*) entityFunctions;
  cnRootNode* learnedTree;
  cnSchema schema;
  stState* state;
  cnList(stState) states;
  int status = EXIT_FAILURE;
  cnRootNode stubTree;
  cnCount trueCount;

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
    goto DISPOSE_BAGS;
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
  if (!stInitSchemaAndEntityFunctions(&schema, &entityFunctions)) {
    printf("Failed to init schema and functions.\n");
    goto DISPOSE_BAGS;
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
  stDisposeSchemaAndEntityFunctions(&schema, &entityFunctions);

  DISPOSE_BAGS:
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


void stDisposeSchemaAndEntityFunctions(
  cnSchema* schema, cnList(cnEntityFunction*)* functions
) {
  // Drop the functions.
  cnListEachBegin(functions, cnEntityFunction*, function) {
    cnEntityFunctionDrop(*function);
  } cnEnd;
  // Dispose of the list and schema.
  cnListDispose(functions);
  cnSchemaDispose(schema);
}


cnBool stInitSchemaAndEntityFunctions(
  cnSchema* schema, cnList(cnEntityFunction*)* functions
) {
  cnEntityFunction* function;
  cnType* itemType;

  // Set up schema.
  if (!stSchemaInit(schema)) {
    printf("Failed to init schema.\n");
    // Nothing to clean up yet.
    return cnFalse;
  }

  // Set up entity functions.
  // TODO Extract this setup, and make it easier to do.
  cnListInit(functions, sizeof(cnEntityFunction*));
  // TODO Look up the type by name.
  itemType = cnListGet(&schema->types, 1);
  // Color.
  if (!(function = cnPushPropertyFunction(
    functions, cnListGet(&itemType->properties, 0)
  ))) {
    printf("Failed to push color function.\n");
    goto FAIL;
  }
  // DifferenceColor
  if (!cnPushDifferenceFunction(functions, function)) {
    printf("Failed to push difference color function.\n");
    goto FAIL;
  }
  // Location.
  // TODO Look up the property by name.
  if (!(function = cnPushPropertyFunction(
    functions, cnListGet(&itemType->properties, 1)
  ))) {
    printf("Failed to push location function.\n");
    goto FAIL;
  }
  // DifferenceLocation
  if (!cnPushDifferenceFunction(functions, function)) {
    printf("Failed to push difference location function.\n");
    goto FAIL;
  }
  // DistanceLocation
  if (!cnPushDistanceFunction(functions, function)) {
    printf("Failed to push distance location function.\n");
    goto FAIL;
  }
  // Velocity.
  // TODO Look up the property by name.
  if (!(function = cnPushPropertyFunction(
    functions, cnListGet(&itemType->properties, 2)
  ))) {
    printf("Failed to push velocity function.\n");
    goto FAIL;
  }

  // We made it!
  return cnTrue;

  FAIL:
  // Clean it all up if we fail.
  stDisposeSchemaAndEntityFunctions(schema, functions);
  return cnFalse;
}
