#include "stackiter-learner.h"

using namespace cuncuno;
using namespace stackiter;
using namespace std;

int main(int argc, char** argv) {
  try {

    // Validate args.
    if (argc < 2) {
      throw "Please specify a data file.";
    }

    // Load file and show stats.
    Loader loader;
    loader.load(argv[1]);
    cout << "Items at end: " << loader.states.back().items.size() << endl;
    cout << "Total states: " << loader.states.size() << endl;

    // Choose and label some samples.
    Chooser chooser;
    vector<Sample> samples;
    chooser.chooseDropWhereLandOnOtherTrue(loader.states, samples);

    cout << "Chosen states: " << samples.size() << endl;
    int totalPos(0);
    double totalItems(0);
    for (
      vector<Sample>::iterator s(samples.begin()); s != samples.end(); s++
    ) {
      if (s->label) {
        totalPos++;
      }
      totalItems += s->entities.size();
    }
    cout << "Positives: " << totalPos << endl;
    cout
      << "Mean items in chosen states: " << (totalItems/samples.size()) << endl
    ;

    Type itemType(Entity2D::type());
    itemType.size = sizeof(Item);

    Learner learner(itemType);
    DifferenceFunction difference(itemType.system.$float().arrayType(2));
    // TODO Once we have multiple attributes, how do we get location easily?
    Function& location(*itemType.attributes.front().get);
    ComposedFunction differenceLocation(difference, location);

    // Totally flaky:
    //    cout << "Item count: " << loader.states[3].items.size() << endl;
    //    Item* items[2] = {&loader.states[3].items[2], &loader.states[3].items[11]};
    //    Float itemLoc[2];
    //    location(items[0], itemLoc);
    //    cout << "Item at " << itemLoc[0] << ", " << itemLoc[1] << endl;
    //    location(items[1], itemLoc);
    //    cout << "Item at " << itemLoc[0] << ", " << itemLoc[1] << endl;
    //    differenceLocation(items, itemLoc);
    //    cout << "Diff =  " << itemLoc[0] << ", " << itemLoc[1] << endl;

    // Simple test/profiling.
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

  } catch (const char* message) {
    cout << message << endl;
  }
}
