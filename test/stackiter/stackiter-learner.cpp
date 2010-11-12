#include "stackiter-learner.h"

using namespace cuncuno;
using namespace stackiter;
using namespace std;

int main(int argc, char** argv) {
  try {
    if (argc < 2) {
      throw "Please specify a data file.";
    }
    Loader loader;
    loader.load(argv[1]);
    Chooser chooser;
    vector<BoolSample> samples;
    chooser.chooseDropWhereLandOnOtherTrue(loader.states, samples);
    cout << "Items at end: " << loader.states.back().items.size() << endl;
    cout << "Total states: " << loader.states.size() << endl;
    cout << "Chosen states: " << samples.size() << endl;
    int totalPos(0);
    double totalItems(0);
    for (
      vector<BoolSample>::iterator s(samples.begin()); s != samples.end(); s++
    ) {
      if (s->value) {
        totalPos++;
      }
      totalItems += s->entities.size();
    }
    cout << "Positives: " << totalPos << endl;
    cout
      << "Mean items in chosen states: " << (totalItems/samples.size()) << endl
    ;
    learn(samples);
  } catch (const char* message) {
    cout << message << endl;
  }
}
