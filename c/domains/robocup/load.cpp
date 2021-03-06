#include <stdio.h>
#include <string.h>
#include "load.h"

using namespace concuno;
using namespace std;


namespace ccndomain {
namespace rcss {


enum cnrParseMode {

  /**
   * Commands issued by a client.
   */
  cnrParseModeCommand,

  /**
   * Identified kick command.
   */
  cnrParseModeCommandKick,

  /**
   * Any child of a command.
   */
  cnrParseModeCommandKid,

  /**
   * A text log command not related to a player.
   */
  cnrParseModeCommandNonPlayer,

  /**
   * A show line is being parsed.
   */
  cnrParseModeShow,

  /**
   * A particular item within a show is being parsed.
   */
  cnrParseModeShowItem,

  /**
   * The id of a show item is being parsed.
   */
  cnrParseModeShowItemId,

  /**
   * Some other kid of a show item.
   */
  cnrParseModeShowItemKid,

  /**
   * A team line is being parsed.
   */
  cnrParseModeTeam,

  /**
   * Top-level line parsing is underway.
   */
  cnrParseModeTop,

  /**
   * Top level of a command.
   */
  cnrParseModeTopCommand,

  /**
   * Top level of a non-player command.
   */
  cnrParseModeTopCommandNonPlayer,

  /**
   * Top level of a show.
   */
  cnrParseModeTopShow,

  /**
   * Top level of a team.
   */
  cnrParseModeTopTeam,

};


struct Parser {

  Parser();

  /**
   * The item is bogus and should be deleted after parsing.
   */
  bool deletePlayer;

  /**
   * The game being loaded into.
   */
  Game* game;

  /**
   * The index in the current parentheses.
   */
  Index index;

  /**
   * The item (ball or player) currently being parsed, if any.
   */
  Item* item;

  /**
   * The current mode of the parser.
   */
  cnrParseMode mode;

