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


#define cnReadLine_BufferSize 1024

cnCount cnReadLine(FILE* file, cnString* string) {
  char buffer[cnReadLine_BufferSize];
  cnIndex bufferIndex = 0;
  cnCount count = 0;
  cnCount maxRead = cnReadLine_BufferSize - 1;
  int i;
  cnStringClear(string);
  while ((i = fgetc(file)) != EOF) {
    char c = (char)i;
    buffer[bufferIndex++] = c;
    if (bufferIndex == maxRead || c == '\n') {
      count += bufferIndex;
      buffer[bufferIndex++] = '\0';
      // Add the buffer at one go.
      if (!cnStringPushStr(string, buffer)) {
        count = -count;
        break;
      }
      if (c == '\n') {
        // All done.
        break;
      }
      // Start the buffer over again, and keep going.
      bufferIndex = 0;
    }
  }
  return count;
}
