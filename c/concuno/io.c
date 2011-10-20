#include <string.h>
#include "io.h"


void cnDedent(cnString* indent) {
  // TODO Some cnStringCountPut to considate this kind of thing?
  indent->count -= 2;
  ((char*)indent->items)[indent->count - 1] = '\0';
}


char* cnDelimit(char** string, char delimiter) {
  char* begin = *string;
  // Loop to delimiter.
  for (; **string && **string != delimiter; (*string)++) {}
  if (**string == delimiter) {
    // Found it. Slice it, point after, and return the old start.
    **string = '\0';
    (*string)++;
    return begin;
  } else {
    // No show.
    return NULL;
  }
}


cnBool cnDelimitInt(char** string, char** token, cnInt* i, char delimiter) {
  char* intEnd;
  cnInt temp;
  char* tempToken;

  // First delimit the token.
  tempToken = cnDelimit(string, delimiter);
  if (token) *token = tempToken;
  if (!tempToken) return cnFalse;

  // Now convert to int.
  // TODO Hand-code instead of using strtol to avoid dependence on errno.
  temp = strtol(tempToken, &intEnd, 10);

  // Now see if we consumed the whole token, failing if not.
  if (*intEnd) return cnFalse;

  // Must be good to go (except for ignoring errno and therefore overflow).
  *i = temp;
  return cnTrue;
}


cnBool cnIndent(cnString* indent) {
  return cnStringPushStr(indent, "  ");
}


char* cnNextChar(char* begin) {
  // Loop to nonwhite, and return that address.
  for (; *begin && isspace(*begin); begin++) {}
  return begin;
}


char cnParseChar(char* begin, char** end) {
  // Loop to nonwhite.
  for (; *begin && isspace(*begin); begin++) {}
  // Point end to where we found, or one later if not at the end of the string.
  *end = begin;
  if (*begin) (*end)++;
  // Return the char found.
  return *begin;
}

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


/**
 * I've tried various sizes for this. I wasn't convinced above 64 was doing much
 * better in my case, so I left it here. Smaller does okay, too, but it
 * gradually starts slowing down as this shrinks.
 */
#define cnReadLine_BufferSize 64

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


cnBool cnStrEndsWith(char* string, const char* ending) {
  cnCount endingLength = strlen(ending);
  cnCount stringLength = strlen(string);
  if (stringLength >= endingLength) {
    // Long enough. Check the contents.
    return strcmp(string + stringLength - endingLength, ending) ?
      // Zero means equal. Nonzero means different.
      cnFalse : cnTrue;
  }
  return cnFalse;
}
