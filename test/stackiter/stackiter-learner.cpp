#include "stackiter-learner.h"

int main(int argc, char** argv) {
  try {
    if (argc < 2) {
      throw "Please specify a data file.";
    }
    Loader loader;
    loader.load(argv[1]);
  } catch (const char* message) {
    cout << message << endl;
  }
}
