#include <stdio.h>
#include <string.h>
#include "load.h"


typedef struct cnrRcgParser {
  // TODO
}* cnrRcgParser;


/**
 * Parses the contents of a parenthesized expression (or the top level of the
 * file). The open paren should already be consumed.
 */
cnBool cnrParseContents(char** line);


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
char* cnrParseQuoted(char** line);


/**
 * Parses a single rcg line.
 */
cnBool cnrParseRcgLine(char* line);


/**
 * Parses all lines in the file. The rcg format is a line-oriented format.
 */
cnBool cnrParseRcgLines(FILE* file);


/**
 * Parse a show command. The leading paren and the show token have both already
 * been consumed from the line.
 */
cnBool cnrParseShow(char* line);


cnBool cnrLoadGameLog(char* name) {
  FILE* file = NULL;
  cnString line;
  cnBool result = cnFalse;

  // Init stuff and open file.
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
  if (!cnrParseRcgLines(file)) cnFailTo(DONE, "Failed parsing.");

  // Winned!
  result = cnTrue;

  DONE:
  if (file) fclose(file);
  cnStringDispose(&line);
  return result;
}


cnBool cnrParseContents(char** line) {
  cnBool result = cnFalse;

  while (*(*line = cnNextChar(*line)) && **line != ')') {
    switch (**line) {
    case '(':
      (*line)++;
      if (!cnrParseContents(line)) cnFailTo(DONE, "Failed parsing contents.");
      break;
    case '"':
      (*line)++;
      if (!cnrParseQuoted(line)) cnFailTo(DONE, "Failed parsing string.");
      break;
    default:
      // TODO Parse ids and numbers. Anything else?
      (*line)++;
      break;
    }
  }
  if (**line != ')') cnFailTo(DONE, "Premature end of line.");

  // Winned.
  result = cnTrue;

  DONE:
  return result;
}


char* cnrParseQuoted(char** line) {
  // Assume we got it.
  char* result = *line;

  for (; **line != '"'; (*line)++) {
    if (!**line) {
      // Nope. Failed.
      result = NULL;
      cnFailTo(DONE, "Unterminated string.");
    }
  }

  // Found the end.
  **line = '\0';

  DONE:
  return result;
}


cnBool cnrParseRcgLine(char* line) {
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
    if (!cnrParseShow(line)) cnFailTo(DONE, "Failed to parse show.");
  }

  // Winned.
  SUCCESS:
  result = cnTrue;

  DONE:
  return result;
}


cnBool cnrParseRcgLines(FILE* file) {
  cnString line;
  cnCount lineCount = 0;
  cnCount readCount;
  cnBool result = cnFalse;

  cnStringInit(&line);

  while ((readCount = cnReadLine(file, &line)) > 0) {
    lineCount++;
    if (!cnrParseRcgLine(cnStr(&line))) {
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


cnBool cnrParseShow(char* line) {
  cnIndex stepIndex;
  cnBool result = cnFalse;

  // Find where we are.
  stepIndex = strtol(line, &line, 10);
  if (!cnrParseContents(&line)) cnFailTo(DONE, "Failed parsing line content.");

  // Winned.
  result = cnTrue;

  DONE:
  return result;
}
