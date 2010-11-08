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
    vector<State*> pos;
    vector<State*> neg;
    chooser.chooseDropWhereLandOnOtherTrue(loader.states, pos, neg);
  } catch (const char* message) {
    cout << message << endl;
  }
}
