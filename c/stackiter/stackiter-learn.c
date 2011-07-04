#include <stdlib.h>

#include "stackiter-learn.h"


cnBool stClusterStuff(
  cnList(stState)* states, cnList(cnEntityFunction*)* functions
);


void stDisposeSchemaAndEntityFunctions(
  cnSchema* schema, cnList(cnEntityFunction*)* functions
);


cnBool stInitSchemaAndEntityFunctions(
  cnSchema* schema, cnList(cnEntityFunction*)* functions
);


cnBool stLearnConcept(
  cnList(stState)* states,
  cnList(cnEntityFunction*)* functions,
  cnBool (*choose)(cnList(stState)* states, cnList(cnBag)* bags)
);


int main(int argc, char** argv) {
  cnList(cnEntityFunction*) entityFunctions;
  cnSchema schema;
  stState* state;
  cnList(stState) states;
  int status = EXIT_FAILURE;

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

  // Set up schema.
  if (!stInitSchemaAndEntityFunctions(&schema, &entityFunctions)) {
    printf("Failed to init schema and functions.\n");
    goto DISPOSE_STATES;
  }

  switch (1) {
  case 1:
    // Attempt learning the "falls on" predictive concept.
    if (!stLearnConcept(
      &states, &entityFunctions, stChooseWhereNoneMoving
    )) {
      printf("No learned tree.\n");
      goto DROP_FUNCTIONS;
    }
    break;
  case 2:
    if (!stClusterStuff(&states, &entityFunctions)) {
      printf("Clustering failed.\n");
      goto DROP_FUNCTIONS;
    }
    break;
  default:
    printf("Didn't do anything!\n");
  }

  // We winned it all!
  status = EXIT_SUCCESS;

  DROP_FUNCTIONS:
  stDisposeSchemaAndEntityFunctions(&schema, &entityFunctions);

  DISPOSE_STATES:
  cnListEachBegin(&states, stState, state) {
    stStateDispose(state);
  } cnEnd;
  cnListDispose(&states);

  DONE:
  return status;
}


cnBool stClusterStuff(
  cnList(stState)* states, cnList(cnEntityFunction*)* functions
) {
  cnList(cnBag) bags;
  cnEntityFunction* function;
  cnBool result = cnFalse;

  // Choose out the states we want to focus on.
  cnListInit(&bags, sizeof(cnBag));
  if (!stAllBagsFalse(states, &bags)) {
    printf("Failed to choose bags.\n");
    goto DISPOSE_BAGS;
  }

  // The last function right now should be velocity. TODO Watch out for changes!
  // TODO Be more thorough about clustering. Try it all as for tree learning.
  function = *(cnEntityFunction**)cnListGet(functions, functions->count - 1);
  if (!cnClusterOnFunction(&bags, function)) {
    goto DISPOSE_BAGS;
  }

  // TODO Cluster!
  result = cnTrue;

  DISPOSE_BAGS:
  cnListEachBegin(&bags, cnBag, bag) {
    cnBagDispose(bag);
  } cnEnd;
  cnListDispose(&bags);

  return result;
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
  if (cnTrue) {
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
  }
  if (cnTrue) {
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
  }
  if (cnFalse) {
    // Velocity.
    // TODO Look up the property by name.
    if (!(function = cnPushPropertyFunction(
      functions, cnListGet(&itemType->properties, 2)
    ))) {
      printf("Failed to push velocity function.\n");
      goto FAIL;
    }
  }

  // We made it!
  return cnTrue;

  FAIL:
  // Clean it all up if we fail.
  stDisposeSchemaAndEntityFunctions(schema, functions);
  return cnFalse;
}


cnBool stLearnConcept(
  cnList(stState)* states,
  cnList(cnEntityFunction*)* functions,
  cnBool (*choose)(cnList(stState)* states, cnList(cnBag)* bags)
) {
  cnList(cnBag) bags;
  cnRootNode* learnedTree;
  cnLearner learner;
  cnBool result = cnFalse;
  cnRootNode stubTree;
  cnCount trueCount;

  // Choose out the states we want to focus on.
  cnListInit(&bags, sizeof(cnBag));
  if (!choose(states, &bags)) {
    printf("Failed to choose bags.\n");
    goto DISPOSE_BAGS;
  }
  trueCount = 0;
  cnListEachBegin(&bags, cnBag, bag) {
    trueCount += bag->label;
  } cnEnd;
  // TODO Also print mean number of items in chosen states?
  printf("%ld true of %ld bags\n", trueCount, bags.count);
  // Shuffle bags, with controlled seed (and my own generator?).
  // TODO Shuffle here copies more than just single pointers.
  cnListShuffle(&bags);

  // Set up the tree.
  if (!cnRootNodeInit(&stubTree, cnTrue)) {
    printf("Failed to init tree.\n");
    goto DISPOSE_BAGS;
  }
  stubTree.entityFunctions = functions;

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
    goto DISPOSE_LEARNER;
  }

  // We made it!
  // TODO Any stats?
  result = cnTrue;

  // Dispose of the learned tree.
  cnNodeDrop(&learnedTree->node);

  DISPOSE_LEARNER:
  cnLearnerDispose(&learner);

  DISPOSE_TREE:
  cnNodeDispose(&stubTree.node);

  DISPOSE_BAGS:
  cnListEachBegin(&bags, cnBag, bag) {
    cnBagDispose(bag);
  } cnEnd;
  cnListDispose(&bags);

  return result;
}
