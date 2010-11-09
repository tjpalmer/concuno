#include "stackiter-learner.h"

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
    vector<BooleanItemSample> samples;
    chooser.chooseDropWhereLandOnOtherTrue(loader.states, samples);
    cout << "Items at end: " << loader.states.back().items.size() << endl;
    cout << "Total states: " << loader.states.size() << endl;
    cout << "Settled drops: " << samples.size() << endl;
  } catch (const char* message) {
    cout << message << endl;
  }
}
