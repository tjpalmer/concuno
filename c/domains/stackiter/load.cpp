#include <concuno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "load.h"

using namespace concuno;


namespace ccndomain {namespace stackiter {


struct Parser {
  List<Index> indices;
  State state;
  List<State>* states;
};


/**
 * Individual parse handlers for specific commands.
 */
bool handleAlive(Parser* parser, char* args);
bool handleClear(Parser* parser, char* args);
bool handleColor(Parser* parser, char* args);
bool handleDestroy(Parser* parser, char* args);
bool handleExtent(Parser* parser, char* args);
bool handleGrasp(Parser* parser, char* args);
bool handleItem(Parser* parser, char* args);
bool handlePos(Parser* parser, char* args);
bool handlePosVel(Parser* parser, char* args);
bool handleRelease(Parser* parser, char* args);
bool handleRot(Parser* parser, char* args);
bool handleRotVel(Parser* parser, char* args);
bool handleTime(Parser* parser, char* args);
bool handleType(Parser* parser, char* args);


/**
 * Parses a single line, returning true for no error.
 */
bool parseLine(Parser* parser, String* line);


Item* parserItem(Parser* parser, char* begin, char** end);


void pushState(Parser* parser);


bool load(char* name, List<State>* states) {
  bool result = true;
  int closeError;
  FILE* file;
  String line;
  Count lineCount, readCount;
  Parser parser;
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
    if (!parseLine(&parser, &line)) {
      // TODO Distinguish parse errors from memory allocation fails.
      printf(
          "Error parsing line %ld of %s: %s\n", lineCount, name, cnStr(&line)
      );
      result = false;
      break;
    }
  }
  // Grab the last state.
  pushState(&parser);
  // Clean up.
  closeError = fclose(file);
  if (readCount < 0 || closeError) {
    printf("Error reading or closing: %s\n", name);
    result = false;
  }
  return result;
}


bool handleAlive(Parser* parser, char* args) {
  Item* item = parserItem(parser, args, &args);
  char* status = cnParseStr(args, &args);
  if (!item || !*status) {
    return false;
  }
  item->alive = !strcmp(status, "true");
  return true;
}


bool handleClear(Parser* parser, char* args) {
  parser->state.cleared = true;
  return true;
}


bool handleColor(Parser* parser, char* args) {
  Item* item = parserItem(parser, args, &args);
  // TODO Use HSV colorspace to begin with?
  // TODO Verify we haven't run out of args?
  item->color[0] = strtod(args, &args);
  item->color[1] = strtod(args, &args);
  item->color[2] = strtod(args, &args);
  // Ignore opacity, the 4th value. It's bogus for now.
  return true;
}


bool handleDestroy(Parser* parser, char* args) {
  Id id = strtol(args, &args, 10);
  Index& index = parser->indices[id];
  if (!index) {
    printf("Already destroyed: %ld\n", id);
    return false;
  }
  // Remove the destroyed item.
  List<Item>& items = parser->state.items;
  cnListRemove(&items, index);
  if (index < items.count) {
    // It wasn't last, so reduce the index of successive items.
    Item* endItem = reinterpret_cast<Item*>(cnListEnd(&items));
    Item* item = &items[index];
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


bool handleExtent(Parser* parser, char* args) {
  Item* item = parserItem(parser, args, &args);
  item->extent[0] = strtod(args, &args);
  item->extent[1] = strtod(args, &args);
  return true;
}


bool handleGrasp(Parser* parser, char* args) {
  Item* tool = parserItem(parser, args, &args);
  Item* item = parserItem(parser, args, &args);
  // TODO Parse and use the relative grasp location?
  tool->grasping = true;
  item->grasped = true;
  return true;
}


bool handleItem(Parser* parser, char* args) {
  Item item;
  Id badId = 0;
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


bool handlePos(Parser* parser, char* args) {
  Item* item = parserItem(parser, args, &args);
  // TODO Verify we haven't run out of args or have other errors?
  item->location[0] = strtod(args, &args);
  item->location[1] = strtod(args, &args);
  return true;
}


bool handlePosVel(Parser* parser, char* args) {
  Item* item = parserItem(parser, args, &args);
  item->velocity[0] = strtod(args, &args);
  item->velocity[1] = strtod(args, &args);
  return true;
}


bool handleRelease(Parser* parser, char* args) {
  Item* tool = parserItem(parser, args, &args);
  Item* item = parserItem(parser, args, &args);
  tool->grasping = false;
  item->grasped = false;
  return true;
}


bool handleRot(Parser* parser, char* args) {
  Item* item = parserItem(parser, args, &args);
  // TODO Angle is in rats. Convert to radians or not?
  item->orientation = strtod(args, &args);
  return true;
}


bool handleRotVel(Parser* parser, char* args) {
  Item* item = parserItem(parser, args, &args);
  // TODO Angular velocity is in rats. Convert to radians or not?
  item->orientationVelocity = strtod(args, &args);
  return true;
}


bool handleTime(Parser* parser, char* args) {
  Count steps;
  char* type = cnParseStr(args, &args);
  if (!strcmp(type, "sim")) {
    pushState(parser);
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


bool handleType(Parser* parser, char* args) {
  Item* item = parserItem(parser, args, &args);
  char* type = cnParseStr(args, &args);
  if (!strcmp(type, "block")) {
    item->type = Item::TypeBlock;
  } else if (!strcmp(type, "tool")) {
    item->type = Item::TypeTool;
  } else {
    // TODO Handle others?
  }
  return true;
}


bool parseLine(Parser* parser, String* line) {
  // TODO Extract command then scanf it?
  char *args, *command;
  bool (*parse)(Parser* parser, char* args) = NULL;
  command = cnParseStr(cnStr(line), &args);
  // TODO Hashtable? This is still quite fast.
  if (!strcmp(command, "alive")) {
    parse = handleAlive;
  } else if (!strcmp(command, "clear")) {
    parse = handleClear;
  } else if (!strcmp(command, "color")) {
    parse = handleColor;
  } else if (!strcmp(command, "destroy")) {
    parse = handleDestroy;
  } else if (!strcmp(command, "extent")) {
    parse = handleExtent;
  } else if (!strcmp(command, "grasp")) {
    parse = handleGrasp;
  } else if (!strcmp(command, "item")) {
    parse = handleItem;
  } else if (!strcmp(command, "pos")) {
    parse = handlePos;
  } else if (!strcmp(command, "posvel")) {
    parse = handlePosVel;
  } else if (!strcmp(command, "release")) {
    parse = handleRelease;
  } else if (!strcmp(command, "rot")) {
    parse = handleRot;
  } else if (!strcmp(command, "rotvel")) {
    parse = handleRotVel;
  } else if (!strcmp(command, "time")) {
    parse = handleTime;
  } else if (!strcmp(command, "type")) {
    parse = handleType;
  }
  if (parse) {
    return parse(parser, args);
  } else {
    // TODO List explicit known okay ignore commands?
    //printf("Unknown command: %s\n", command);
    return true;
  }
}


Item* parserItem(Parser* parser, char* begin, char** end) {
  // TODO Better validation?
  Id id = strtol(begin, end, 10);
  Index index = parser->indices[id];
  return &parser->state.items[index];
}


void pushState(Parser* parser) {
  // Save a copy of the current state.
  State* state;
  if (!(state = reinterpret_cast<State*>(cnListExpand(parser->states)))) {
    throw Error("Failed to expand states for copy.");
  }
  if (!stateCopy(state, &parser->state)) {
    throw Error("Failed state copy.");
  }
}


}}
