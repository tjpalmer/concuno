#include <stdio.h>
#include <string.h>
#include "load.h"


typedef enum {

  /**
   * A show line is being parsed.
   */
  cnrRcgModeShow,

  /**
   * A particular item within a show is being parsed.
   */
  cnrRcgModeShowItem,

  /**
   * The id of a show item is being parsed.
   */
  cnrRcgModeShowItemId,

  /**
   * Some other kid of a show item.
   */
  cnrRcgModeShowItemKid,

  /**
   * Top-level line parsing is underway.
   */
  cnrRcgModeTop,

} cnrRcgMode;


typedef struct cnrRcgParser {

  // TODO

  /**
   * The index in the current parentheses.
   */
  cnIndex index;

  /**
   * The current mode of the parser.
   */
  cnrRcgMode mode;

  /**
   * The state currently being put together.
   */
  cnrState state;

  cnList(struct cnrState) states;

}* cnrRcgParser;


/**
 * Parses the contents of a parenthesized expression (or the top level of the
 * file). The open paren should already be consumed.
 */
cnBool cnrParseContents(cnrRcgParser parser, char** line);


/**
 * An identifier following some kind of rules.
 */
cnBool cnrParseId(cnrRcgParser parser, char** line);


cnBool cnrParseNumber(cnrRcgParser parser, char** line);


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
char* cnrParseQuoted(cnrRcgParser parser, char** line);


/**
 * Parses a single rcg line.
 */
cnBool cnrParseRcgLine(cnrRcgParser parser, char* line);


/**
 * Parses all lines in the file. The rcg format is a line-oriented format.
 */
cnBool cnrParseRcgLines(cnrRcgParser parser, FILE* file);


/**
 * Trigger the beginning of contents.
 */
cnBool cnrParserTriggerContentsBegin(cnrRcgParser parser);


/**
 * Trigger the beginning of contents.
 */
cnBool cnrParserTriggerContentsEnd(cnrRcgParser parser);


/**
 * Trigger an id. Treat id as temporary only for this function call.
 */
cnBool cnrParserTriggerId(cnrRcgParser parser, char* id);


/**
 * Trigger an number.
 */
cnBool cnrParserTriggerNumber(cnrRcgParser parser, cnFloat number);


/**
 * Parse a show command. The leading paren and the show token have both already
 * been consumed from the line.
 */
cnBool cnrParseShow(cnrRcgParser parser, char* line);


void cnrRcgParserDispose(cnrRcgParser parser);


void cnrRcgParserInit(cnrRcgParser parser);


cnBool cnrLoadGameLog(char* name) {
  FILE* file = NULL;
  cnString line;
  struct cnrRcgParser parser;
  cnBool result = cnFalse;

  // Init stuff and open file.
  cnrRcgParserInit(&parser);
  cnStringInit(&line);
  if (!(file = fopen(name, "r"))) cnFailTo(DONE, "Couldn't open file!");

  // Consume version indicator.
  if (cnReadLine(file, &line) < 0) cnFailTo(DONE, "Failed first line.");
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
    cnFailTo(DONE, "Unsupported file type or version: %s", cnStr(&line));
  }

  // Parse everything.
  if (!cnrParseRcgLines(&parser, file)) cnFailTo(DONE, "Failed parsing.");

  // Winned!
  result = cnTrue;

  DONE:
  if (file) fclose(file);
  cnStringDispose(&line);
  cnrRcgParserDispose(&parser);
  return result;
}


cnBool cnrParseContents(cnrRcgParser parser, char** line) {
  cnIndex index = 0;
  cnBool result = cnFalse;

  if (!cnrParserTriggerContentsBegin(parser)) {
    cnFailTo(DONE, "Failed begin trigger.");
  }
  while (*(*line = cnNextChar(*line)) && **line != ')') {
    char c = **line;
    // Set the index with each loop iteration. The C stack will help us be
    // where we need to be otherwise.
    parser->index = index;

    // Now see what to do next.
    switch (c) {
    case '(':
      // Nested parens.
      (*line)++;
      if (!cnrParseContents(parser, line)) {
        cnFailTo(DONE, "Failed parsing contents.");
      }
      break;
    case '"':
      // Double-quoted string.
      (*line)++;
      if (!cnrParseQuoted(parser, line)) {
        cnFailTo(DONE, "Failed parsing string.");
      }
      break;
    default:
      // Something else.
      if (('0' <= c && c <= '9') || c == '.') {
        // Number.
        if (!cnrParseNumber(parser, line)) cnFailTo(DONE, "Failed number.");
      } else {
        // Treat anything else as an identifier.
        if (!cnrParseId(parser, line)) cnFailTo(DONE, "Failed number.");
      }
      break;
    }

    // Increment.
    index++;
  }
  if (**line != ')') cnFailTo(DONE, "Premature end of line.");

  // Good to go. Move on.
  (**line)++;
  if (!cnrParserTriggerContentsEnd(parser)) {
    cnFailTo(DONE, "Failed end trigger.");
  }

  // Winned.
  result = cnTrue;

  DONE:
  return result;
}


cnBool cnrParseId(cnrRcgParser parser, char** line) {
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
  if (!triggerResult) cnFailTo(DONE, "Failed id trigger.");

  // Winned.
  result = cnTrue;

  DONE:
  return result;
}


cnBool cnrParseNumber(cnrRcgParser parser, char** line) {
  char* end;
  cnFloat number;
  cnBool result = cnFalse;

  // Parser the number.
  number = strtod(*line, &end);
  if (!end) cnFailTo(DONE, "No number.");
  *line = end;

  // Handle it.
  if (!cnrParserTriggerNumber(parser, number)) {
    cnFailTo(DONE, "Failed number trigger.");
  }

  // Winned.
  result = cnTrue;

  DONE:
  return cnTrue;
}


