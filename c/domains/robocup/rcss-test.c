#include <stdio.h>
#include <stdlib.h>

#include "load.h"


int main(int argc, char** argv) {
  cnList(struct cnrState) states;
  int result = EXIT_FAILURE;

  // Init first.
  cnListInit(&states, sizeof(struct cnrState));

  // Check args.
  if (argc < 2) cnFailTo(DONE, "No file specified.");

  // Just test loading for now.
  if (!cnrLoadGameLog(&states, argv[1])) {
    cnFailTo(DONE, "Failed to load: %s", argv[1]);
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
