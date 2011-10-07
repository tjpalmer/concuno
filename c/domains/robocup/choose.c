#include "choose.h"


cnBool cnrExtractHoldOrPass(
  cnrGame game, cnrState state, cnrPlayer kicker,
  cnList(cnBag)* holdBags, cnList(cnBag)* passBags,
  cnList(cnList(cnEntity)*)* entityLists
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
      if (!cnrExtractHoldOrPass(
        game, state, kicker, holdBags, passBags, entityLists
      )) cnFailTo(DONE, "No bag extracted.");
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
  cnList(cnBag)* holdBags, cnList(cnBag)* passBags,
  cnList(cnList(cnEntity)*)* entityLists
) {
  cnBool result = cnFalse;
  cnBag* bag = NULL;
  cnrPlayer receiver = NULL;
  cnrBall ball = &state->ball;
  cnFloat* ballLocation = ball->item.location;
  cnList(cnEntity)* entities;
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

  // We'll need an entity list for the state, whether it's a hold or a pass.
  if (!(entities = cnListCreate(sizeof(cnEntity)))) {
    cnFailTo(DONE, "No entities.");
  }
  // Push the ball.
  if (!cnListPush(entities, &ball)) cnFailTo(DONE, "No ball for list.");
  // And the players.
  cnListEachBegin(&state->players, struct cnrPlayer, player) {
    if (!cnListPush(entities, &player)) {
      cnFailTo(DONE, "No player for entity list.");
    }
  } cnEnd;
  if (!cnListPush(entityLists, &entities)) {
    // Clean up before fail.
    cnListDestroy(entities);
    cnFailTo(DONE, "Can't push entities list.");
  }

  // Now see if we have a pass or a hold.
  // Kickable margin is commonly 0.7 units, so balls past about this range are
  // probably because of passes. Be a little conservative on holds, however,
  // because we have another round of auditing at the pass level, based on
  // whether the ball actually seems to be going toward a team member.
  if (ballSpeed > 0.6 && ballDistance > 0.6) {
    // Presume it's a pass for now. Find the receiver, if within pi/10.
    cnRadian minDiff = 0.1 * cnPi;
    cnRadian passAngle = atan2(ballVelocity[1], ballVelocity[0]);
    cnListEachBegin(&state->players, struct cnrPlayer, player) {
      // Actually, for the same state, pointer equality for player and kicker
      // should be good enough, but check indices to be more general.
      if (player->index != kicker->index && player->team == kicker->team) {
        cnFloat* playerLocation = player->item.location;
        cnFloat playerDelta[] = {
          playerLocation[0] - kickerLocation[0],
          playerLocation[1] - kickerLocation[1]
        };
        cnRadian playerAngle = atan2(playerDelta[1], playerDelta[0]);
        cnRadian diff = fabs(cnWrapRadians(playerAngle - passAngle));
        if (diff < minDiff) {
          minDiff = diff;
          receiver = player;
        }
      }
    } cnEnd;
    if (receiver) {
      bag = cnListExpand(passBags);
      if (!bag) cnFailTo(DONE, "No pass bag pushed.");
      // With provided entities, bag init doesn't fail.
      cnBagInit(bag, entities);
      printf("Pass: time %ld, keeper %ld\n", state->time, receiver->index);
    }
  }

  // See if it's a hold.
  if (!bag) {
    // Must be, since we didn't already define a bag for passing.
    // TODO Work this.
    result = cnTrue;
    goto DONE;
  }

  // We should have a bag by now. Pin the participants.
  if (!cnBagPushParticipant(bag, 0, kicker)) {
    cnFailTo(DONE, "Failed to push kicker.");
  }
  if (receiver) {
    // Pin the receiver, too.
    if (!cnBagPushParticipant(bag, 1, receiver)) {
      cnFailTo(DONE, "Failed to push receiver.");
    }
  }

  // And provide a bag label, looking ahead for success or failure.
  // TODO Need more info from parsing?

  // Winned.
  result = cnTrue;

  DONE:
  return result;
}
