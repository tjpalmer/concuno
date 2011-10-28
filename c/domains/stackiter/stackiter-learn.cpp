#include <stdlib.h>

#include "stackiter-learn.h"

using namespace concuno;


bool stClusterStuff(
  cnList(stState)* states, cnList(EntityFunction*)* functions
);


void stDisposeSchemaAndEntityFunctions(
  Schema* schema, cnList(EntityFunction*)* functions
);


bool stInitSchemaAndEntityFunctions(
  Schema* schema, cnList(EntityFunction*)* functions
);


bool stLearnConcept(
  cnList(stState)* states,
  cnList(EntityFunction*)* functions,
  bool (*choose)(
    cnList(stState)* states, cnList(Bag)* bags,
    cnList(cnList(cnEntity)*)* entityLists
  )
);


int main(int argc, char** argv) {
  cnList(EntityFunction*) entityFunctions;
  Schema schema;
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
  state = reinterpret_cast<stState*>(cnListGet(&states, states.count - 1));
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
      &states, &entityFunctions, stChooseDropWhereLandOnOther
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


bool stClusterStuff(
  cnList(stState)* states, cnList(EntityFunction*)* functions
) {
  cnList(Bag) bags;
  EntityFunction* function;
  bool result = false;

  // Choose out the states we want to focus on.
  cnListInit(&bags, sizeof(Bag));
  if (!stAllBagsFalse(states, &bags, NULL)) {
    cnErrTo(DISPOSE_BAGS, "Failed to choose bags.");
  }

  // The last function right now should be velocity. TODO Watch out for changes!
  // TODO Be more thorough about clustering. Try it all as for tree learning.
  function = *(EntityFunction**)cnListGet(functions, functions->count - 1);
  if (!cnClusterOnFunction(&bags, function)) {
    goto DISPOSE_BAGS;
  }

  // TODO Cluster!
  result = true;

  DISPOSE_BAGS:
  cnListEachBegin(&bags, Bag, bag) {
    bag->dispose();
  } cnEnd;
  cnListDispose(&bags);

  return result;
}


void stDisposeSchemaAndEntityFunctions(
  Schema* schema, cnList(EntityFunction*)* functions
) {
  // Drop the functions.
  cnListEachBegin(functions, EntityFunction*, function) {
    cnEntityFunctionDrop(*function);
  } cnEnd;
  // Dispose of the list and schema.
  cnListDispose(functions);
  cnSchemaDispose(schema);
}


bool stInitSchemaAndEntityFunctions(
  Schema* schema, cnList(EntityFunction*)* functions
) {
  EntityFunction* function;
  Type* itemType;

  // Set up schema.
  if (!stSchemaInit(schema)) {
    printf("Failed to init schema.\n");
    // Nothing to clean up yet.
    return false;
  }

  // Set up entity functions.
  // TODO Extract this setup, and make it easier to do.
  cnListInit(functions, sizeof(EntityFunction*));
  // TODO Look up the type by name.
  itemType = reinterpret_cast<Type*>(cnListGetPointer(&schema->types, 1));

  // Valid.
  if (true) {
    if (!cnPushValidFunction(functions, schema, 1)) {
      cnErrTo(FAIL, "Failed to push Valid1.");
    }
    if (!cnPushValidFunction(functions, schema, 2)) {
      cnErrTo(FAIL, "Failed to push Valid2.");
    }
  }

  // Color.
  if (true) {
    if (!(function = cnPushPropertyFunction(
      functions,
      reinterpret_cast<Property*>(cnListGet(&itemType->properties, 0))
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

  // Location.
  if (true) {
    // TODO Look up the property by name.
    if (!(function = cnPushPropertyFunction(
      functions,
      reinterpret_cast<Property*>(cnListGet(&itemType->properties, 1))
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

  // Velocity.
  if (false) {
    // TODO Look up the property by name.
    if (!(function = cnPushPropertyFunction(
      functions,
      reinterpret_cast<Property*>(cnListGet(&itemType->properties, 2))
    ))) {
      printf("Failed to push velocity function.\n");
      goto FAIL;
    }
  }

  // We made it!
  return true;

  FAIL:
  // Clean it all up if we fail.
  stDisposeSchemaAndEntityFunctions(schema, functions);
  return false;
}


bool stLearnConcept(
  cnList(stState)* states,
  cnList(EntityFunction*)* functions,
  bool (*choose)(
    cnList(stState)* states, cnList(Bag)* bags,
    cnList(cnList(cnEntity)*)* entityLists
  )
) {
  cnList(Bag) bags;
  cnList(cnList(cnEntity)*) entityLists;
  RootNode* learnedTree = NULL;
  Learner learner;
  bool result = false;
  Count trueCount;

  // Init for safety.
  cnListInit(&bags, sizeof(Bag));
  cnListInit(&entityLists, sizeof(cnList(cnEntity)*));

  // Choose out the states we want to focus on.
  if (!choose(states, &bags, &entityLists)) {
    cnErrTo(DONE, "Failed to choose bags.");
  }
  trueCount = 0;
  cnListEachBegin(&bags, Bag, bag) {
    trueCount += bag->label;
  } cnEnd;
  // TODO Also print mean number of items in chosen states?
  printf("%ld true of %ld bags\n", trueCount, bags.count);
  // Shuffle bags, with controlled seed (and my own generator?).
  // TODO Shuffle here copies more than just single pointers.
  cnListShuffle(&bags);

  // Learn a tree.
  learner.bags = &bags;
  learner.entityFunctions = functions;
  learnedTree = learner.learnTree();
  if (!learnedTree) cnErrTo(DONE, "No learned tree.");

  // Display the learned tree.
  printf("\n");
  cnTreeWrite(learnedTree, stdout);
  printf("\n");

  // We made it!
  // TODO Any stats?
  result = true;

  // Dispose of the learned tree.
  cnNodeDrop(&learnedTree->node);

  DONE:
  // Bags and entities.
  cnBagListDispose(&bags, entityLists.count ? &entityLists : NULL);
  // Result.
  return result;
}
