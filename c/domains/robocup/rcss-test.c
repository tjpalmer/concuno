#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "choose.h"
#include "load.h"


cnBool cnrExtractInfo(cnrGame game);


cnBool cnrSaveBags(char* name, cnList(cnBag)* bags);


int main(int argc, char** argv) {
  struct cnrGame game;
  char* name;
  int result = EXIT_FAILURE;

  // Init first.
  cnrGameInit(&game);

  // Check args.
  if (argc < 2) cnFailTo(DONE, "No file specified.");

  // Load all the states in the game log.
  // TODO Check if rcl or rcg to work either way.
  name = argv[1];
  if (!cnrLoadGameLog(&game, name)) cnFailTo(DONE, "Failed to load: %s", name);

  // Show all team names.
  cnListEachBegin(&game.teamNames, cnString, name) {
    printf("Team: %s\n", cnStr(name));
  } cnEnd;

  // Now look at the command log to see what actions were taken.
  // Assume the file is in the same place but named rcl instead of rcg.
  if (cnStrEndsWith(name, ".rcg")) {
    // Abusively just modify the arg. TODO Is this legit?
    cnIndex lastIndex = strlen(name) - 1;
    name[lastIndex] = 'l';
    printf("Trying also to read %s\n", name);
    if (!cnrLoadCommandLog(&game, name)) {
      cnFailTo(DONE, "Failed to load: %s", name);
    }
  }

  // Report.
  printf("Loaded %ld states.\n", game.states.count);

  // TODO Extract actions of hold or kick to player.
  if (!cnrExtractInfo(&game)) cnFailTo(DONE, "Failed extract.");

  // Winned.
  result = EXIT_SUCCESS;

  DONE:
  cnrGameDispose(&game);
  return result;
}


cnBool cnrExtractInfo(cnrGame game) {
  cnList(cnBag) holdBags;
  cnList(cnList(cnEntity)*) entityLists;
  cnList(cnBag) passBags;
  cnBool result = cnFalse;

  // Init for safety.
  cnListInit(&holdBags, sizeof(cnBag));
  cnListInit(&passBags, sizeof(cnBag));
  cnListInit(&entityLists, sizeof(cnList(cnEntity)*));

  if (!cnrChooseHoldsAndPasses(game, &holdBags, &passBags, &entityLists)) {
    cnFailTo(DONE, "Choose failed.");
  }

  // Export stuff.
  // TODO Allow specifying file names? Choose automatically by date?
  // TODO Option for learning in concuno rather than saving?
  if (!cnrSaveBags("keepaway-hold-bags.json", &holdBags)) {
    cnFailTo(DONE, "Failed saving holds.")
  }
  if (!cnrSaveBags("keepaway-pass-bags.json", &passBags)) {
    cnFailTo(DONE, "Failed saving passes.");
  }

  // Winned.
  result = cnTrue;

  DONE:
  // Bags and entities. The entity lists get disposed with the first call,
  // leaving bogus pointers inside the passBags, but they'll be unused.
  cnBagListDispose(&holdBags, &entityLists);
  cnBagListDispose(&passBags, &entityLists);
  // Result.
  return result;
}


cnBool cnrSaveBags(char* name, cnList(cnBag)* bags) {
  FILE* file = NULL;
  cnBool result = cnFalse;

  printf("Writing %s\n", name);
  if (!(file = fopen(name, "w"))) cnFailTo(DONE, "Failed to open: %s", name);

  cnListEachBegin(bags, cnBag, bag) {
    // TODO Export bag.
  } cnEnd;

  // Winned.
  result = cnTrue;

  DONE:
  if (file) fclose(file);
  return result;
}
