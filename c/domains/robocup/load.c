#include <stdio.h>
#include <string.h>
#include "load.h"


typedef enum {

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

} cnrParseMode;


typedef struct cnrParser {

  /**
   * The item is bogus and should be deleted after parsing.
   */
  cnBool deletePlayer;

  /**
   * The game being loaded into.
   */
  cnrGame game;

  /**
   * The index in the current parentheses.
   */
  cnIndex index;

  /**
   * The item (ball or player) currently being parsed, if any.
   */
  cnrItem item;

  /**
   * The current mode of the parser.
   */
  cnrParseMode mode;

  /**
   * The state currently being put together.
   */
  cnrState state;

}* cnrParser;


/**
 * Parses the contents of a parenthesized expression (or the top level of the
 * file). The open paren should already be consumed.
 */
cnBool cnrParseContents(cnrParser parser, char** line);


/**
 * An identifier following some kind of rules.
 */
cnBool cnrParseId(cnrParser parser, char** line);


cnBool cnrParseNumber(cnrParser parser, char** line);


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
char* cnrParseQuoted(cnrParser parser, char** line);


/**
 * Parses a single rcg line.
 */
cnBool cnrParseRcgLine(cnrParser parser, char* line);


/**
 * Parses all lines in the file. The rcg format is a line-oriented format.
 */
cnBool cnrParseRcgLines(cnrParser parser, FILE* file);


/**
 * Trigger the beginning of contents.
 */
cnBool cnrParserTriggerContentsBegin(cnrParser parser);


/**
 * Trigger the beginning of contents.
 */
cnBool cnrParserTriggerContentsEnd(cnrParser parser);


/**
 * Trigger an id. Treat id as temporary only for this function call.
 */
cnBool cnrParserTriggerId(cnrParser parser, char* id);


/**
 * Trigger an number.
 */
cnBool cnrParserTriggerNumber(cnrParser parser, cnFloat number);


/**
 * Parse a show command. The leading paren and the show token have both already
 * been consumed from the line.
 */
cnBool cnrParseShow(cnrParser parser, char* line);


/**
 * Parse a team command. The leading paren and the show token have both already
 * been consumed from the line.
 */
cnBool cnrParseTeam(cnrParser parser, char* line);


void cnrRcgParserItemLocation(
  cnrParser parser, cnIndex xIndex, cnFloat value
);


void cnrRcgParserDispose(cnrParser parser);


void cnrRcgParserInit(cnrParser parser);


cnBool cnrRclParseLine(cnrParser parser, char* line);


cnBool cnrLoadCommandLog(cnrGame game, char* name) {
  FILE* file = NULL;
  cnString line;
  cnCount lineCount = 0;
  struct cnrParser parser;
  cnCount readCount;
  cnBool result = cnFalse;

  // Inits.
  cnrRcgParserInit(&parser);
  parser.game = game;
  parser.state = (cnrState)parser.game->states.items;
  cnStringInit(&line);

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
  result = cnTrue;

  DONE:
  if (file) fclose(file);
  cnStringDispose(&line);
  cnrRcgParserDispose(&parser);
  return result;
}


cnBool cnrLoadGameLog(cnrGame game, char* name) {
  FILE* file = NULL;
  cnString line;
  struct cnrParser parser;
  cnBool result = cnFalse;

  // Init stuff and open file.
  cnrRcgParserInit(&parser);
  parser.game = game;
  cnStringInit(&line);
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
  result = cnTrue;

  DONE:
  if (file) fclose(file);
  cnStringDispose(&line);
  cnrRcgParserDispose(&parser);
  return result;
}


cnBool cnrParseContents(cnrParser parser, char** line) {
  cnIndex index = -1;
  cnBool result = cnFalse;

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
  result = cnTrue;

  DONE:
  return result;
}


cnBool cnrParseId(cnrParser parser, char** line) {
  char c;
  char* id = *line;
  cnBool result = cnFalse;
  cnBool triggerResult;

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
  result = cnTrue;

  DONE:
  return result;
}