char* cnrParseQuoted(cnrRcgParser parser, char** line) {
  // Assume we got it.
  char* result = *line;

  for (; **line != '"'; (*line)++) {
    if (!**line) {
      // Nope. Failed.
      result = NULL;
      cnFailTo(DONE, "Unterminated string.");
    }
  }

  // Found the end. Mark it and go past.
  **line = '\0';
  (**line)++;

  DONE:
  return result;
}


cnBool cnrParseRcgLine(cnrRcgParser parser, char* line) {
  cnBool result = cnFalse;
  char* type;

  // Make sure we have a paren.
  line = cnNextChar(line);
  if (!*line) {
    // Empty line. That's okay.
    goto SUCCESS;
  }
  if (*line != '(') cnFailTo(DONE, "Expected '('.");
  line++;

  // Get our line type.
  type = cnParseStr(line, &line);
  if (!*type) cnFailTo(DONE, "No line type.");

  // Dispatch by type. TODO Hash table? Anything other than show?
  if (!strcmp(type, "show")) {
    if (!cnrParseShow(parser, line)) cnFailTo(DONE, "Failed to parse show.");
  }

  // Winned.
  SUCCESS:
  result = cnTrue;

  DONE:
  return result;
}


cnBool cnrParseRcgLines(cnrRcgParser parser, FILE* file) {
  cnString line;
  cnCount lineCount = 0;
  cnCount readCount;
  cnBool result = cnFalse;

  cnStringInit(&line);

  while ((readCount = cnReadLine(file, &line)) > 0) {
    lineCount++;
    if (!cnrParseRcgLine(parser, cnStr(&line))) {
      cnFailTo(DONE, "Failed parsing line %ld.", lineCount);
    }
  }
  if (readCount < 0) cnFailTo(DONE, "Failed reading line.");

  // Winned.
  result = cnTrue;

  DONE:
  cnStringDispose(&line);
  return result;
}


cnBool cnrParserTriggerContentsBegin(cnrRcgParser parser) {
  cnBool result = cnFalse;

  // TODO State management?

  // Mode state "stack".
  switch (parser->mode) {
  case cnrRcgModeShow:
    parser->mode = cnrRcgModeShowItem;
    break;
  case cnrRcgModeShowItem:
    if (parser->index) {
      parser->mode = cnrRcgModeShowItemKid;
    } else {
      parser->mode = cnrRcgModeShowItemId;
    }
    break;
  case cnrRcgModeTop:
    parser->mode = cnrRcgModeShow;
    break;
  default:
    cnFailTo(DONE, "Unknown parser mode: %d", parser->mode);
  }

  // Winned.
  result = cnTrue;

  DONE:
  return result;
}


cnBool cnrParserTriggerContentsEnd(cnrRcgParser parser) {
  cnBool result = cnFalse;

  // TODO Anything else?

  // Mode state "stack".
  switch (parser->mode) {
  case cnrRcgModeShow:
    parser->mode = cnrRcgModeTop;
    break;
  case cnrRcgModeShowItem:
    parser->mode = cnrRcgModeShow;
    break;
  case cnrRcgModeShowItemId:
  case cnrRcgModeShowItemKid:
    parser->mode = cnrRcgModeShowItem;
    break;
  default:
    cnFailTo(DONE, "Unknown parser mode: %d", parser->mode);
  }

  // Winned.
  result = cnTrue;

  DONE:
  return result;
}


cnBool cnrParserTriggerId(cnrRcgParser parser, char* id) {
  cnBool result = cnFalse;

  // Mode state machine.
  switch (parser->mode) {
  case cnrRcgModeShowItemId:
    if (!parser->index) {
      // TODO Choose or create focus item.
      // TODO For players, however, we don't know until the number.
      //printf("%s ", id);
    }
    break;
  default:
    // Ignore.
    break;
  }

  // Winned.
  result = cnTrue;

  // DONE:
  return result;
}


cnBool cnrParserTriggerNumber(cnrRcgParser parser, cnFloat number) {
  cnBool result = cnFalse;

  // Mode state machine.
  switch (parser->mode) {
  case cnrRcgModeShowItemId:
    if (parser->index == 1) {
      // TODO Choose or create focus item.
      // TODO For players, however, we don't know until the number.
      //printf("%lg ", number);
    }
    break;
  default:
    // Ignore.
    break;
  }

  // Winned.
  result = cnTrue;

  // DONE:
  return result;
}


cnBool cnrParseShow(cnrRcgParser parser, char* line) {
  cnIndex stepIndex;
  cnBool result = cnFalse;

  // Prepare a new state to work with.
  //cnListExpand()

  // Find where we are.
  stepIndex = strtol(line, &line, 10);
  if (!cnrParseContents(parser, &line)) {
    cnFailTo(DONE, "Failed parsing line content.");
  }
  //printf("\n");

  // Winned.
  result = cnTrue;

  DONE:
  parser->mode = cnrRcgModeTop;
  return result;
}


void cnrRcgParserDispose(cnrRcgParser parser) {
  cnListDispose(&parser->states);
}


void cnrRcgParserInit(cnrRcgParser parser) {
  cnListInit(&parser->states, sizeof(struct cnrState));
  // Later point this to the current state being parsed.
  parser->index = 0;
  parser->mode = cnrRcgModeTop;
  parser->state = NULL;
}
