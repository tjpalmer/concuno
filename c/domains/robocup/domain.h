#ifndef concuno_robocup_domain_h
#define concuno_robocup_domain_h


#include <concuno.h>
#include <string>
#include <vector>


namespace ccndomain {
namespace rcss {


/**
 * Index rather than enum because I want to use it as an index. In the abstract,
 * although perhaps not in RoboCup, there also could technically be more than
 * two teams.
 */
typedef size_t Team;

#define cnrTeamLeft 0
#define cnrTeamRight 1

// Are keepers (in keepaway) the left team?
#define cnrTeamKeepers 0
#define cnrTeamTakers 1


typedef concuno::Index Time;


struct Item {

  enum Type {

    /**
     * The matter in question works for any type of Item.
     */
    TypeAny,

    TypeBall,

    TypePlayer,

  };

  Item(Type type);

  Type type;

  concuno::Float location[2];

  // TODO Velocity.

};


struct Ball: Item {

  Ball();

  // Probably nothing else to add ever?

};


struct Player: Item {

  Player();

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

  Team team;

};


struct State {

  State();

  Ball ball;

  /**
   * This state is the beginning of a new session. In keepaway, that means the
   * previous state signaled loss of possession.
   *
   * TODO For soccer, could this represent resumed play after certain events?
   */
  bool newSession;

  concuno::List<Player> players;

  /**
   * The secondary clock of the game that ticks during setup or after fouls.
   */
  Time subtime;

  /**
   * The primary clock of the game.
   */
  Time time;

};


struct Game {

  ~Game();

  concuno::List<State> states;

  std::vector<std::string> teamNames;

};


void cnrItemInit(Item* item, Item::Type type);


/**
 * Inits the schema from scratch to have everything needed for current rcss
 * needs.
 *
 * Dispose with cnSchemaDispose (since nothing special needed here).
 */
void schemaInit(concuno::Schema& schema);



}
}

#endif
