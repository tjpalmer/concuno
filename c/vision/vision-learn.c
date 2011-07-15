#include <stdio.h>
#include <stdlib.h>

#include "vision-learn.h"


cnBool cnvLoadBags(char* indicatorsName, char* featuresName);


cnBool cnvLoadGroupedMatrix(char* name);


cnBool cnvSchemaInit(cnSchema* schema);


int main(int argc, char** argv) {
  int result = EXIT_FAILURE;

  if (argc < 3) cnFailTo(DONE, "Tell me which two files to open!");
  cnvLoadBags(argv[1], argv[2]);
  result = EXIT_SUCCESS;

  DONE:
  return result;
}


cnBool cnvLoadBags(char* indicatorsName, char* featuresName) {
  cnBool result = cnFalse;
  cnvLoadGroupedMatrix(featuresName);
  return result;
}


cnBool cnvLoadGroupedMatrix(char* name) {
  cnBool result = cnFalse;
  FILE* file;
  cnList(char*) headers;
  cnString line;
  char* remaining;

  // Open the file.
  file = fopen(name, "r");
  if (!file) cnFailTo(DONE, "Couldn't open '%s'.", name);

  // Init the data.
  cnStringInit(&line);
  cnListInit(&headers, sizeof(cnString));

  // Read the headers.
  if (!cnReadLine(file, &line)) cnFailTo(DONE, "No headers.");
  remaining = cnStr(&line);
  while (cnTrue) {
    //char* header;
    char* next = cnParseStr(remaining, &remaining);
    if (!*next) break;
    printf("%s\n", next);
    //header = malloc(strlen(next) + 1);
    //if (!header) cnFailTo(DONE, "No header string.");
    //strcpy(header, next);
    // TODO If not already there: if (!cnListPush(&headers, header))
  }

  DONE:
  cnStringDispose(&line);
  if (file) fclose(file);
  return result;
}


cnBool cnvSchemaInit(cnSchema* schema) {
  cnBool result = cnFalse;
  cnType* type;

  if (!cnSchemaInitDefault(schema)) cnFailTo(DONE, "No default schema.");
  if (!(type = cnListExpand(&schema->types))) {
    cnFailTo(FAIL, "No type for schema.");
  }
  if (!cnTypeInit(type, "Entity", 0)) { // What size, really?
    // Pretend the type isn't there.
    schema->types.count--;
    cnFailTo(FAIL, "No type init.");
  }

  // We winned! Skip fail, too.
  result = cnTrue;
  goto DONE;

  FAIL:
  cnSchemaDispose(schema);

  DONE:
  return result;
}
