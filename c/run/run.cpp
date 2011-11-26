#include "run.h"

using namespace concuno;
using namespace concuno::run;
using namespace std;


int main(int argc, char** argv) {
  Args args(argc, argv);
  List<Bag> bags;
  ListAny features;
  AutoVec<EntityFunction*> functions;
  ListAny labels;
  RootNode* learnedTree = NULL;
  Learner learner;
  Schema schema;

  // Load all the data. Calling labels a "type" is abusive but works enough.
  Type* featureType =
    loadTable(args.featuresFile, "Feature", schema, &features);
  Type* labelType = loadTable(args.labelsFile, "Label", schema, &labels);

  // Build labeled bags.
  if (
    !buildBags(&bags, args.label, labelType, &labels, featureType, &features)
  ) throw Error("No bags.");
  // Choose some functions. TODO How to specify which??
  pickFunctions(*functions, featureType);
  printf("\n");

  // Learn something.
  cnListShuffle(&bags);
  learner.bags = &bags;
  learner.entityFunctions = &*functions;
  learnedTree = learner.learnTree();
  if (!learnedTree) throw Error("No learned tree.");

  // Display the learned tree.
  printf("\n");
  cnTreeWrite(learnedTree, cout);
  printf("\n");

  // All done. TODO RAII.
  cnNodeDrop(&learnedTree->node);
  cnBagListDispose(&bags, NULL);
  return EXIT_SUCCESS;
}
