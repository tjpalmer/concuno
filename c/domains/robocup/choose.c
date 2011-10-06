#include "choose.h"


cnBool cnrExtractHoldOrPass(
  cnrGame game, cnrState state, cnrPlayer kicker,
  cnList(cnBag)* holdBags, cnList(cnBag)* passBags
);


cnBool cnrChooseHoldsAndPasses(
  cnrGame game,
  cnList(cnBag)* holdBags, cnList(cnBag)* passBags,
  cnList(cnList(cnEntity)*)* entityLists
) {
  cnBool result = cnFalse;

  // Loop through states.
  cnListEachBegin(&game->states, struct cnrState, state) {
    // Look through players in each state to find kicks.
    cnrPlayer kicker = NULL;
    cnListEachBegin(&state->players, struct cnrPlayer, player) {
      if (player->team == cnrTeamKeepers && !cnIsNaN(player->kickPower)) {
        if (kicker) {
          cnFailTo(DONE, "Multiple kickers at %ld.", state->time);
        }
        kicker = player;
      }
    } cnEnd;

    // Look ahead to see whether this kick was a pass or a hold.
    if (kicker && state < end) {
      // Ooh. We have a next state.
      if (!cnrExtractHoldOrPass(game, state, kicker, holdBags, passBags)) {
        cnFailTo(DONE, "No bag extracted.");
      }
    }

    // Also look to see if the kick was "successful" in the sense that the
    // keepers still have the ball.
  } cnEnd;

  // Winned.
  result = cnTrue;

  DONE:
  return result;
}


cnBool cnrExtractHoldOrPass(
  cnrGame game, cnrState state, cnrPlayer kicker,
  cnList(cnBag)* holdBags, cnList(cnBag)* passBags
) {
  cnBool result = cnFalse;
  cnFloat* ballLocation = state->ball.item.location;
  cnFloat* kickerLocation = kicker->item.location;
  cnrState next = state + 1;
  cnFloat* nextBallLocation = next->ball.item.location;
  cnFloat ballVelocity[] = {
    nextBallLocation[0] - ballLocation[0],
    nextBallLocation[1] - ballLocation[1]
  };
  cnFloat ballSpeed = cnNorm(2, ballVelocity);
  cnFloat ballDistance = cnEuclideanDistance(
    2, nextBallLocation, kickerLocation
  );

  //  printf(
  //    "%ld %ld %lg %lg %d\n",
  //    state->time, kicker->index,
  //    ballSpeed, ballDistance, ballSpeed <= 0.6 || ballDistance <= 0.6
  //  );
  if (ballSpeed > 0.6 && ballDistance > 0.6) {
    // Presume it's a pass for now. Find the recipient.
    cnRadian passAngle = atan2(ballVelocity[1], ballVelocity[0]);
    printf("Pass: time %ld, angle %lg", state->time, passAngle);
    cnListEachBegin(&state->players, struct cnrPlayer, player) {
      // Actually, for the same state, pointer equality for player and kicker
      // should be good enough, but check indices to be more general.
      if (player->index != kicker->index && player->team == cnrTeamKeepers) {
        cnFloat* playerLocation = player->item.location;
        cnFloat playerDelta[] = {
          playerLocation[0] - kickerLocation[0],
          playerLocation[1] - kickerLocation[1]
        };
        cnRadian playerAngle = atan2(playerDelta[1], playerDelta[0]);
        cnRadian diff = cnWrapRadians(playerAngle - passAngle);
        printf(", d%ld %lg (%lg %lg) ", player->index, diff, playerAngle, passAngle);
        //        if (kicker) {
        //          cnFailTo(DONE, "Multiple kickers at %ld.", state->time);
        //        }
      }
    } cnEnd;
    printf("\n");
  }

  // Winned.
  result = cnTrue;

  // DONE:
  return result;
}
