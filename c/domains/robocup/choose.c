#include "choose.h"


/**
 * Only call this function if there is a next state available, too.
 */
cnBool cnrExtractHoldOrPass(
  cnrGame game, cnrState state, cnrPlayer kicker,
  cnList(cnBag)* holdBags, cnList(cnBag)* passBags,
  cnList(cnList(cnEntity)*)* entityLists,
  cnBool label
);


cnBool cnrChooseHoldsAndPasses(
  cnrGame game,
  cnList(cnBag)* holdBags, cnList(cnBag)* passBags,
  cnList(cnList(cnEntity)*)* entityLists
) {
  // Assume that failure withing a certain number of timesteps means the action
  // was a bad choice. This isn't always true. Holding might be good but the
  // ball is lost for a bad later pass. On the other hand, when holding really
  // is a problem, that problem does need to precede loss of the ball by a
  // certain amount. Better heuristics might improve labeling, but that might
  // also be too much assistance. What's best?
  cnCount failureLookahead = 8;
  cnBool result = cnFalse;

  // Loop through states.
  cnListEachBegin(&game->states, struct cnrState, state) {
    // Look through players in each state to find kicks.
    cnrPlayer kicker = NULL;
    //    if (state->newSession) {
    //      printf("New session at %ld.\n", state->time);
    //    }
    cnListEachBegin(&state->players, struct cnrPlayer, player) {
      if (player->team == cnrTeamKeepers && !cnIsNaN(player->kickPower)) {
        if (kicker) {
          cnErrTo(DONE, "Multiple kickers at %ld.", state->time);
        }
        kicker = player;
      }
    } cnEnd;

    // If we have a kick and enough future, we can label this state. However,
    // don't label for any immediate switch to a new sessions. Without looking
    // at the kick action itself, we can't infer what action was intended.
    //
    // For this reason and also because of trouble interpreting effects when
    // multiple kickers (including takers) are involved, we might want to move
    // to interpreting the kick parameters themselves instead of looking at
    // resulting ball activity.
    //
    if (kicker && state + failureLookahead < end && !(state + 1)->newSession) {
      // Ooh. We have a next state. Look even further to see if keepers keep.
      cnBool anyLaterKick = cnFalse;
      cnBool label = cnTrue;
      cnrState next;
      cnrState lookaheadEnd = state + failureLookahead;
      for (next = state + 1; next < end; next++) {
        if (!anyLaterKick) {
          cnListEachBegin(&next->players, struct cnrPlayer, player) {
            if (player->team == cnrTeamKeepers && !cnIsNaN(player->kickPower)) {
              // Found another keeper kick. Still a positive label.
              anyLaterKick = cnTrue;
              break;
            }
          } cnEnd;
        }
        if (next->newSession) {
          // Nope. We failed too soon. Must have made a bad choice.
          label = cnFalse;
          break;
        }
        if (anyLaterKick && next >= lookaheadEnd) {
          // We've seen another kick from our team, and we've still not ended
          // the session, so call it good.
          //
          // TODO Better would be to track how soon we our action is before the
          // TODO failing action, but this will do for now.
          //
          break;
        }
      }
      // Now that all that label determination is over, let's build the scene.
      if (!cnrExtractHoldOrPass(
        game, state, kicker, holdBags, passBags, entityLists, label
      )) cnErrTo(DONE, "No bag extracted.");
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
  cnList(cnList(cnEntity)*)* entityLists,
  cnBool label
) {
  cnBool result = cnFalse;
  cnBag* bag = NULL;
  cnrPlayer receiver = NULL;
  cnrBall ball = &state->ball;
  cnFloat* ballLocation = ball->item.location;
  cnList(cnEntity)* entities;
  cnBool kickedAgain = cnFalse;
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
    cnErrTo(DONE, "No entities.");
  }
  // Push the ball.
  if (!cnListPush(entities, &ball)) cnErrTo(DONE, "No ball for list.");
  // And the players.
  cnListEachBegin(&state->players, struct cnrPlayer, player) {
    if (!cnListPush(entities, &player)) {
      cnErrTo(DONE, "No player for entity list.");
    }
  } cnEnd;
  if (!cnListPush(entityLists, &entities)) {
    // Clean up before fail.
    cnListDestroy(entities);
    cnErrTo(DONE, "Can't push entities list.");
  }

  // Now see if we have a pass or a hold. First see if the same player kicks
  // again at the next time step.
  cnListEachBegin(&next->players, struct cnrPlayer, player) {
    if (player->index == kicker->index && player->team == kicker->team) {
      // Same player. Do we kick?
      if (!cnIsNaN(player->kickPower)) {
        kickedAgain = cnTrue;
        break;
      }
    }
  } cnEnd;

  // If we didn't kick immediately again, but otherise kick fast, see if it's
  // a pass. Kickable margin is commonly 0.7 units, so balls past about this
  // range are probably because of passes. Be a little conservative on holds,
  // however, because we have another round of auditing at the pass level, based
  // on whether the ball actually seems to be going toward a team member.
  if (!kickedAgain && ballSpeed > 0.6 && ballDistance > 0.6) {
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
      if (!(bag = cnListExpand(passBags))) {
        cnErrTo(DONE, "No pass bag pushed.");
      }
      // With provided entities, bag init doesn't fail.
      cnBagInit(bag, entities);
      //      printf(
      //        "Pass at %ld by %ld to %ld.%s\n",
      //        state->time, kicker->index, receiver->index, label ? "" : " :("
      //      );
    }
  }

  // See if it's a hold.
  if (!bag) {
    // Must be, since we didn't already define a bag for passing.
    if (!(bag = cnListExpand(holdBags))) {
      cnErrTo(DONE, "No pass bag pushed.");
    }
    // With provided entities, bag init doesn't fail.
    cnBagInit(bag, entities);
    //    printf(
    //      "Hold at %ld by %ld.%s\n",
    //      state->time, kicker->index, label ? "" : " :("
    //    );
  }

  // We should have a bag by now. Pin the participants.
  if (!cnBagPushParticipant(bag, 0, kicker)) {
    cnErrTo(DONE, "Failed to push kicker.");
  }
  if (receiver) {
    // Pin the receiver, too.
    if (!cnBagPushParticipant(bag, 1, receiver)) {
      cnErrTo(DONE, "Failed to push receiver.");
    }
  }

  // And provide the given bag label already determined before the call.
  bag->label = !label;

  // Winned.
  result = cnTrue;

  DONE:
  return result;
}
