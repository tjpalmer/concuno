#include <cuncuno.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "load.h"


typedef struct stParser {
  cnList(cnIndex) indices;
  stState state;
  cnList(stState)* states;
} stParser;


/**
 * Individual parse handlers for specific commands.
 */
cnBool stHandleAlive(stParser* parser, char* args);
cnBool stHandleClear(stParser* parser, char* args);
cnBool stHandleColor(stParser* parser, char* args);
cnBool stHandleDestroy(stParser* parser, char* args);
cnBool stHandleExtent(stParser* parser, char* args);
cnBool stHandleGrasp(stParser* parser, char* args);
cnBool stHandleItem(stParser* parser, char* args);
cnBool stHandlePos(stParser* parser, char* args);
cnBool stHandlePosVel(stParser* parser, char* args);
cnBool stHandleRelease(stParser* parser, char* args);
cnBool stHandleRot(stParser* parser, char* args);
cnBool stHandleRotVel(stParser* parser, char* args);
cnBool stHandleTime(stParser* parser, char* args);
cnBool stHandleType(stParser* parser, char* args);


/**
 * Parses a single line, returning true for no error.
 */
cnBool stParseLine(stParser* parser, cnString* line);


/**
 * Finds a non-whitespace string if it exists, overwriting the first trailing
 * whitespace (if any) with a null char. The end will point past that null
 * char if added, or at the already existing null char if at the end. This
 * function returns where the first non-whitespace was found, if any, for usage
 * like below:
 *
 * char* string = stParseString(source, &source);
 */
char* stParseString(char* begin, char** end);


stItem* stParserItem(stParser* parser, char* begin, char** end);


cnBool stPushState(stParser* parser);


cnBool stLoad(char* name, cnList(stState)* states) {
  cnBool result = cnTrue;
  int closeError;
  cnString line;
  cnCount lineCount, readCount;
  stParser parser;
  parser.states = states;
  stStateInit(&parser.state);
  cnListInit(&parser.indices, sizeof(cnIndex));
  // Open file.
  FILE* file = fopen(name, "r");
  if (!file) {
    printf("Failed to open: %s\n", name);
    return cnFalse;
  }
  printf("Parsing %s ...\n", name);
  // TODO Init state.
  // Read lines.
  cnStringInit(&line);
  lineCount = 0;
  while ((readCount = cnReadLine(file, &line)) > 0) {
    //printf("Line: %s", cnStr(&line));
    lineCount++;
    if (!stParseLine(&parser, &line)) {
      // TODO Distinguish parse errors from memory allocation fails.
      printf(
          "Error parsing line %ld of %s: %s\n", lineCount, name, cnStr(&line)
      );
      result = cnFalse;
      break;
    }
  }
  // Grab the last state.
  if (!stPushState(&parser)) {
    result = cnFalse;
  }
  // Clean up.
  cnStringDispose(&line);
  cnListDispose(&parser.indices);
  stStateDispose(&parser.state);
  closeError = fclose(file);
  if (readCount < 0 || closeError) {
    printf("Error reading or closing: %s\n", name);
    result = cnFalse;
  }
  return result;
}


cnBool stHandleAlive(stParser* parser, char* args) {
  stItem* item = stParserItem(parser, args, &args);
  char* status = stParseString(args, &args);
  if (!item || !*status) {
    return cnFalse;
  }
  item->alive = !strcmp(status, "true");
  return cnTrue;
}


cnBool stHandleClear(stParser* parser, char* args) {
  parser->state.cleared = cnTrue;
  return cnTrue;
}


cnBool stHandleColor(stParser* parser, char* args) {
  stItem* item = stParserItem(parser, args, &args);
  // TODO Use HSV colorspace to begin with?
  // TODO Verify we haven't run out of args?
  item->color[0] = strtod(args, &args);
  item->color[1] = strtod(args, &args);
  item->color[2] = strtod(args, &args);
  // Ignore opacity, the 4th value. It's bogus for now.
  return cnTrue;
}


cnBool stHandleDestroy(stParser* parser, char* args) {
  stId id = strtol(args, &args, 10);
  cnIndex* index = (cnIndex*)cnListGet(&parser->indices, id);
  cnIndex* indices = parser->indices.items;
  cnList(stItem)* items;
  stItem *item, *endItem;
  if (!index) {
    printf("Bad id: %ld\n", id);
    return cnFalse;
  }
  if (!*index) {
    printf("Already destroyed: %ld\n", id);
    return cnFalse;
  }
  // Remove the destroyed item.
  items = &parser->state.items;
  cnListRemove(items, *index);
  endItem = cnListEnd(items);
  // Reduce the index of successive items.
  for (item = cnListGet(items, *index); item < endItem; item++) {
    // All items ids in our list should be valid, so index directly.
    // TODO Could optimize further if we assume seeing items always in
    // TODO increasing order.
    indices[item->id]--;
  }
  *index = 0;
  return cnTrue;
}


cnBool stHandleExtent(stParser* parser, char* args) {
  stItem* item = stParserItem(parser, args, &args);
  item->extent[0] = strtod(args, &args);
  item->extent[1] = strtod(args, &args);
  return cnTrue;
}


cnBool stHandleGrasp(stParser* parser, char* args) {
  stItem* tool = stParserItem(parser, args, &args);
  stItem* item = stParserItem(parser, args, &args);
  // TODO Parse and use the relative grasp location?
  tool->grasping = cnTrue;
  item->grasped = cnTrue;
  return cnTrue;
}