cnBool cnrParseNumber(cnrParser parser, char** line) {
  char* end;
  cnFloat number;
  cnBool result = cnFalse;

  // Parse the number.
  number = strtod(*line, &end);
  if (!end) cnErrTo(DONE, "No number.");
  *line = end;

  // Handle it.
  if (!cnrParserTriggerNumber(parser, number)) {
    cnErrTo(DONE, "Failed number trigger.");
  }

  // Winned.
  result = cnTrue;

  DONE:
  return result;
}


char* cnrParseQuoted(cnrParser parser, char** line) {
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


cnBool cnrParseRcgLine(cnrParser parser, char* line) {
  cnBool result = cnFalse;
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
  result = cnTrue;

  DONE:
  return result;
}


cnBool cnrParseRcgLines(cnrParser parser, FILE* file) {
  cnString line;
  cnCount lineCount = 0;
  cnCount readCount;
  cnBool result = cnFalse;

  cnStringInit(&line);

  while ((readCount = cnReadLine(file, &line)) > 0) {
    lineCount++;
    if (!cnrParseRcgLine(parser, cnStr(&line))) {
      cnErrTo(DONE, "Failed parsing line %ld.", lineCount);
    }
  }
  if (readCount < 0) cnErrTo(DONE, "Failed reading line.");

  // Winned.
  result = cnTrue;

  DONE:
  cnStringDispose(&line);
  return result;
}


cnBool cnrParserTriggerContentsBegin(cnrParser parser) {
  cnBool result = cnFalse;

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
  result = cnTrue;

  DONE:
  return result;
}


cnBool cnrParserTriggerContentsEnd(cnrParser parser) {
  cnBool result = cnFalse;

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
      parser->deletePlayer = cnFalse;
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
  result = cnTrue;

  DONE:
  return result;
}


cnBool cnrParserTriggerId(cnrParser parser, char* id) {
  cnBool result = cnFalse;

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
      parser->state->newSession = cnTrue;
    }
    break;
  case cnrParseModeShowItemId:
    if (!parser->index) {
      // TODO Choose or create focus item.
      // TODO For players, however, we don't know until the number.
      //printf("%s ", id);
      if (!strcmp(id, "b")) {
        // Ball.
        parser->item = &parser->state->ball.item;
      } else if (!(strcmp(id, "l") && strcmp(id, "r"))) {
        // Player. We don't preallocate these, so make space now.
        cnrPlayer player = cnListExpand(&parser->state->players);
        if (!player) cnErrTo(DONE, "No player.");
        cnrPlayerInit(player);
        // Be explicit here for clarity.
        player->team = *id == 'l' ? cnrTeamLeft : cnrTeamRight;
        // And track the item.
        parser->item = &player->item;
      }
    }
    break;
  case cnrParseModeTeam:
    if (parser->index == 1 || parser->index == 2) {
      cnrTeam team = parser->index - 1;
      if (parser->game->teamNames.count == team) {
        // Team index as expected.
        cnString name;
        cnStringInit(&name);
        if (!cnStringPushStr(&name, id)) cnErrTo(DONE, "No team name %s.", id);
        if (!cnListPush(&parser->game->teamNames, &name)) {
          // Clean then fail.
          cnStringDispose(&name);
          cnErrTo(DONE, "No push name.");
        }
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
  result = cnTrue;

  DONE:
  return result;
}


cnBool cnrParserTriggerNumber(cnrParser parser, cnFloat number) {
  cnBool result = cnFalse;
  cnrPlayer player;

  // Mode state machine.
  switch (parser->mode) {
  case cnrParseModeCommandKick:
    player = (cnrPlayer)parser->item;
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
      parser->state->time = (cnrTime)number;
      if (parser->state > (cnrState)parser->game->states.items) {
        // Past the first.
        cnrState previous = parser->state - 1;
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
      case cnrTypeBall:
        // Ball location at indices 1, 2.
        cnrRcgParserItemLocation(parser, 1, number);
        break;
      case cnrTypePlayer:
        // Player location at indices 3, 4.
        cnrRcgParserItemLocation(parser, 3, number);
        switch (parser->index) {
        case 2:
          // Active status? It seems to be, but I haven't checked specs.
          if (!number) {
            // This player is actually bogus. Mark for deletion.
            parser->deletePlayer = cnTrue;
          }
          break;
        case 7:
          // Body angle. TODO Units?
          ((cnrPlayer)parser->item)->orientation = number;
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
      player = (cnrPlayer)parser->item;
      if (player->item.type != cnrTypePlayer) {
        cnErrTo(DONE, "Index %ld of non-player.", (cnIndex)number);
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
  result = cnTrue;

  DONE:
  return result;
}


cnBool cnrParseShow(cnrParser parser, char* line) {
  cnBool result = cnFalse;

  // Prepare a new state to work with.
  if (!(parser->state = cnListExpand(&parser->game->states))) {
    cnErrTo(DONE, "No new state.");
  }
  cnrStateInit(parser->state);

  // Parse through the rest.
  parser->mode = cnrParseModeTopShow;
  if (!cnrParseContents(parser, &line)) {
    cnErrTo(DONE, "Failed parsing line content.");
  }
  //printf("\n");

  // Winned.
  result = cnTrue;

  DONE:
  parser->mode = cnrParseModeTop;
  return result;
}


cnBool cnrParseTeam(cnrParser parser, char* line) {
  cnBool result = cnFalse;

  // Parse through the rest.
  parser->mode = cnrParseModeTopTeam;
  if (!cnrParseContents(parser, &line)) {
    cnErrTo(DONE, "Failed parsing line content.");
  }
  //printf("\n");

  // Winned.
  result = cnTrue;

  DONE:
  parser->mode = cnrParseModeTop;
  return result;
}


void cnrRcgParserItemLocation(
  cnrParser parser, cnIndex xIndex, cnFloat value
) {
  if (xIndex <= parser->index && parser->index <= xIndex + 1) {
    parser->item->location[parser->index - xIndex] = value;
  }
}


void cnrRcgParserDispose(cnrParser parser) {
  // TODO Anything?
  cnrRcgParserInit(parser);
}


void cnrRcgParserInit(cnrParser parser) {
  parser->deletePlayer = cnFalse;
  parser->game = NULL;
  parser->index = 0;
  parser->item = NULL;
  parser->mode = cnrParseModeTop;
  parser->state = NULL;
}


cnBool cnrRclParseLine(cnrParser parser, char* line) {
  cnIndex playerIndex;
  cnBool result = cnFalse;
  cnrState statesEnd;
  cnIndex subtime;
  cnrTeam team;
  cnIndex time;
  char* token;

  // Format: time,subtime\t(Recv|\(...\)) TeamName_N: (command (content))
  if (!cnDelimitInt(&line, NULL, &time, ',')) cnErrTo(DONE, "No time.");
  if (!cnDelimitInt(&line, NULL, &subtime, '\t')) cnErrTo(DONE, "No subtime.");

  // Find out which state we're in. Major time.
  // TODO Unify time/subtime in some fashion? Array instead of separate vars?
  statesEnd = cnListEnd(&parser->game->states);
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
  cnListEachBegin(&parser->game->teamNames, cnString, name) {
    // Break if we have it, and inc if not.
    if (!strcmp(token, cnStr(name))) break;
    team++;
  } cnEnd;
  if (team >= parser->game->teamNames.count) {
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
  cnListEachBegin(&parser->state->players, struct cnrPlayer, player) {
    if (team == player->team && playerIndex == player->index) {
      parser->item = &player->item;
    }
  } cnEnd;
  if (!parser->item) cnErrTo(DONE, "No player %ld/%ld.", team, playerIndex);

  // Parse deeper for kicks (literally).
  parser->mode = cnrParseModeTopCommand;
  if (!cnrParseContents(parser, &line)) {
    cnErrTo(DONE, "Failed command contents.");
  }

  WIN:
  result = cnTrue;

  DONE:
  return result;
}
