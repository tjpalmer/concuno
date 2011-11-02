#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yajl/api/yajl_gen.h>

#include "choose.h"
#include "load.h"

using namespace concuno;
using namespace std;


namespace ccndomain {
namespace rcss {


bool cnrGenColumnVector(yajl_gen gen, Count count, Float* x);


bool cnrGenFloat(yajl_gen gen, Float x);


bool cnrGenStr(yajl_gen gen, const char* str);


/**
 * TODO This is duped from run's cnvPickFunctions. Perhaps centralize.
 */
bool cnrPickFunctions(List<EntityFunction*>* functions, Type* type);


bool cnrProcess(
  Game* game,
  bool (*process)(List<Bag>* holdBags, List<Bag>* passBags)
);


bool cnrProcessExport(List<Bag>* holdBags, List<Bag>* passBags);


bool cnrProcessLearn(List<Bag>* holdBags, List<Bag>* passBags);


bool cnrSaveBags(const char* name, List<Bag>* bags);


}
}


int main(int argc, char** argv) {
  using namespace ccndomain::rcss;
  Game game;
  char* name;
  int result = EXIT_FAILURE;

  try {
    // Check args.
    if (argc < 2) cnErrTo(DONE, "No file specified.");

    // Load all the states in the game log.
    // TODO Check if rcl or rcg to work either way.
    name = argv[1];
    if (!cnrLoadGameLog(&game, name)) cnErrTo(DONE, "Failed to load: %s", name);

    // Show all team names.
    for (size_t n = 0; n < game.teamNames.size(); n++) {
      cout << "Team: " << game.teamNames[n] << endl;
    }

    // Now look at the command log to see what actions were taken.
    // Assume the file is in the same place but named rcl instead of rcg.
    if (cnStrEndsWith(name, ".rcg")) {
      // Abusively just modify the arg. TODO Is this legit?
      Index lastIndex = strlen(name) - 1;
      name[lastIndex] = 'l';
      printf("Trying also to read %s\n", name);
      if (!cnrLoadCommandLog(&game, name)) {
        cnErrTo(DONE, "Failed to load: %s", name);
      }
    }

    // Report.
    printf("Loaded %ld states.\n", game.states.count);

    // Extract actions of hold or kick to player, then process them.
    if (!cnrProcess(&game, cnrProcessLearn)) cnErrTo(DONE, "Failed extract.");

    // Winned.
    result = EXIT_SUCCESS;
  } catch (const Error& error) {
    cout << error.what() << endl;
  }

  DONE:
  return result;
}


