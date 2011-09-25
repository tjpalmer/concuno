#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "load.h"


int main(int argc, char** argv) {
  char* name;
  cnList(struct cnrState) states;
  int result = EXIT_FAILURE;

  // Init first.
  cnListInit(&states, sizeof(struct cnrState));

  // Check args.
  if (argc < 2) cnFailTo(DONE, "No file specified.");

  // Load all the states in the game log.
  name = argv[1];
  if (!cnrLoadGameLog(&states, name)) {
    cnFailTo(DONE, "Failed to load: %s", name);
  }

  // Now look at the command log to see what actions were taken.
  // Assume the file is in the same place but named rcl instead of rcg.
  if (cnStrEndsWith(name, ".rcg")) {
    // Abusively just modify the arg. TODO Is this legit?
    cnIndex lastIndex = strlen(name) - 1;
    name[lastIndex] = 'l';
    printf("Wanting to read %s\n", name);
    // TODO cnrLoadCommandLog(&states, name);
  }

  // Report.
  printf("Loaded %ld states.\n", states.count);

  // Winned.
  result = EXIT_SUCCESS;

  DONE:
  cnListEachBegin(&states, struct cnrState, state) {
    cnrStateDispose(state);
  } cnEnd;
  cnListDispose(&states);
  return result;
}
