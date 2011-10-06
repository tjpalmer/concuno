#include "choose.h"


cnBool cnrChoosePasses(
  cnrGame game, cnList(cnBag)* bags, cnList(cnList(cnEntity)*)* entityLists
) {
  cnBool result = cnFalse;

  // Loop through states.
  cnListEachBegin(&game->states, struct cnrState, state) {
    // Look through players in each state to find kicks.
    cnrPlayer kickingPlayer = NULL;
    cnListEachBegin(&state->players, struct cnrPlayer, player) {
      if (player->team == cnrTeamKeepers && !cnIsNaN(player->kickPower)) {
        if (kickingPlayer) {
          cnFailTo(DONE, "Multiple kickers at %ld.", state->time);
        }
        kickingPlayer = player;
      }
    } cnEnd;

    // Look ahead to see whether this kick was a pass or a hold.
    if (kickingPlayer && state < end) {
      // Ooh. We have a next state.
      cnFloat* ballLocation = state->ball.item.location;
      cnrState next = state + 1;
      cnFloat* nextBallLocation = next->ball.item.location;
      cnFloat ballVelocity[] = {
        nextBallLocation[0] - ballLocation[0],
        nextBallLocation[1] - ballLocation[1]
      };
      cnFloat ballSpeed = sqrt(
        ballVelocity[0] * ballVelocity[0] + ballVelocity[1] * ballVelocity[1]);
      cnFloat ballDistance = cnEuclideanDistance(2, nextBallLocation, kickingPlayer->item.location);
      printf("%ld %ld %lg %lg %d\n", state->time, kickingPlayer->index, ballSpeed, ballDistance, ballSpeed <= 0.6 || ballDistance <= 0.6);
    }

    // Also look to see if the kick was "successful" in the sense that the
    // keepers still have the ball.
  } cnEnd;

  // Winned.
  result = cnTrue;

  DONE:
  return result;
}
