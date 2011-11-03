#include <stdlib.h>

#include "stackiter-learn.h"

using namespace concuno;


void stClusterStuff(List<stState>& states, List<EntityFunction*>& functions);


void stDisposeEntityFunctions(List<EntityFunction*>* functions);


bool stInitSchemaAndEntityFunctions(
  Schema* schema, List<EntityFunction*>* functions
);


bool stLearnConcept(
  List<stState>* states,
  List<EntityFunction*>* functions,
  bool (*choose)(
    List<stState>* states, List<Bag>* bags,
    List<List<Entity>*>* entityLists
  )
);


int main(int argc, char** argv) {
  List<EntityFunction*> entityFunctions;
  Schema schema;
  List<stState> states;
  int status = EXIT_FAILURE;

  // Validate args.
  if (argc < 2) {
    printf("No data file specified.\n");
    goto DONE;
  }

  // Load file and show stats.
  if (!stLoad(argv[1], &states)) {
    printf("Failed to load file.\n");
    goto DISPOSE_STATES;
  }
  printf("At end:\n");
  printf("%ld items\n", states[states.count - 1].items.count);
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
    stClusterStuff(states, entityFunctions);
    break;
  default:
    printf("Didn't do anything!\n");
  }

  // We winned it all!
  status = EXIT_SUCCESS;

  DROP_FUNCTIONS:
  stDisposeEntityFunctions(&entityFunctions);

  DISPOSE_STATES:
  cnListEachBegin(&states, stState, state) {
    state->~stState();
  } cnEnd;

  DONE:
  return status;
}


void stClusterStuff(List<stState>& states, List<EntityFunction*>& functions) {
  List<Bag> bags;

  // Choose out the states we want to focus on.
  if (!stAllBagsFalse(&states, &bags, NULL)) {
    throw Error("Failed to choose bags.");
  }

  // The last function right now should be velocity. TODO Watch out for changes!
  // TODO Be more thorough about clustering. Try it all as for tree learning.
  EntityFunction* function = functions[functions.count - 1];
  if (!cnClusterOnFunction(&bags, function)) {
    throw Error("Clustering failed.");
  }

  // TODO Auto!
  cnListEachBegin(&bags, Bag, bag) {
    bag->~Bag();
  } cnEnd;
}


void stDisposeEntityFunctions(List<EntityFunction*>* functions) {
  // Drop the functions.
  cnListEachBegin(functions, EntityFunction*, function) {
    cnEntityFunctionDrop(*function);
  } cnEnd;
}


bool stInitSchemaAndEntityFunctions(
  Schema* schema, List<EntityFunction*>* functions
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
    if (!(
      function = cnPushPropertyFunction(functions, itemType->properties[0])
    )) {
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
    if (!(
      function = cnPushPropertyFunction(functions, itemType->properties[1])
    )) {
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
    if (!(
      function = cnPushPropertyFunction(functions, itemType->properties[2])
    )) {
      printf("Failed to push velocity function.\n");
      goto FAIL;
    }
  }

  // We made it!
  return true;

  FAIL:
  // Clean it all up if we fail.
  stDisposeEntityFunctions(functions);
  return false;
}


bool stLearnConcept(
  List<stState>* states,
  List<EntityFunction*>* functions,
  bool (*choose)(
    List<stState>* states, List<Bag>* bags,
    List<List<Entity>*>* entityLists
  )
) {
  List<Bag> bags;
  List<List<Entity>*> entityLists;
  RootNode* learnedTree = NULL;
  Learner learner;
  bool result = false;
  Count trueCount;

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
