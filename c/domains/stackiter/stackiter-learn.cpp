#include <stdlib.h>

#include "stackiter-learn.h"


namespace ccndomain {namespace stackiter {


void clusterStuff(
  concuno::List<State>& states,
  std::vector<concuno::EntityFunction*>& functions
);


void initSchemaAndEntityFunctions(
  concuno::Schema& schema, std::vector<concuno::EntityFunction*>& functions
);


bool learnConcept(
  concuno::List<State>* states,
  std::vector<concuno::EntityFunction*>* functions,
  bool (*choose)(
    concuno::List<State>* states, concuno::List<concuno::Bag>* bags,
    concuno::List<concuno::List<concuno::Entity>*>* entityLists
  )
);


}}


using namespace ccndomain::stackiter;
using namespace concuno;
using namespace std;


int main(int argc, char** argv) {
  AutoVec<EntityFunction*> entityFunctions;
  Schema schema;
  List<State> states;
  int status = EXIT_FAILURE;

  // Validate args.
  if (argc < 2) {
    printf("No data file specified.\n");
    goto DONE;
  }

  // Load file and show stats.
  if (!load(argv[1], &states)) {
    throw Error("Failed to load file.");
  }
  printf("At end:\n");
  printf("%ld items\n", states[states.count - 1].items.count);
  printf("%ld states\n", states.count);

  // Set up schema.
  initSchemaAndEntityFunctions(schema, *entityFunctions);

  switch (1) {
  case 1:
    // Attempt learning the "falls on" predictive concept.
    if (!learnConcept(
      &states, &*entityFunctions, chooseDropWhereLandOnOther
    )) {
      throw Error("No learned tree.");
    }
    break;
  case 2:
    clusterStuff(states, *entityFunctions);
    break;
  default:
    printf("Didn't do anything!\n");
  }

  // We winned it all!
  status = EXIT_SUCCESS;

  DONE:
  cnListEachBegin(&states, State, state) {
    state->~State();
  } cnEnd;
  return status;
}


namespace ccndomain {namespace stackiter {


void clusterStuff(
  List<State>& states, vector<EntityFunction*>& functions
) {
  List<Bag> bags;

  // Choose out the states we want to focus on.
  if (!allBagsFalse(&states, &bags, NULL)) {
    throw Error("Failed to choose bags.");
  }

  // The last function right now should be velocity. TODO Watch out for changes!
  // TODO Be more thorough about clustering. Try it all as for tree learning.
  EntityFunction* function = functions[functions.size() - 1];
  if (!cnClusterOnFunction(&bags, function)) {
    throw Error("Clustering failed.");
  }

  // TODO Auto!
  cnListEachBegin(&bags, Bag, bag) {
    bag->~Bag();
  } cnEnd;
}


void initSchemaAndEntityFunctions(
  Schema& schema, vector<EntityFunction*>& functions
) {
  Type* itemType;

  // Set up schema.
  schemaInit(schema);

  // Set up entity functions.
  // TODO Extract this setup, and make it easier to do.
  // TODO Look up the type by name.
  itemType = schema.types[1];

  // Valid.
  if (true) {
    (new ValidityEntityFunction(schema, 1))->pushOrDelete(functions);
    (new ValidityEntityFunction(schema, 2))->pushOrDelete(functions);
  }

  // Color.
  if (true) {
    Property& property = *itemType->properties[0];
    EntityFunction* function = new PropertyEntityFunction(property);
    function->pushOrDelete(functions);
    // DifferenceColor
    (new DifferenceEntityFunction(*function))->pushOrDelete(functions);
  }

  // Location.
  if (true) {
    // TODO Look up the property by name.
    Property& property = *itemType->properties[1];
    EntityFunction* function = new PropertyEntityFunction(property);
    function->pushOrDelete(functions);
    // Difference and distance (in that order for now).
    (new DifferenceEntityFunction(*function))->pushOrDelete(functions);
    (new DistanceEntityFunction(*function))->pushOrDelete(functions);
  }

  // Velocity.
  if (false) {
    // TODO Look up the property by name.
    Property& property = *itemType->properties[2];
    EntityFunction* function = new PropertyEntityFunction(property);
    function->pushOrDelete(functions);
  }
}


bool learnConcept(
  List<State>* states,
  vector<EntityFunction*>* functions,
  bool (*choose)(
    List<State>* states, List<Bag>* bags,
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
  learner.entityFunctions = &*functions;
  learnedTree = learner.learnTree();
  if (!learnedTree) cnErrTo(DONE, "No learned tree.");

  // Display the learned tree.
  printf("\n");
  cnTreeWrite(learnedTree, cout);
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


}}
