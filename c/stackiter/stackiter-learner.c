#include <stdlib.h>


#include "stackiter-learner.h"


int main(int argc, char** argv) {
  cnList bags;
  stState* state;
  cnList states;
  int status = EXIT_FAILURE;
  cnRootNode tree;
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
    goto DISPOSE_SAMPLES;
  }
  trueCount = 0;
  cnListEachBegin(&bags, cnBag, bag) {
    trueCount += bag->label;
  } cnEnd;
  printf("%ld true of %ld samples\n", trueCount, bags.count);

  // Set up the tree.
  cnRootNodeInit(&tree);

  /*
    cout
      << "Mean items in chosen states: " << (totalItems/samples.size()) << endl
    ;

    // TODO Support copied and/or extended types???
    Type itemType(Entity2D::type());
    itemType.size = sizeof(Item);

    Learner learner(itemType);
    DifferenceFunction difference(itemType.system.$float().arrayType(2));
    // TODO Once we have multiple attributes, how do we get location easily?
    PointerFunction location(*itemType.attributes.front().get);
    ComposedFunction differenceLocation(difference, location);
    learner.functions.push_back(&location);
    learner.functions.push_back(&differenceLocation);

    // Simple test/profiling. TODO Try again with pointers support.
    //    Function& blah(difference);
    //    Float ab[] = {1.5, 3.0, 2.0, 0.3};
    //    Float c[2];
    //    for (Count i(0); i < 100000000; i++) {
    //      blah(ab, c);
    //      c[0] = ab[0] - ab[2];
    //      c[1] = ab[1] - ab[3];
    //    }
    //    cout << "Result: " << c[0] << ", " << c[1] << endl;

    learner.learn(samples);
  */

  status = EXIT_SUCCESS;

DISPOSE_TREE:

  cnRootNodeDispose(&tree);

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