cnBool stHandleItem(stParser* parser, char* args) {
  stItem item;
  stId badId = 0;
  cnIndex i, index = parser->state.items.count;
  stItemInit(&item);
  item.id = strtol(args, &args, 10);
  // TODO Verify against duplicate ID?
  // TODO Extra data copy here. Do I care?
  if (!cnListPush(&parser->state.items, &item)) {
    return cnFalse;
  }
  // TODO Bulk expand?
  for (i = parser->indices.count; i <= item.id; i++) {
    if (!cnListPush(&parser->indices, &badId)) {
      return cnFalse;
    }
  }
  // Guaranteed good array spot here.
  ((cnIndex*)parser->indices.items)[item.id] = index;
  return cnTrue;
}


cnBool stHandlePos(stParser* parser, char* args) {
  stItem* item = stParserItem(parser, args, &args);
  // TODO Verify we haven't run out of args or have other errors?
  item->location[0] = strtod(args, &args);
  item->location[1] = strtod(args, &args);
  return cnTrue;
}


cnBool stHandlePosVel(stParser* parser, char* args) {
  stItem* item = stParserItem(parser, args, &args);
  item->velocity[0] = strtod(args, &args);
  item->velocity[1] = strtod(args, &args);
  return cnTrue;
}


cnBool stHandleRelease(stParser* parser, char* args) {
  stItem* tool = stParserItem(parser, args, &args);
  stItem* item = stParserItem(parser, args, &args);
  tool->grasping = cnFalse;
  item->grasped = cnFalse;
  return cnTrue;
}


cnBool stHandleRot(stParser* parser, char* args) {
  stItem* item = stParserItem(parser, args, &args);
  // TODO Angle is in rats. Convert to radians or not?
  item->orientation = strtod(args, &args);
  return cnTrue;
}


cnBool stHandleRotVel(stParser* parser, char* args) {
  stItem* item = stParserItem(parser, args, &args);
  // TODO Angular velocity is in rats. Convert to radians or not?
  item->orientationVelocity = strtod(args, &args);
  return cnTrue;
}


cnBool stHandleTime(stParser* parser, char* args) {
  cnCount steps;
  char* type = stParseString(args, &args);
  if (!strcmp(type, "sim")) {
    if (!stPushState(parser)) {
      return cnFalse;
    }
    // TODO Some general 'reset' function?
    parser->state.cleared = cnFalse;
    // Just eat the number of steps for now. Maybe I'll care more about it
    // later.
    steps = strtol(args, &args, 10);
    // I pretend the sim time (in seconds) is what matters here.
    parser->state.time = strtod(args, &args);
  }
  return cnTrue;
}


cnBool stHandleType(stParser* parser, char* args) {
  stItem* item = stParserItem(parser, args, &args);
  char* type = stParseString(args, &args);
  if (!strcmp(type, "block")) {
    item->type = stTypeBlock;
  } else if (!strcmp(type, "tool")) {
    item->type = stTypeTool;
  } else {
    // TODO Handle others?
  }
  return cnTrue;
}


cnBool stParseLine(stParser* parser, cnString* line) {
  // TODO Extract command then scanf it?
  char *args, *command;
  cnBool (*parse)(stParser* parser, char* args) = NULL;
  command = stParseString(line->items, &args);
  // TODO Hashtable? This is still quite fast.
  if (!strcmp(command, "alive")) {
    parse = stHandleAlive;
  } else if (!strcmp(command, "clear")) {
    parse = stHandleClear;
  } else if (!strcmp(command, "color")) {
    parse = stHandleColor;
  } else if (!strcmp(command, "destroy")) {
    parse = stHandleDestroy;
  } else if (!strcmp(command, "extent")) {
    parse = stHandleExtent;
  } else if (!strcmp(command, "grasp")) {
    parse = stHandleGrasp;
  } else if (!strcmp(command, "item")) {
    parse = stHandleItem;
  } else if (!strcmp(command, "pos")) {
    parse = stHandlePos;
  } else if (!strcmp(command, "posvel")) {
    parse = stHandlePosVel;
  } else if (!strcmp(command, "release")) {
    parse = stHandleRelease;
  } else if (!strcmp(command, "rot")) {
    parse = stHandleRot;
  } else if (!strcmp(command, "rotvel")) {
    parse = stHandleRotVel;
  } else if (!strcmp(command, "time")) {
    parse = stHandleTime;
  } else if (!strcmp(command, "type")) {
    parse = stHandleType;
  }
  if (parse) {
    return parse(parser, args);
  } else {
    // TODO List explicit known okay ignore commands?
    //printf("Unknown command: %s\n", command);
    return cnTrue;
  }
}


char* stParseString(char* begin, char** end) {
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


stItem* stParserItem(stParser* parser, char* begin, char** end) {
  // TODO Better validation?
  stId id = strtol(begin, end, 10);
  cnIndex* index = (cnIndex*)cnListGet(&parser->indices, id);
  return cnListGet(&parser->state.items, *index);
}


cnBool stPushState(stParser* parser) {
  // Save a copy of the current state.
  stState state;
  if (!stStateCopy(&state, &parser->state)) {
    printf("Failed state copy.\n");
    return cnFalse;
  }
  if (!cnListPush(parser->states, &state)) {
    printf("Failed to push state copy.\n");
    return cnFalse;
  }
  return cnTrue;
}