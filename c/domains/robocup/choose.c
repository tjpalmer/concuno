#include "choose.h"


cnBool cnrChoosePasses(
  cnrGame game, cnList(cnBag)* bags, cnList(cnList(cnEntity)*)* entityLists
) {
  cnBool result = cnFalse;

  // Loop through states.
  cnListEachBegin(&game->states, struct cnrState, state) {
    // And players in those states to look for kicks.
    cnListEachBegin(&state->players, struct cnrPlayer, player) {
      if (!cnIsNaN(player->kickPower)) {
        printf("Kick at %ld by %ld/%ld.\n", state->time, player->team, player->index);
      }
    } cnEnd;
  } cnEnd;

  // Winned.
  result = cnTrue;

  // DONE:
  return result;
}
