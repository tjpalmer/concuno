#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "choose.h"
#include "load.h"


cnBool cnrExtractInfo(cnrGame game);


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
  cnList(cnBag) bags;
  cnList(cnList(cnEntity)*) entityLists;
  cnBool result = cnFalse;

  // Init for safety.
  cnListInit(&bags, sizeof(cnBag));
  cnListInit(&entityLists, sizeof(cnList(cnEntity)*));

  cnrChoosePasses(game, &bags, &entityLists);
  // TODO Export stuff.

  // Winned.
  result = cnTrue;

  // DONE:
  // TODO These disposals are duplicated from stackiter code. Centralize them!
  // Bags.
  cnListEachBegin(&bags, cnBag, bag) {
    // Dispose the bag, but first hide entities if we manage them separately.
    if (entityLists.count) bag->entities = NULL;
    cnBagDispose(bag);
  } cnEnd;
  cnListDispose(&bags);
  // Lists in the bags.
  cnListEachBegin(&entityLists, cnList(cnEntity)*, list) {
    cnListDispose(*list);
    free(*list);
  } cnEnd;
  cnListDispose(&entityLists);
  // Result.
  return result;
}
