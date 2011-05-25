#include <cuncuno.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "loader.h"


typedef struct stParser {
  cnList indices;
  stState state;
  cnList* states;
} stParser;


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


/**
 * Individual parse handlers for specific commands.
 */
cnBool stParseAlive(stParser* parser, char* args);
cnBool stParseClear(stParser* parser, char* args);
cnBool stParseColor(stParser* parser, char* args);
cnBool stParseDestroy(stParser* parser, char* args);
cnBool stParseExtent(stParser* parser, char* args);
cnBool stParseGrasp(stParser* parser, char* args);
cnBool stParseItem(stParser* parser, char* args);
cnBool stParsePos(stParser* parser, char* args);
cnBool stParsePosVel(stParser* parser, char* args);
cnBool stParseRelease(stParser* parser, char* args);
cnBool stParseRot(stParser* parser, char* args);
cnBool stParseRotVel(stParser* parser, char* args);
cnBool stParseTime(stParser* parser, char* args);
cnBool stParseType(stParser* parser, char* args);


cnBool stLoad(char* name, cnList* states) {
  cnBool result = cnTrue;
  int closeError;
  cnString line;
  cnCount lineCount, readCount;
  stParser parser;
  stState state;
  stStateInit(&parser.state);
  cnListInit(&parser.indices, sizeof(cnIndex));
  // Open file.
  FILE* file = fopen(name, "r");
  if (!file) {
    printf("Failed to open: %s\n", name);
    return cnFalse;
  }
  printf("Opened: %s\n", name);
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
  printf("Lines: %ld\n", lineCount);
  printf("Items: %ld\n", parser.state.items.count);
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


cnBool stParseLine(stParser* parser, cnString* line) {
  // TODO Extract command then scanf it?
  char *args, *command;
  cnBool (*parse)(stParser* parser, char* args) = NULL;
  command = stParseString(line->items, &args);
  // TODO Hashtable? This is still quite fast.
  if (!strcmp(command, "alive")) {
    parse = stParseAlive;
  } else if (!strcmp(command, "clear")) {
    parse = stParseClear;
  } else if (!strcmp(command, "color")) {
    parse = stParseColor;
  } else if (!strcmp(command, "destroy")) {
    parse = stParseDestroy;
  } else if (!strcmp(command, "extent")) {
    parse = stParseExtent;
  } else if (!strcmp(command, "grasp")) {
    parse = stParseGrasp;
  } else if (!strcmp(command, "item")) {
    parse = stParseItem;
  } else if (!strcmp(command, "pos")) {
    parse = stParsePos;
  } else if (!strcmp(command, "posvel")) {
    parse = stParsePosVel;
  } else if (!strcmp(command, "release")) {
    parse = stParseRelease;
  } else if (!strcmp(command, "rot")) {
    parse = stParseRot;
  } else if (!strcmp(command, "rotvel")) {
    parse = stParseRotVel;
  } else if (!strcmp(command, "time")) {
    parse = stParseTime;
  } else if (!strcmp(command, "type")) {
    parse = stParseType;
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


cnBool stParseAlive(stParser* parser, char* args) {
  char* status;
  stItem* item;
  item = stParserItem(parser, args, &args);
  status = stParseString(args, &args);
  if (!item || !*status) {
    return cnFalse;
  }
  item->alive = !strcmp(status, "true");
  return cnTrue;
}


cnBool stParseClear(stParser* parser, char* args) {
  parser->state.cleared = cnTrue;
  return cnTrue;
}


cnBool stParseColor(stParser* parser, char* args) {
  stItem* item = stParserItem(parser, args, &args);
  // TODO Use HSV colorspace to begin with?
  // TODO Verify we haven't run out of args?
  item->color[0] = strtod(args, &args);
  item->color[1] = strtod(args, &args);
  item->color[2] = strtod(args, &args);
  // Ignore opacity, the 4th value. It's bogus for now.
  return cnTrue;
}


cnBool stParseDestroy(stParser* parser, char* args) {
  cnList* indices = &parser->indices;
  stId id = strtol(args, &args, 10);
  cnIndex* index = (cnIndex*)cnListGet(indices, id);
  cnList* items;
  stItem *item, *endItem;
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
    // TODO Could optimize further if we assume seeing items always in
    // TODO increasing order.
    (*(cnIndex*)cnListGet(indices, item->id))--;
  }
  *index = 0;
  return cnTrue;
}


cnBool stParseExtent(stParser* parser, char* args) {
  return cnTrue;
}


cnBool stParseGrasp(stParser* parser, char* args) {
  return cnTrue;
}


cnBool stParseItem(stParser* parser, char* args) {
  stItem item;
  stId badId = 0;
  cnIndex i, index = parser->state.items.count;
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
  *(cnIndex*)cnListGet(&parser->indices, item.id) = index;
  return cnTrue;
}


cnBool stParsePos(stParser* parser, char* args) {
  return cnTrue;
}


cnBool stParsePosVel(stParser* parser, char* args) {
  return cnTrue;
}


cnBool stParseRelease(stParser* parser, char* args) {
  return cnTrue;
}


cnBool stParseRot(stParser* parser, char* args) {
  return cnTrue;
}


cnBool stParseRotVel(stParser* parser, char* args) {
  return cnTrue;
}


cnBool stParseTime(stParser* parser, char* args) {
  return cnTrue;
}


cnBool stParseType(stParser* parser, char* args) {
  return cnTrue;
}


/*

void Loader::handleExtent(stringstream& tokens) {
  Item& item = getItem(tokens);
  tokens >> item.extent[0];
  tokens >> item.extent[1];
}

int Loader::handleId(stringstream& tokens) {
  int id;
  tokens >> id;
  if (id <= 0) {
    throw "nonpositive id";
  }
  return id;
}

void Loader::handleGrasp(stringstream& tokens) {
  Item& tool = getItem(tokens);
  Item& item = getItem(tokens);
  // TODO Parse and use the relative grasp location?
  tool.grasping = true;
  item.grasped = true;
}

void Loader::handleItem(stringstream& tokens) {
  Item item;
  item.id = handleId(tokens);
  // TODO Verify against duplicate ID?
  // TODO Evil data copy here. Do I care?
  state.items.push_back(item);
  while (indexes.size() < static_cast<size_t>(item.id + 1)) {
    // TODO Or is it 0 by default for resize?
    indexes.push_back(0);
  }
  indexes[item.id] = state.items.size() - 1;
}

void Loader::handleLocation(stringstream& tokens) {
  Item& item = getItem(tokens);
  tokens >> item.location[0];
  tokens >> item.location[1];
}

void Loader::handleOrientation(stringstream& tokens) {
  Item& item = getItem(tokens);
  // TODO Angle is in rats. Convert to radians or not?
  tokens >> item.orientation;
}

void Loader::handleOrientationVelocity(stringstream& tokens) {
  Item& item = getItem(tokens);
  // TODO Angular velocity is in rats. Convert to radians or not?
  tokens >> item.orientationVelocity;
}

void Loader::handleRelease(stringstream& tokens) {
  Item& tool = getItem(tokens);
  Item& item = getItem(tokens);
  tool.grasping = false;
  item.grasped = false;
}

void Loader::handleTime(stringstream& tokens) {
  string type;
  tokens >> type;
  if (type == "sim") {
    pushState();
    // TODO Some general 'reset' function?
    state.cleared = false;
    // Just eat the number of steps for now. Maybe I'll care more about it
    // later.
    int steps;
    tokens >> steps;
    // I pretend the sim time (in seconds) is what matters here.
    tokens >> state.time;
  }
}

void Loader::handleType(stringstream& tokens) {
  Item& item = getItem(tokens);
  string type;
  tokens >> type;
  // TODO Validate id.
  if (type == "block") {
    item.typeId = stackiter::Block;
  } else if (type == "tool") {
    item.typeId = Tool;
  }
}

void Loader::handleVelocity(stringstream& tokens) {
  Item& item = getItem(tokens);
  tokens >> item.velocity[0];
  tokens >> item.velocity[1];
}

void Loader::load(const string& name) {
  cout << "Loading " << name << endl;
  ifstream in(name.c_str());
  if (!in.is_open()) {
    throw "failed to open";
  }
  string command;
  string line;
  while (!getline(in, line).eof()) {
    stringstream tokens(line);
    tokens >> command;
    map<string,LoaderHandler>::iterator h = handlers.find(command);
    if (h != handlers.end()) {
      // We've defined a handler for this command, so handle it.
      LoaderHandler handler = h->second;
      (this->*handler)(tokens);
    }
  }
  // Record the end state.
  pushState();
}

void Loader::pushState() {
  // Save a copy of the current state.
  states.push_back(state);
}

*/