namespace ccndomain {
namespace rcss {


bool cnrGenColumnVector(yajl_gen gen, Count count, Float* x) {
  Float* end = x + count;
  if (yajl_gen_array_open(gen)) cnFailTo(FAIL);
  for (; x < end; x++) {
    if (yajl_gen_array_open(gen)) cnFailTo(FAIL);
    if (!cnrGenFloat(gen, *x)) cnFailTo(FAIL);
    if (yajl_gen_array_close(gen)) cnFailTo(FAIL);
  }
  if (yajl_gen_array_close(gen)) cnFailTo(FAIL);

  // Winned.
  return true;

  FAIL:
  return false;
}


bool cnrGenFloat(yajl_gen gen, Float x) {
  // TODO Fancierness to get nicer results.
  return yajl_gen_double(gen, x) ? false : true;
}


bool cnrGenStr(yajl_gen gen, const char* str) {
  return yajl_gen_string(gen, (unsigned char*)str, strlen(str)) ?
    false : true;
}


bool cnrPickFunctions(List<EntityFunction*>* functions, Type* type) {
  // For now, just put in valid and common functions for each property.
  if (!cnPushValidFunction(functions, type->schema, 1)) {
    cnErrTo(FAIL, "No valid 1.");
  }
  if (!cnPushValidFunction(functions, type->schema, 2)) {
    cnErrTo(FAIL, "No valid 2.");
  }
  for (size_t p = 0; p < type->properties.size(); p++) {
    Property* property = type->properties[p];
    EntityFunction* function;
    if (!(function = cnEntityFunctionCreateProperty(property))) {
      cnErrTo(FAIL, "No function.");
    }
    if (!cnListPush(functions, &function)) {
      cnEntityFunctionDrop(function);
      cnErrTo(FAIL, "Function not pushed.");
    }
    // TODO Distance (and difference?) angle, too?
    if (true || function->name == "Location") {
      EntityFunction* distance;
      if (true) {
        // Actually, skip this N^2 thing for now. For many items per bag and few
        // bags, this is both extremely slow and allows overfit, since there are
        // so many options to consider.
        //continue;
      }

      // Distance.
      if (!(distance = cnEntityFunctionCreateDistance(function))) {
        cnErrTo(FAIL, "No distance %s.", function->name.c_str());
      }
      if (!cnListPush(functions, &distance)) {
        cnEntityFunctionDrop(distance);
        cnErrTo(FAIL, "Function %s not pushed.", distance->name.c_str());
      }

      // Difference.
      if (!(distance = cnEntityFunctionCreateDifference(function))) {
        cnErrTo(FAIL, "No distance %s.", function->name.c_str());
      }
      if (!cnListPush(functions, &distance)) {
        cnEntityFunctionDrop(distance);
        cnErrTo(FAIL, "Function %s not pushed.", distance->name.c_str());
      }

      // Reframe.
      if (!(distance = cnEntityFunctionCreateReframe(function))) {
        cnErrTo(FAIL, "No reframe %s.", function->name.c_str());
      }
      if (!cnListPush(functions, &distance)) {
        cnEntityFunctionDrop(distance);
        cnErrTo(FAIL, "Function %s not pushed.", distance->name.c_str());
      }
    }
  }

  // We winned!
  return true;

  FAIL:
  // TODO Remove all added functions?
  return false;
}


bool cnrProcess(
  Game* game,
  bool (*process)(List<Bag>* holdBags, List<Bag>* passBags)
) {
  List<Bag> holdBags;
  List<List<Entity>*> entityLists;
  List<Bag> passBags;
  bool result = false;

  if (!cnrChooseHoldsAndPasses(game, &holdBags, &passBags, &entityLists)) {
    cnErrTo(DONE, "Choose failed.");
  }

  // Process the bags.
  if (!process(&holdBags, &passBags)) cnFailTo(DONE);

  // Winned.
  result = true;

  DONE:
  // Bags and entities. The entity lists get disposed with the first call.
  cnBagListDispose(&holdBags, &entityLists);
  // Clear out entities manually for safe disposal fo bag list later.
  cnListEachBegin(&passBags, Bag, bag) {
    bag->entities = NULL;
  } cnEnd;
  cnBagListDispose(&passBags, NULL);
  // Result.
  return result;
}


bool cnrProcessExport(List<Bag>* holdBags, List<Bag>* passBags) {
  // Export stuff.
  // TODO Allow specifying file names? Choose automatically by date?
  // TODO Option for learning in concuno rather than saving?
  if (!cnrSaveBags("keepaway-hold-bags.json", holdBags)) cnFailTo(FAIL);
  if (!cnrSaveBags("keepaway-pass-bags.json", passBags)) cnFailTo(FAIL);
  return true;

  FAIL:
  return false;
}


bool cnrProcessLearn(List<Bag>* holdBags, List<Bag>* passBags) {
  List<EntityFunction*> functions;
  RootNode* learnedTree = NULL;
  Learner learner;
  bool result = false;
  Schema schema;

  // Inits.
  if (!cnrSchemaInit(&schema)) cnFailTo(DONE);
  cnrPickFunctions(&functions, *(Type**)cnListGet(&schema.types, 1));

  // Learn something.
  // TODO How to choose pass vs. hold?
  cnListShuffle(holdBags);
  cnListShuffle(passBags);
  learner.bags = passBags;
  learner.entityFunctions = &functions;
  learnedTree = learner.learnTree();
  if (!learnedTree) cnErrTo(DONE, "No learned tree.");

  // Display the learned tree.
  printf("\n");
  cnTreeWrite(learnedTree, stdout);
  printf("\n");

  // Winned.
  result = true;

  DONE:
  cnNodeDrop(&learnedTree->node);
  cnListEachBegin(&functions, EntityFunction*, function) {
    cnEntityFunctionDrop(*function);
  } cnEnd;
  return result;
}


bool cnrSaveBags(const char* name, List<Bag>* bags) {
  const unsigned char* buffer;
  size_t bufferSize;
  ofstream file;
  yajl_gen gen = NULL;
  const char* itemsKey = "items";
  const char* locationKey = "location";
  bool result = false;

  printf("Writing %s\n", name);
  if (!(gen = yajl_gen_alloc(NULL))) cnErrTo(DONE, "No json formatter.");
  // TODO How to get custom info (like file names) on error message?
  file.open(name); if (!file) throw Error("Failed to open output file.");

  if (!(
    yajl_gen_config(gen, yajl_gen_beautify, true) &&
    yajl_gen_config(gen, yajl_gen_indent_string, "  ")
  )) cnFailTo(DONE);
  if (yajl_gen_array_open(gen) || yajl_gen_array_open(gen)) cnFailTo(DONE);
  cnListEachBegin(bags, Bag, bag) {
    if (yajl_gen_map_open(gen)) cnFailTo(DONE);
    if (!cnrGenStr(gen, itemsKey)) cnFailTo(DONE);
    if (yajl_gen_array_open(gen)) cnFailTo(DONE);
    // Write each item (ball or player).
    cnListEachBegin(bag->entities, Entity, entity) {
      Index depth = 0;
      Item* item = reinterpret_cast<Item*>(*entity);
      if (yajl_gen_map_open(gen)) cnFailTo(DONE);
      // Type as ball, left (team), or right (team).
      if (!cnrGenStr(gen, "type")) cnFailTo(DONE);
      if (item->type == Item::TypeBall) {
        if (!cnrGenStr(gen, "ball")) cnFailTo(DONE);
      } else {
        Player* player = (Player*)item;
        if (!cnrGenStr(gen, player->team ? "right" : "left")) cnFailTo(DONE);
      }
      // Location.
      if (!cnrGenStr(gen, locationKey)) cnFailTo(DONE);
      if (!cnrGenColumnVector(gen, 2, item->location)) cnFailTo(DONE);
      // Pinning.
      cnListEachBegin(&bag->participantOptions, List<Entity>, options) {
        if (options->count > 1) {
          cnErrTo(
            DONE, "%ld > 1 options at depth %ld.", options->count, depth
          );
        }
        // Skip out if we found the player pinned.
        if (item == *reinterpret_cast<Item**>(options->items)) break;
        depth++;
      } cnEnd;
      if (!cnrGenStr(gen, "__instantiable_depth__")) cnFailTo(DONE);
      if (yajl_gen_integer(gen, depth)) cnFailTo(DONE);
      // SMRF Python class.
      if (!cnrGenStr(gen, "__json_class__")) cnFailTo(DONE);
      if (!cnrGenStr(gen, "Item")) cnFailTo(DONE);
      // End item.
      if (yajl_gen_map_close(gen)) cnFailTo(DONE);
    } cnEnd;
    if (yajl_gen_array_close(gen)) cnFailTo(DONE);
    // Bag label.
    if (!cnrGenStr(gen, "label")) cnFailTo(DONE);
    if (yajl_gen_bool(gen, bag->label)) cnFailTo(DONE);
    // SMRF Python class.
    if (!cnrGenStr(gen, "__json_class__")) cnFailTo(DONE);
    if (!cnrGenStr(gen, "Graph")) cnFailTo(DONE);
    // End bag.
    if (yajl_gen_map_close(gen)) cnFailTo(DONE);
  } cnEnd;
  if (yajl_gen_array_close(gen) || yajl_gen_array_open(gen)) cnFailTo(DONE);
  if (!cnrGenStr(gen, locationKey)) cnFailTo(DONE);
  if (yajl_gen_array_close(gen) || yajl_gen_array_close(gen)) cnFailTo(DONE);

  // Output.
  // TODO Alternatively set the gen print function to print directly!
  if (yajl_gen_get_buf(gen, &buffer, &bufferSize)) cnFailTo(DONE);
  if (!file.write(
    reinterpret_cast<const char*>(buffer),
    static_cast<streamsize>(bufferSize)
  )) throw Error("Failed to write to file.");

  // Only bother with manual close if we didn't have other errors.
  file.close();
  if (!file) throw Error("Failed to close file.");

  // Winned.
  result = true;

  DONE:
  if (gen) yajl_gen_free(gen);
  return result;
}


}
}
