#include <concuno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "load.h"

using namespace concuno;


typedef struct stParser {
  List<Index> indices;
  stState state;
  List<stState>* states;
} stParser;


/**
 * Individual parse handlers for specific commands.
 */
bool stHandleAlive(stParser* parser, char* args);
bool stHandleClear(stParser* parser, char* args);
bool stHandleColor(stParser* parser, char* args);
bool stHandleDestroy(stParser* parser, char* args);
bool stHandleExtent(stParser* parser, char* args);
bool stHandleGrasp(stParser* parser, char* args);
bool stHandleItem(stParser* parser, char* args);
bool stHandlePos(stParser* parser, char* args);
bool stHandlePosVel(stParser* parser, char* args);
bool stHandleRelease(stParser* parser, char* args);
bool stHandleRot(stParser* parser, char* args);
bool stHandleRotVel(stParser* parser, char* args);
bool stHandleTime(stParser* parser, char* args);
bool stHandleType(stParser* parser, char* args);


/**
 * Parses a single line, returning true for no error.
 */
bool stParseLine(stParser* parser, String* line);


stItem* stParserItem(stParser* parser, char* begin, char** end);


void stPushState(stParser* parser);


bool stLoad(char* name, List<stState>* states) {
  bool result = true;
  int closeError;
  FILE* file;
  String line;
  Count lineCount, readCount;
  stParser parser;
  parser.states = states;
  // Open file.
  file = fopen(name, "r");
  if (!file) {
    printf("Failed to open: %s\n", name);
    return false;
  }
  printf("Parsing %s ...\n", name);
  // TODO Init state.
  // Read lines.
  lineCount = 0;
  while ((readCount = cnReadLine(file, &line)) > 0) {
    //printf("Line: %s", cnStr(&line));
    lineCount++;
    if (!stParseLine(&parser, &line)) {
      // TODO Distinguish parse errors from memory allocation fails.
      printf(
          "Error parsing line %ld of %s: %s\n", lineCount, name, cnStr(&line)
      );
      result = false;
      break;
    }
  }
  // Grab the last state.
  stPushState(&parser);
  // Clean up.
  closeError = fclose(file);
  if (readCount < 0 || closeError) {
    printf("Error reading or closing: %s\n", name);
    result = false;
  }
  return result;
}


bool stHandleAlive(stParser* parser, char* args) {
  stItem* item = stParserItem(parser, args, &args);
  char* status = cnParseStr(args, &args);
  if (!item || !*status) {
    return false;
  }
  item->alive = !strcmp(status, "true");
  return true;
}


bool stHandleClear(stParser* parser, char* args) {
  parser->state.cleared = true;
  return true;
}


bool stHandleColor(stParser* parser, char* args) {
  stItem* item = stParserItem(parser, args, &args);
  // TODO Use HSV colorspace to begin with?
  // TODO Verify we haven't run out of args?
  item->color[0] = strtod(args, &args);
  item->color[1] = strtod(args, &args);
  item->color[2] = strtod(args, &args);
  // Ignore opacity, the 4th value. It's bogus for now.
  return true;
}


bool stHandleDestroy(stParser* parser, char* args) {
  stId id = strtol(args, &args, 10);
  Index& index = parser->indices[id];
  if (!index) {
    printf("Already destroyed: %ld\n", id);
    return false;
  }
  // Remove the destroyed item.
  List<stItem>& items = parser->state.items;
  cnListRemove(&items, index);
  if (index < items.count) {
    // It wasn't last, so reduce the index of successive items.
    stItem* endItem = reinterpret_cast<stItem*>(cnListEnd(&items));
    stItem* item = &items[index];
    for (; item < endItem; item++) {
      // All items ids in our list should be valid, so index directly.
      // TODO Could optimize further if we assume seeing items always in
      // TODO increasing order.
      parser->indices[item->id]--;
    }
  }
  index = 0;
  return true;
}


bool stHandleExtent(stParser* parser, char* args) {
  stItem* item = stParserItem(parser, args, &args);
  item->extent[0] = strtod(args, &args);
  item->extent[1] = strtod(args, &args);
  return true;
}