  /**
   * The state currently being put together.
   */
  State* state;

};


/**
 * Parses the contents of a parenthesized expression (or the top level of the
 * file). The open paren should already be consumed.
 */
bool cnrParseContents(Parser* parser, char** line);


/**
 * An identifier following some kind of rules.
 */
bool cnrParseId(Parser* parser, char** line);


bool cnrParseNumber(Parser* parser, char** line);


/**
 * Parses a double-quote terminated string. The open double-quote should already
 * be consumed.
 *
 * Assumes no backslash escaping of anything and can therefore return pointers
 * into the original line. The closing double-quote will be replaced with a null
 * char.
 *
 * If there is no terminating null char, a null pointer is returned.
 *
 * TODO Are escapes really not supported?
 */
char* cnrParseQuoted(Parser* parser, char** line);


/**
 * Parses a single rcg line.
 */
bool cnrParseRcgLine(Parser* parser, char* line);


/**
 * Parses all lines in the file. The rcg format is a line-oriented format.
 */
bool cnrParseRcgLines(Parser* parser, FILE* file);


/**
 * Trigger the beginning of contents.
 */
bool cnrParserTriggerContentsBegin(Parser* parser);


/**
 * Trigger the beginning of contents.
 */
bool cnrParserTriggerContentsEnd(Parser* parser);


/**
 * Trigger an id. Treat id as temporary only for this function call.
 */
bool cnrParserTriggerId(Parser* parser, char* id);


/**
 * Trigger an number.
 */
bool cnrParserTriggerNumber(Parser* parser, Float number);


/**
 * Parse a show command. The leading paren and the show token have both already
 * been consumed from the line.
 */
bool cnrParseShow(Parser* parser, char* line);


/**
 * Parse a team command. The leading paren and the show token have both already
 * been consumed from the line.
 */
bool cnrParseTeam(Parser* parser, char* line);


void cnrRcgParserItemLocation(
  Parser* parser, Index xIndex, Float value
);


bool cnrRclParseLine(Parser* parser, char* line);


bool cnrLoadCommandLog(Game* game, char* name) {
  FILE* file = NULL;
  String line;
  Count lineCount = 0;
  Parser parser;
  Count readCount;
  bool result = false;

  // Inits.
  parser.game = game;
  parser.state = reinterpret_cast<State*>(parser.game->states.items);

  // Load file, and parse lines.
  if (!(file = fopen(name, "r"))) cnErrTo(DONE, "Couldn't open file!");
  // TODO Load the file, eh?
  while ((readCount = cnReadLine(file, &line)) > 0) {
    lineCount++;
    if (!cnrRclParseLine(&parser, cnStr(&line))) {
      cnErrTo(DONE, "Failed parsing line %ld.", lineCount);
    }
  }
  if (readCount < 0) cnErrTo(DONE, "Failed reading line.");

  // Winned.
  result = true;

  DONE:
  if (file) fclose(file);
  return result;
}


bool cnrLoadGameLog(Game* game, char* name) {
  FILE* file = NULL;
  String line;
  Parser parser;
  bool result = false;

  // Init stuff and open file.
  parser.game = game;
  if (!(file = fopen(name, "r"))) cnErrTo(DONE, "Couldn't open file!");

  // Consume version indicator.
  if (cnReadLine(file, &line) < 0) cnErrTo(DONE, "Failed first line.");
  // Trim any newline.
  // TODO Some "cnEndsWith" or something! Or just cnStringTrim!
  if (line.count > 4 && cnStr(&line)[4] == '\n') {
    cnStr(&line)[4] = '\0';
    line.count--;
  }
  // Check it.
  // TODO Binary forms (versions 2 and 3) also start with ULG, but they might
  // TODO not have trailing newline or white space, so we should just do fgetc
  // TODO reads instead.
  if (strcmp(cnStr(&line), "ULG5")) {
    cnErrTo(DONE, "Unsupported file type or version: %s", cnStr(&line));
  }

  // Parse everything.
  if (!cnrParseRcgLines(&parser, file)) cnErrTo(DONE, "Failed parsing.");

  // Winned!
  result = true;

  DONE:
  if (file) fclose(file);
  return result;
}


bool cnrParseContents(Parser* parser, char** line) {
  Index index = -1;
  bool result = false;

  if (!cnrParserTriggerContentsBegin(parser)) {
    cnErrTo(DONE, "Failed begin trigger.");
  }
  while (*(*line = cnNextChar(*line)) && **line != ')') {
    char c = **line;
    // Set the index with each loop iteration. The C stack will help us be
    // where we need to be otherwise.
    parser->index = ++index;

    // Now see what to do next.
    switch (c) {
    case '(':
      // Nested parens.
      (*line)++;
      if (!cnrParseContents(parser, line)) {
        cnErrTo(DONE, "Failed parsing contents.");
      }
      break;
    case '"':
      // Double-quoted string.
      (*line)++;
      if (!cnrParseQuoted(parser, line)) {
        cnErrTo(DONE, "Failed parsing string.");
      }
      break;
    default:
      // Something else.
      if (('0' <= c && c <= '9') || c == '.' || c == '-') {
        // Number.
        if (!cnrParseNumber(parser, line)) cnErrTo(DONE, "Failed number.");
      } else {
        // Treat anything else as an identifier.
        if (!cnrParseId(parser, line)) cnErrTo(DONE, "Failed number.");
      }
      break;
    }
  }
  if (**line != ')' && parser->mode != cnrParseModeCommand) {
    // Contents should end in ')' except for the top level of commands.
    cnErrTo(DONE, "Premature end of line.");
  }

  // Good to go. Move on.
  (*line)++;
  if (!cnrParserTriggerContentsEnd(parser)) {
    cnErrTo(DONE, "Failed end trigger.");
  }

  // Winned.
  result = true;

  DONE:
  return result;
}


bool cnrParseId(Parser* parser, char** line) {
  char c;
  char* id = *line;
  bool result = false;
  bool triggerResult;

  // Parse and handle.
  for (; **line && !(isspace(**line) || **line == ')'); (*line)++) {}

  // Remember the old char for reverting, then chop to a string.
  c = **line;
  **line = '\0';
  // Trigger and revert.
  triggerResult = cnrParserTriggerId(parser, id);
  **line = c;
  // Only check failure after revert.
  if (!triggerResult) cnErrTo(DONE, "Failed id trigger.");

  // Winned.
  result = true;

  DONE:
  return result;
}


bool cnrParseNumber(Parser* parser, char** line) {
  char* end;
  Float number;
  bool result = false;

  // Parse the number.
  number = strtod(*line, &end);
  if (!end) cnErrTo(DONE, "No number.");
  *line = end;

  // Handle it.
  if (!cnrParserTriggerNumber(parser, number)) {
    cnErrTo(DONE, "Failed number trigger.");
  }

  // Winned.
  result = true;

  DONE:
  return result;
}


char* cnrParseQuoted(Parser* parser, char** line) {
  // Assume we got it.
  char* result = *line;

  for (; **line != '"'; (*line)++) {
    if (!**line) {
      // Nope. Failed.
      result = NULL;
      cnErrTo(DONE, "Unterminated string.");
    }
  }

  // Found the end. Mark it and go past.
  **line = '\0';
  (**line)++;

  DONE:
  return result;
}


bool cnrParseRcgLine(Parser* parser, char* line) {
  bool result = false;
  char* type;

  // Make sure we have a paren.
  line = cnNextChar(line);
  if (!*line) {
    // Empty line. That's okay.
    goto SUCCESS;
  }
  if (*line != '(') cnErrTo(DONE, "Expected '('.");
  line++;

  // Get our line type.
  type = cnParseStr(line, &line);
  if (!*type) cnErrTo(DONE, "No line type.");

  // Dispatch by type. TODO Hash table? Anything other than show?
  if (!strcmp(type, "show")) {
    if (!cnrParseShow(parser, line)) cnErrTo(DONE, "Failed to parse show.");
  } else if (!strcmp(type, "team")) {
    if (!cnrParseTeam(parser, line)) cnErrTo(DONE, "Failed to parse show.");
  }
  // TODO (team 1 WrightEagle HELIOS2011 0 0)

  // Winned.
  SUCCESS:
  result = true;

  DONE:
  return result;
}


bool cnrParseRcgLines(Parser* parser, FILE* file) {
  String line;
  Count lineCount = 0;
  Count readCount;
  bool result = false;

  while ((readCount = cnReadLine(file, &line)) > 0) {
    lineCount++;
    if (!cnrParseRcgLine(parser, cnStr(&line))) {
      cnErrTo(DONE, "Failed parsing line %ld.", lineCount);
    }
  }
  if (readCount < 0) cnErrTo(DONE, "Failed reading line.");

  // Winned.
  result = true;

  DONE:
  return result;
}


Parser::Parser():
  deletePlayer(false), game(NULL), index(0), item(NULL), mode(cnrParseModeTop),
  state(NULL) {}


bool cnrParserTriggerContentsBegin(Parser* parser) {
  bool result = false;

  // TODO State management?

  // Mode state "stack".
  switch (parser->mode) {
  case cnrParseModeCommand:
    parser->mode = cnrParseModeCommandKid;
    break;
  case cnrParseModeShow:
    parser->mode = cnrParseModeShowItem;
    break;
  case cnrParseModeShowItem:
    if (parser->index) {
      parser->mode = cnrParseModeShowItemKid;
    } else {
      parser->mode = cnrParseModeShowItemId;
    }
    break;
  case cnrParseModeTopCommand:
    parser->mode = cnrParseModeCommand;
    break;
  case cnrParseModeTopCommandNonPlayer:
    parser->mode = cnrParseModeCommandNonPlayer;
    break;
  case cnrParseModeTopShow:
    parser->mode = cnrParseModeShow;
    break;
  case cnrParseModeTopTeam:
    parser->mode = cnrParseModeTeam;
    break;
  default:
    cnErrTo(DONE, "Unknown parser mode: %d", parser->mode);
  }

  // Winned.
  result = true;

  DONE:
  return result;
}


bool cnrParserTriggerContentsEnd(Parser* parser) {
  bool result = false;

  // TODO Anything else?

  // Mode state "stack".
  switch (parser->mode) {
  case cnrParseModeCommand:
  case cnrParseModeCommandNonPlayer:
  case cnrParseModeShow:
  case cnrParseModeTeam:
    parser->mode = cnrParseModeTop;
    break;
  case cnrParseModeCommandKick:
  case cnrParseModeCommandKid:
    parser->mode = cnrParseModeCommand;
    break;
  case cnrParseModeShowItem:
    //printf("(%lg %lg) ", parser->item->location[0], parser->item->location[1]);
    if (parser->deletePlayer) {
      // Pop off the most recent player. Players don't need disposed.
      parser->state->players.count--;
      parser->deletePlayer = false;
    }
    parser->mode = cnrParseModeShow;
    parser->item = NULL;
    break;
  case cnrParseModeShowItemId:
  case cnrParseModeShowItemKid:
    parser->mode = cnrParseModeShowItem;
    break;
  default:
    cnErrTo(DONE, "Unknown parser mode: %d", parser->mode);
  }

  // Winned.
  result = true;

  DONE:
  return result;
}


bool cnrParserTriggerId(Parser* parser, char* id) {
  bool result = false;

  // Mode state machine.
  switch (parser->mode) {
  case cnrParseModeCommandKid:
    if (!parser->index && !strcmp(id, "kick")) {
      parser->mode = cnrParseModeCommandKick;
    }
    break;
  case cnrParseModeCommandNonPlayer:
    if (parser->index == 2 && !strcmp(id, "Keepaway")) {
      // It's a new keepaway session.
      parser->state->newSession = true;
    }
    break;
  case cnrParseModeShowItemId:
    if (!parser->index) {
      // TODO Choose or create focus item.
      // TODO For players, however, we don't know until the number.
      //printf("%s ", id);
      if (!strcmp(id, "b")) {
        // Ball.
        parser->item = &parser->state->ball;
      } else if (!(strcmp(id, "l") && strcmp(id, "r"))) {
        // Player. We don't preallocate these, so make space now.
        Player* player =
          reinterpret_cast<Player*>(cnListExpand(&parser->state->players));
        if (!player) cnErrTo(DONE, "No player.");
        new(player) Player();
        // Be explicit here for clarity.
        player->team = *id == 'l' ? cnrTeamLeft : cnrTeamRight;
        // And track the item.
        parser->item = player;
      }
    }
    break;
  case cnrParseModeTeam:
    if (parser->index == 1 || parser->index == 2) {
      Team team = parser->index - 1;
      if (parser->game->teamNames.size() == team) {
        // Team index as expected.
        //string name(id);
        parser->game->teamNames.push_back(id);
      } else {
        // TODO Validate unchanged team names?
      }
    }
    break;
  default:
    // Ignore.
    break;
  }

  // Winned.
  result = true;

  DONE:
  return result;
}


bool cnrParserTriggerNumber(Parser* parser, Float number) {
  bool result = false;
  Player* player;

  // Mode state machine.
  switch (parser->mode) {
  case cnrParseModeCommandKick:
    player = reinterpret_cast<Player*>(parser->item);
    switch (parser->index) {
    case 1:
      player->kickPower = number;
      break;
    case 2:
      player->kickAngle = number;
      break;
    default:
      cnErrTo(DONE, "Kick number at index %ld?", parser->index);
    }
    break;
  case cnrParseModeShow:
    if (!parser->index) {
      parser->state->time = (Time)number;
      if (
        parser->state > reinterpret_cast<State*>(parser->game->states.items)
      ) {
        // Past the first.
        State* previous = parser->state - 1;
        if (previous->time == parser->state->time) {
          // Must be a subtime increment. First submit is 0. Presumed to go up
          // by 1 each for each show. Subtime is only implicit in rcg files.
          parser->state->subtime = previous->subtime + 1;
        }
      }
    }
    break;
  case cnrParseModeShowItem:
    if (parser->item) {
      switch (parser->item->type) {
      case Item::TypeBall:
        // Ball location at indices 1, 2.
        cnrRcgParserItemLocation(parser, 1, number);
        break;
      case Item::TypePlayer:
        // Player location at indices 3, 4.
        cnrRcgParserItemLocation(parser, 3, number);
        switch (parser->index) {
        case 2:
          // Active status? It seems to be, but I haven't checked specs.
          if (!number) {
            // This player is actually bogus. Mark for deletion.
            parser->deletePlayer = true;
          }
          break;
        case 7:
          // Body angle. TODO Units?
          ((Player*)parser->item)->orientation = number;
          break;
        default:
          // Skip it.
          break;
        }
        break;
      default:
        cnErrTo(DONE, "Unknown item type %d.", parser->item->type);
      }
    }
    break;
  case cnrParseModeShowItemId:
    if (parser->index == 1) {
      player = (Player*)parser->item;
      if (player->type != Item::TypePlayer) {
        cnErrTo(DONE, "Index %ld of non-player.", (Index)number);
      }
      // TODO Verify nonduplicate player?
      player->index = number;
      //printf("P%ld%02ld ", player->team, player->index);
    }
    break;
  default:
    // Ignore.
    break;
  }

  // Winned.
  result = true;

  DONE:
  return result;
}


bool cnrParseShow(Parser* parser, char* line) {
  bool result = false;

  // Prepare a new state to work with.
  if (!(
    parser->state =
      reinterpret_cast<State*>(cnListExpand(&parser->game->states))
  )) {
    cnErrTo(DONE, "No new state.");
  }
  new(parser->state) State();

  // Parse through the rest.
  parser->mode = cnrParseModeTopShow;
  if (!cnrParseContents(parser, &line)) {
    cnErrTo(DONE, "Failed parsing line content.");
  }
  //printf("\n");

  // Winned.
  result = true;

  DONE:
  parser->mode = cnrParseModeTop;
  return result;
}


bool cnrParseTeam(Parser* parser, char* line) {
  bool result = false;

  // Parse through the rest.
  parser->mode = cnrParseModeTopTeam;
  if (!cnrParseContents(parser, &line)) {
    cnErrTo(DONE, "Failed parsing line content.");
  }
  //printf("\n");

  // Winned.
  result = true;

  DONE:
  parser->mode = cnrParseModeTop;
  return result;
}


void cnrRcgParserItemLocation(
  Parser* parser, Index xIndex, Float value
) {
  if (xIndex <= parser->index && parser->index <= xIndex + 1) {
    parser->item->location[parser->index - xIndex] = value;
  }
}


bool cnrRclParseLine(Parser* parser, char* line) {
  Index playerIndex;
  bool result = false;
  State* statesEnd;
  Index subtime;
  Team team;
  Index time;
  char* token;

  // Format: time,subtime\t(Recv|\(...\)) TeamName_N: (command (content))
  if (!cnDelimitInt(&line, NULL, &time, ',')) cnErrTo(DONE, "No time.");
  if (!cnDelimitInt(&line, NULL, &subtime, '\t')) cnErrTo(DONE, "No subtime.");

  // Find out which state we're in. Major time.
  // TODO Unify time/subtime in some fashion? Array instead of separate vars?
  statesEnd = reinterpret_cast<State*>(cnListEnd(&parser->game->states));
  while (parser->state < statesEnd && parser->state->time < time) {
    // Go to the next state, while we have any.
    parser->state++;
  }
  // If we haven't gotten to our next state, skip out.
  if (parser->state >= statesEnd || parser->state->time != time) goto WIN;

  // Subtime handling.
  while (
    parser->state < statesEnd &&
    // Make sure we stay on the same time, in case things don't align.
    // TODO Special handling for time 0?
    parser->state->time == time &&
    parser->state->subtime < subtime
  ) {
    // Go to the next state, while we have any.
    parser->state++;
  }
  // If we haven't gotten to our next state, skip out.
  if (
    parser->state >= statesEnd ||
    parser->state->time != time ||
    parser->state->subtime != subtime
  ) goto WIN;

  // Check for non-player lines.
  if (*line == '(') {
    // Skip the paren for content parsing.
    line++;
    parser->mode = cnrParseModeTopCommandNonPlayer;
    if (!cnrParseContents(parser, &line)) {
      cnErrTo(DONE, "Failed parsing non-player line.");
    }
    goto WIN;
  }

  // Check for some other line type. We only care about Recv.
  if (!(token = cnParseStr(line, &line))) cnErrTo(DONE, "No line type.");
  if (strcmp(token, "Recv")) goto WIN;

  // Team name and index.
  if (!(token = cnDelimit(&line, '_'))) cnErrTo(DONE, "No team_player split.");
  team = 0;
  for (size_t n = 0; n < parser->game->teamNames.size(); n++) {
    // Break if we have it, and increment if not.
    if (parser->game->teamNames[n] == token) break;
    team++;
  }
  if (team >= parser->game->teamNames.size()) {
    cnErrTo(DONE, "Unrecognized team.");
  }

  // Player index.
  if (!cnDelimitInt(&line, &token, &playerIndex, ':')) {
    // Ignore coach, and fail otherwise.
    if (!strcmp(token, "Coach")) goto WIN;
    cnErrTo(DONE, "No player.");
  }

  // Find the player in the state.
  parser->item = NULL;
  cnListEachBegin(&parser->state->players, Player, player) {
    if (team == player->team && playerIndex == player->index) {
      parser->item = player;
    }
  } cnEnd;
  if (!parser->item) cnErrTo(DONE, "No player %zu/%ld.", team, playerIndex);

  // Parse deeper for kicks (literally).
  parser->mode = cnrParseModeTopCommand;
  if (!cnrParseContents(parser, &line)) {
    cnErrTo(DONE, "Failed command contents.");
  }

  WIN:
  result = true;

  DONE:
  return result;
}


}
}
