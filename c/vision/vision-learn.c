#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vision-learn.h"


cnBool cnvLoadBags(char* indicatorsName, char* featuresName);


cnBool cnvLoadGroupedMatrix(char* name);


cnBool cnvPushOrExpandProperty(cnType* type, char* propertyName);


/**
 * Returns the created entity type or null for failure.
 */
cnType* cnvSchemaInit(cnSchema* schema);


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
  FILE* file = NULL;
  cnList(char*) headers;
  cnString line;
  char* remaining;
  cnSchema schema;
  cnType* type;

  // Init the data.
  cnStringInit(&line);
  cnListInit(&headers, sizeof(cnString));
  if (!(type = cnvSchemaInit(&schema))) cnFailTo(DONE, "No schema.");

  // Open the file.
  file = fopen(name, "r");
  if (!file) cnFailTo(DONE, "Couldn't open '%s'.", name);

  // Read the headers.
  if (!cnReadLine(file, &line)) cnFailTo(DONE, "No headers.");
  remaining = cnStr(&line);
  while (cnTrue) {
    //char* header;
    char* next = cnParseStr(remaining, &remaining);
    if (!*next) break;
    if (next <= (char*)line.items) continue; // File group name. Ignored.
    if (!cnvPushOrExpandProperty(type, next)) cnFailTo(DONE, "Property stuck.");
    //header = malloc(strlen(next) + 1);
    //if (!header) cnFailTo(DONE, "No header string.");
    //strcpy(header, next);
    // TODO If not already there: if (!cnListPush(&headers, header))
  }

  printf("type %s of size %ld\n", cnStr(&type->name), type->size);
  cnListEachBegin(&type->properties, cnProperty, property) {
    printf(
      "  var %s: %s[%ld]\n",
      cnStr(&property->name), cnStr(&property->type->name), property->count
    );
  } cnEnd;

  DONE:
  cnSchemaDispose(&schema);
  cnListDispose(&headers);
  cnStringDispose(&line);
  if (file) fclose(file);
  return result;
}


cnBool cnvPushOrExpandProperty(cnType* type, char* propertyName) {
  cnBool found = cnFalse;
  cnCount laterOffset = 0;
  cnBool result = cnFalse;

  // Find the property with this name.
  cnListEachBegin(&type->properties, cnProperty, property) {
    if (!strcmp(cnStr(&property->name), propertyName)) {
      // Found it. Expand it, and offset following ones.
      found = cnTrue;
      property->count++;
      laterOffset = property->type->size;
    } else if (found) {
      // Found a previous property, so offset this following one.
      property->offset += laterOffset;
    }
  } cnEnd;

  if (!found) {
    // Didn't find anything, so push a new property.
    cnProperty* property = cnListExpand(&type->properties);
    if (!property) cnFailTo(DONE, "Failed to alloc %s property.", propertyName);
    // TODO Assumed always float for now.
    if (!cnPropertyInitField(
      property, type, type->schema->floatType, propertyName, type->size, 1
    )) cnFailTo(DONE, "Failed to init %s property.", propertyName);
    laterOffset = property->type->size;
  }

  // Update the type's overall size, too.
  type->size += laterOffset;
  result = cnTrue;

  DONE:
  return result;
}


cnType* cnvSchemaInit(cnSchema* schema) {
  cnType* type = NULL;

  if (!cnSchemaInitDefault(schema)) cnFailTo(DONE, "No default schema.");
  if (!(type = cnListExpand(&schema->types))) {
    cnFailTo(FAIL, "No type for schema.");
  }
  if (!cnTypeInit(type, "Entity", 0)) { // What size, really?
    type = NULL;
    cnFailTo(FAIL, "No type init.");
  }
  type->schema = schema;

  // We winned! Skip fail, too.
  goto DONE;

  FAIL:
  cnSchemaDispose(schema);

  DONE:
  return type;
}
