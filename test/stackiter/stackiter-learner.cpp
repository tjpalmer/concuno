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
  } catch (const char* message) {
    cout << message << endl;
  }
}
