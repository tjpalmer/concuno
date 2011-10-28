#ifndef concuno_robocup_domain_h
#define concuno_robocup_domain_h


#include <concuno.h>


enum cnrType {

  /**
   * The matter in question works for any type of cnrItem.
   */
  cnrTypeAny,

  cnrTypeBall,

  cnrTypePlayer,

};


/**
 * Index rather than enum because I want to use it as an index. In the abstract,
 * although perhaps not in RoboCup, there also could technically be more than
 * two teams.
 */
typedef concuno::Index cnrTeam;

#define cnrTeamLeft 0
#define cnrTeamRight 1

// Are keepers (in keepaway) the left team?
#define cnrTeamKeepers 0
#define cnrTeamTakers 1


typedef concuno::Index cnrTime;


struct cnrItem {

  cnrType type;

  concuno::Float location[2];

  // TODO Velocity.

};


struct cnrBall {

  cnrItem item;

  // Probably that's all we'll ever have in here.

};


struct cnrPlayer {

  struct cnrItem item;

  // TODO Better action representation?

  /**
   * Use NaN for no kick.
   *
   * TODO Units?
   */
  concuno::Float kickAngle;

  /**
   * Use NaN for no kick.
   *
   * TODO Units?
   */
  concuno::Float kickPower;

  // TODO Explicit ball posession indicator?

  /**
   * Unique only by team.
   */
  concuno::Index index;

  /**
   * Body angle.
   *
   * TODO What units?
   */
  concuno::Float orientation;

  cnrTeam team;

};


struct cnrState {

  cnrBall ball;

  /**
   * This state is the beginning of a new session. In keepaway, that means the
   * previous state signaled loss of possession.
   *
   * TODO For soccer, could this represent resumed play after certain events?
   */
  bool newSession;

  concuno::cnList(cnrPlayer) players;

  /**
   * The secondary clock of the game that ticks during setup or after fouls.
   */
  cnrTime subtime;

  /**
   * The primary clock of the game.
   */
  cnrTime time;

};


struct cnrGame {

  concuno::cnList(cnrState) states;

  concuno::cnList(cnString) teamNames;

};


void cnrGameDispose(cnrGame* game);


void cnrGameInit(cnrGame* game);


void cnrItemInit(cnrItem* item, cnrType type);


void cnrPlayerInit(cnrPlayer* player);


/**
 * Inits the schema from scratch to have everything needed for current rcss
 * needs.
 *
 * Dispose with cnSchemaDispose (since nothing special needed here).
 */
bool cnrSchemaInit(concuno::Schema* schema);


/**
 * Currently, after state disposal, it's actually reinit'd.
 * TODO Will this always hold?
 */
void cnrStateDispose(cnrState* state);


void cnrStateInit(cnrState* state);


#endif
