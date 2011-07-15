#include "io.h"


char* cnParseStr(char* begin, char** end) {
  cnBool pastSpace = cnFalse;
  char* c;
  for (c = begin; *c; c++) {
    if (isspace(*c)) {
      if (pastSpace) {
        // We found the end!
        *c = '\0';
        // Advance past the new null char.
        c++;
        break;
      }
    } else if (!pastSpace) {
      begin = c;
      pastSpace = cnTrue;
    }
  }
  if (!pastSpace) {
    // No content. Make sure it's empty.
    begin = c;
  }
  *end = c;
  return begin;
}


cnCount cnReadLine(FILE* file, cnString* string) {
  cnCount count = 0;
  int i;
  cnStringClear(string);
  while ((i = fgetc(file)) != EOF) {
    char c = (char)i;
    count++;
    if (!cnStringPushChar(string, c)) {
      count = -count;
      break;
    }
    if (c == '\n') {
      break;
    }
  }
  return count;
}
