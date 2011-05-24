#include <cuncuno.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "loader.h"


/**
 * Parses a single line, returning true for no error.
 */
cnBool stParseLine(cnString* line, stState* state, cnList* states);


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


/**
 * Individual parse handlers for specific commands.
 */
cnBool stParseAlive(char* args, stState* state, cnList* states);
cnBool stParseClear(char* args, stState* state, cnList* states);
cnBool stParseColor(char* args, stState* state, cnList* states);
cnBool stParseDestroy(char* args, stState* state, cnList* states);
cnBool stParseExtent(char* args, stState* state, cnList* states);
cnBool stParseGrasp(char* args, stState* state, cnList* states);
cnBool stParseItem(char* args, stState* state, cnList* states);
cnBool stParsePos(char* args, stState* state, cnList* states);
cnBool stParsePosVel(char* args, stState* state, cnList* states);
cnBool stParseRelease(char* args, stState* state, cnList* states);
cnBool stParseRot(char* args, stState* state, cnList* states);
cnBool stParseRotVel(char* args, stState* state, cnList* states);
cnBool stParseTime(char* args, stState* state, cnList* states);
cnBool stParseType(char* args, stState* state, cnList* states);


cnBool stLoad(char* name, cnList* states) {
  cnBool result = cnTrue;
  int closeError;
  cnString line;
  cnCount lineCount, readCount;
  stState state;
  stStateInit(&state);
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
    if (!stParseLine(&line, &state, states)) {
      // TODO Distinguish parse errors from memory allocation fails.
      printf(
          "Error parsing line %d of %s: %s\n", lineCount, name, cnStr(&line)
      );
      result = cnFalse;
      break;
    }
  }
  printf("Read lines: %d\n", lineCount);
  cnStringDispose(&line);
  stStateDispose(&state);
  closeError = fclose(file);
  if (readCount < 0 || closeError) {
    printf("Error reading or closing: %s\n", name);
    result = cnFalse;
  }
  return result;
}


cnBool stParseLine(cnString* line, stState* state, cnList* states) {
  // TODO Extract command then scanf it?
  char *args, *command;
  cnBool (*parse)(char* args, stState* state, cnList* states) = NULL;
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
    return parse(args, state, states);
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


cnBool stParseAlive(char* args, stState* state, cnList* states) {
  cnIndex id = strtol(args, &args, 10);
  char* status = stParseString(args, &args);
  // TODO Find item at id.
  // item->alive = !strcmp(status, "true");
  return cnTrue;
}


cnBool stParseClear(char* args, stState* state, cnList* states) {
  return cnTrue;
}


cnBool stParseColor(char* args, stState* state, cnList* states) {
  return cnTrue;
}


cnBool stParseDestroy(char* args, stState* state, cnList* states) {
  return cnTrue;
}


cnBool stParseExtent(char* args, stState* state, cnList* states) {
  return cnTrue;
}


cnBool stParseGrasp(char* args, stState* state, cnList* states) {
  return cnTrue;
}


cnBool stParseItem(char* args, stState* state, cnList* states) {
  return cnTrue;
}


cnBool stParsePos(char* args, stState* state, cnList* states) {
  return cnTrue;
}


cnBool stParsePosVel(char* args, stState* state, cnList* states) {
  return cnTrue;
}


cnBool stParseRelease(char* args, stState* state, cnList* states) {
  return cnTrue;
}


cnBool stParseRot(char* args, stState* state, cnList* states) {
  return cnTrue;
}


cnBool stParseRotVel(char* args, stState* state, cnList* states) {
  return cnTrue;
}


cnBool stParseTime(char* args, stState* state, cnList* states) {
  return cnTrue;
}


cnBool stParseType(char* args, stState* state, cnList* states) {
  return cnTrue;
}


/*


Item& Loader::getItem(stringstream& tokens) {
  // TODO Any validation?
  return state.items[indexes[handleId(tokens)]];
}

void Loader::handleAlive(stringstream& tokens) {
  Item& item = getItem(tokens);
  // TODO Does this really work from strings?
  string alive;
  tokens >> alive;
  item.alive = alive == "true";
}

void Loader::handleClear(stringstream& tokens) {
  state.cleared = true;
}

void Loader::handleColor(stringstream& tokens) {
  Item& item = getItem(tokens);
  // TODO Use HSV colorspace to begin with?
  // TODO Verify we haven't run out?
  tokens >> item.color[0];
  tokens >> item.color[1];
  tokens >> item.color[2];
  // Ignore opacity, the 4th value. It's bogus for now.
}

void Loader::handleDestroy(stringstream& tokens) {
  int id = handleId(tokens);
  int index = indexes[id];
  if (!index) {
    throw "double destroy";
  }
  // Remove the destroyed item.
  indexes[id] = 0;
  state.items.erase(state.items.begin() + index);
  // Reduce the index of successive items.
  for (
    vector<Item>::iterator i = state.items.begin() + index;
    i < state.items.end();
    i++
  ) {
    // TODO Could optimize further if we assume seeing items always in
    // TODO increasing order.
    const Item& item = *i;
    indexes[item.id]--;
  }
}

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
