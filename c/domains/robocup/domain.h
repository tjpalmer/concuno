#ifndef concuno_robocup_domain_h
#define concuno_robocup_domain_h


#include <concuno.h>


typedef enum {

  cnrTypeBall,

  cnrTypePlayer,

} cnrType;


/**
 * Index rather than enum because I want to use it as an index. In the abstract,
 * although perhaps not in RoboCup, there also could technically be more than
 * two teams.
 */
typedef cnIndex cnrTeam;

#define cnrTeamLeft 0
#define cnrTeamRight 1

// Are keepers (in keepaway) the left team?
#define cnrTeamKeepers 0
#define cnrTeamTakers 1


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

  // TODO Better action representation?

  /**
   * Use NaN for no kick.
   *
   * TODO Units?
   */
  cnFloat kickAngle;

  /**
   * Use NaN for no kick.
   *
   * TODO Units?
   */
  cnFloat kickPower;

  // TODO Explicit ball posession indicator?

  /**
   * Unique only by team.
   */
  cnIndex index;

  /**
   * Body angle.
   *
   * TODO What units?
   */
  cnFloat orientation;

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


typedef struct cnrGame {

  cnList(struct cnrState) states;

  cnList(cnString) teamNames;

}* cnrGame;


void cnrGameDispose(cnrGame game);


void cnrGameInit(cnrGame game);


void cnrItemInit(cnrItem item, cnrType type);


void cnrPlayerInit(cnrPlayer player);


/**
 * Currently, after state disposal, it's actually reinit'd.
 * TODO Will this always hold?
 */
void cnrStateDispose(cnrState state);


void cnrStateInit(cnrState state);


#endif
