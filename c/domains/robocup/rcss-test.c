#include <stdio.h>
#include <stdlib.h>

#include "load.h"


int main(int argc, char** argv) {
  int result = EXIT_FAILURE;
  if (argc < 2) cnFailTo(DONE, "No file specified.");

  // Just test loading for now.
  if (!cnrLoadGameLog(argv[1])) cnFailTo(DONE, "Failed to load: %s", argv[1]);

  // Winned.
  result = EXIT_SUCCESS;

  DONE:
  return result;
}
