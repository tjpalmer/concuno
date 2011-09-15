#include <stdio.h>
#include <string.h>
#include "load.h"


typedef struct cnrRcgParser {
  // TODO
}* cnrRcgParser;


/**
 * Skips whitespace, returning nonspace or null char (for eof or error).
 *
 * Check for error with ferror after returning.
 */
char cnrAdvance(FILE* file);


/**
 * Parses the contents of a parenthesized expression (or the top level of the
 * file). The open paren should already be consumed.
 */
cnBool cnrParseContents(FILE* file);


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


/**
 * Parses a double-quote terminated string. The open double-quote should already
 * be consumed.
 *
 * TODO Is backslash escaping used?
 */
cnBool cnrParseString(FILE* file);


char cnrAdvance(FILE* file) {
  int i;
  while ((i = fgetc(file)) != EOF) {
    if (!isspace(i)) {
      // The nonspace we're looking for.
      return (char)i;
    }
    if (i == '\n') fputc(i, stdout);
  }
  return '\0';
}


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


cnBool cnrParseContents(FILE* file) {
  char c;
  cnBool result = cnFalse;

  while ((c = cnrAdvance(file)) && c != ')') {
    switch (c) {
    case '(':
      fputc(c, stdout);
      if (!cnrParseContents(file)) cnFailTo(DONE, "Failed parsing contents.");
      break;
    case '"':
      if (!cnrParseString(file)) cnFailTo(DONE, "Failed parsing string.");
      break;
    default:
      // TODO Parse ids and numbers. Anything else?
      break;
    }
  }
  if (ferror(file)) cnFailTo(DONE, "Failed advancing.");
  if (c == ')') fputc(c, stdout);
  // TODO Also track when ')' appropriate (e.g., not at file top)?

  // Winned.
  result = cnTrue;

  DONE:
  return result;
}


cnBool cnrParseRcgLine(char* line) {
  char c;
  int count;
  cnBool result = cnFalse;
  char* type;

  // Make sure we have a paren.
  if (sscanf(line, "%1s%n", &c, &count) <= 0) {
    // Nothing on this line.
    goto SUCCESS;
  }
  if (c != '(') cnFailTo(DONE, "Expected '('.");
  line += count;

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

  // Winned.
  result = cnTrue;

  return result;
}


cnBool cnrParseString(FILE* file) {
  int c;
  cnBool result = cnFalse;

  if ((c = fgetc(file)) == EOF) cnFailTo(DONE, "Untermined string.");

  if (c == '(') {
    // Parenthesized string.
    printf("\"(");
    if (!cnrParseContents(file)) cnFailTo(DONE, "Failed paren string.");
    if ((c = fgetc(file)) == EOF) cnFailTo(DONE, "Untermined string.");
    if (c != '"') cnFailTo(DONE, "No ending double-quote on paren string.");
    printf("\"");
  }

  while ((c = cnrAdvance(file)) && c != '"') {
    // TODO Also break string on newline?
    if (c == '\\') {
      // Escape sequence.
      // TODO Is this really how escapes work for rcss log files?
      int i = fgetc(file);
      if (i == EOF) break;
      // TODO Do something with the escaped char.
    }
  }

  if (ferror(file)) cnFailTo(DONE, "Failed advancing.");
  if (feof(file)) cnFailTo(DONE, "Unterminated string.");

  // Winned.
  result = cnTrue;

  DONE:
  return result;
}