bool stHandleGrasp(stParser* parser, char* args) {
  stItem* tool = stParserItem(parser, args, &args);
  stItem* item = stParserItem(parser, args, &args);
  // TODO Parse and use the relative grasp location?
  tool->grasping = true;
  item->grasped = true;
  return true;
}


bool stHandleItem(stParser* parser, char* args) {
  stItem item;
  stId badId = 0;
  Index i, index = parser->state.items.count;
  stItemInit(&item);
  item.id = strtol(args, &args, 10);
  // TODO Verify against duplicate ID?
  // TODO Extra data copy here. Do I care?
  if (!cnListPush(&parser->state.items, &item)) {
    return false;
  }
  // TODO Bulk expand?
  for (i = parser->indices.count; i <= item.id; i++) {
    if (!cnListPush(&parser->indices, &badId)) {
      return false;
    }
  }
  // Guaranteed good array spot here.
  ((Index*)parser->indices.items)[item.id] = index;
  return true;
}


bool stHandlePos(stParser* parser, char* args) {
  stItem* item = stParserItem(parser, args, &args);
  // TODO Verify we haven't run out of args or have other errors?
  item->location[0] = strtod(args, &args);
  item->location[1] = strtod(args, &args);
  return true;
}


bool stHandlePosVel(stParser* parser, char* args) {
  stItem* item = stParserItem(parser, args, &args);
  item->velocity[0] = strtod(args, &args);
  item->velocity[1] = strtod(args, &args);
  return true;
}


bool stHandleRelease(stParser* parser, char* args) {
  stItem* tool = stParserItem(parser, args, &args);
  stItem* item = stParserItem(parser, args, &args);
  tool->grasping = false;
  item->grasped = false;
  return true;
}


bool stHandleRot(stParser* parser, char* args) {
  stItem* item = stParserItem(parser, args, &args);
  // TODO Angle is in rats. Convert to radians or not?
  item->orientation = strtod(args, &args);
  return true;
}


bool stHandleRotVel(stParser* parser, char* args) {
  stItem* item = stParserItem(parser, args, &args);
  // TODO Angular velocity is in rats. Convert to radians or not?
  item->orientationVelocity = strtod(args, &args);
  return true;
}


bool stHandleTime(stParser* parser, char* args) {
  Count steps;
  char* type = cnParseStr(args, &args);
  if (!strcmp(type, "sim")) {
    stPushState(parser);
    // TODO Some general 'reset' function?
    parser->state.cleared = false;
    // Just eat the number of steps for now. Maybe I'll care more about it
    // later.
    steps = strtol(args, &args, 10);
    // I pretend the sim time (in seconds) is what matters here.
    parser->state.time = strtod(args, &args);
  }
  return true;
}


bool stHandleType(stParser* parser, char* args) {
  stItem* item = stParserItem(parser, args, &args);
  char* type = cnParseStr(args, &args);
  if (!strcmp(type, "block")) {
    item->type = stTypeBlock;
  } else if (!strcmp(type, "tool")) {
    item->type = stTypeTool;
  } else {
    // TODO Handle others?
  }
  return true;
}


bool stParseLine(stParser* parser, String* line) {
  // TODO Extract command then scanf it?
  char *args, *command;
  bool (*parse)(stParser* parser, char* args) = NULL;
  command = cnParseStr(cnStr(line), &args);
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
    return true;
  }
}


stItem* stParserItem(stParser* parser, char* begin, char** end) {
  // TODO Better validation?
  stId id = strtol(begin, end, 10);
  Index index = parser->indices[id];
  return &parser->state.items[index];
}


void stPushState(stParser* parser) {
  // Save a copy of the current state.
  stState* state;
  if (!(state = reinterpret_cast<stState*>(cnListExpand(parser->states)))) {
    throw Error("Failed to expand states for copy.");
  }
  if (!stStateCopy(state, &parser->state)) {
    throw Error("Failed state copy.");
  }
}
