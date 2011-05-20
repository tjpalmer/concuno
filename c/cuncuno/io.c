#include "io.h"

cnCount cnReadLine(FILE* file, cnString* string) {
  cnCount count = 0;
  int i;
  cnStringClear(string);
  while ((i = fgetc(file)) != EOF) {
    cnChar c = (cnChar)i;
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
