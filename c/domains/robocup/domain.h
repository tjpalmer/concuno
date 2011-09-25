#ifndef concuno_robocup_domain_h
#define concuno_robocup_domain_h


#include <concuno.h>


typedef enum {

  cnrTypeBall,

  cnrTypePlayer,

} cnrType;


/**
 * Usually just 0 for left (or keepers?) or 1 for right (or takers?).
 */
typedef cnIndex cnrTeam;


typedef cnIndex cnrTime;


typedef struct cnrItem {

  cnrType type;

  cnFloat location[2];

  // TODO Velocity.

}* cnrItem;


typedef struct cnrBall {

  struct cnrItem item;

  // Probably that's all we'll ever have in here.

}* cnrBall;


typedef struct cnrPlayer {

  struct cnrItem item;

  // TODO Actions indicated where?

  // TODO Explicit ball posession indicator?

  /**
   * Unique only by team.
   */
  cnIndex index;

  cnrTeam team;

}* cnrPlayer;


typedef struct cnrState {

  struct cnrBall ball;

  cnList(struct cnrPlayer) players;

  /**
   * The secondary clock of the game that ticks during setup or after fouls.
   */
  cnrTime subtime;

  /**
   * The primary clock of the game.
   */
  cnrTime time;

}* cnrState;


void cnrItemInit(cnrItem item, cnrType type);


void cnrPlayerInit(cnrPlayer player);


/**
 * Currently, after state disposal, it's actually reinit'd.
 * TODO Will this always hold?
 */
void cnrStateDispose(cnrState state);


void cnrStateInit(cnrState state);


#endif
