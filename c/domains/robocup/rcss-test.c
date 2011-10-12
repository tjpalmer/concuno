#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yajl/api/yajl_gen.h>

#include "choose.h"
#include "load.h"


cnBool cnrGenColumnVector(yajl_gen gen, cnCount count, cnFloat* x);


cnBool cnrGenFloat(yajl_gen gen, cnFloat x);


cnBool cnrGenStr(yajl_gen gen, char* str);


/**
 * TODO This is duped from run's cnvPickFunctions. Perhaps centralize.
 */
cnBool cnrPickFunctions(cnList(cnEntityFunction*)* functions, cnType* type);


cnBool cnrProcess(
  cnrGame game,
  cnBool (*process)(cnList(cnBag)* holdBags, cnList(cnBag)* passBags)
);


cnBool cnrProcessExport(cnList(cnBag)* holdBags, cnList(cnBag)* passBags);


cnBool cnrProcessLearn(cnList(cnBag)* holdBags, cnList(cnBag)* passBags);


cnBool cnrSaveBags(char* name, cnList(cnBag)* bags);


int main(int argc, char** argv) {
  struct cnrGame game;
  char* name;
  int result = EXIT_FAILURE;

  // Init first.
  cnrGameInit(&game);

  // Check args.
  if (argc < 2) cnFailTo(DONE, "No file specified.");

  // Load all the states in the game log.
  // TODO Check if rcl or rcg to work either way.
  name = argv[1];
  if (!cnrLoadGameLog(&game, name)) cnFailTo(DONE, "Failed to load: %s", name);

  // Show all team names.
  cnListEachBegin(&game.teamNames, cnString, name) {
    printf("Team: %s\n", cnStr(name));
  } cnEnd;

  // Now look at the command log to see what actions were taken.
  // Assume the file is in the same place but named rcl instead of rcg.
  if (cnStrEndsWith(name, ".rcg")) {
    // Abusively just modify the arg. TODO Is this legit?
    cnIndex lastIndex = strlen(name) - 1;
    name[lastIndex] = 'l';
    printf("Trying also to read %s\n", name);
    if (!cnrLoadCommandLog(&game, name)) {
      cnFailTo(DONE, "Failed to load: %s", name);
    }
  }

  // Report.
  printf("Loaded %ld states.\n", game.states.count);

  // Extract actions of hold or kick to player, then process them.
  if (!cnrProcess(&game, cnrProcessLearn)) cnFailTo(DONE, "Failed extract.");

  // Winned.
  result = EXIT_SUCCESS;

  DONE:
  cnrGameDispose(&game);
  return result;
}


cnBool cnrGenColumnVector(yajl_gen gen, cnCount count, cnFloat* x) {
  cnFloat* end = x + count;
  if (yajl_gen_array_open(gen)) cnErrTo(FAIL);
  for (; x < end; x++) {
    if (yajl_gen_array_open(gen)) cnErrTo(FAIL);
    if (!cnrGenFloat(gen, *x)) cnErrTo(FAIL);
    if (yajl_gen_array_close(gen)) cnErrTo(FAIL);
  }
  if (yajl_gen_array_close(gen)) cnErrTo(FAIL);

  // Winned.
  return cnTrue;

  FAIL:
  return cnFalse;
}


cnBool cnrGenFloat(yajl_gen gen, cnFloat x) {
  // TODO Fancierness to get nicer results.
  return yajl_gen_double(gen, x) ? cnFalse : cnTrue;
}


cnBool cnrGenStr(yajl_gen gen, char* str) {
  return yajl_gen_string(gen, (unsigned char*)str, strlen(str)) ?
    cnFalse : cnTrue;
}


cnBool cnrPickFunctions(cnList(cnEntityFunction*)* functions, cnType* type) {
  // For now, just put in valid and common functions for each property.
  if (!cnPushValidFunction(functions, type->schema, 1)) {
    cnFailTo(FAIL, "No valid 1.");
  }
  if (!cnPushValidFunction(functions, type->schema, 2)) {
    cnFailTo(FAIL, "No valid 2.");
  }
  cnListEachBegin(&type->properties, cnProperty, property) {
    cnEntityFunction* function;
    if (!(function = cnEntityFunctionCreateProperty(property))) {
      cnFailTo(FAIL, "No function.");
    }
    if (!cnListPush(functions, &function)) {
      cnEntityFunctionDrop(function);
      cnFailTo(FAIL, "Function not pushed.");
    }
    // TODO Distance (and difference?) angle, too?
    if (cnTrue || !strcmp("Location", cnStr(&function->name))) {
      cnEntityFunction* distance;
      if (cnTrue) {
        // Actually, skip this N^2 thing for now. For many items per bag and few
        // bags, this is both extremely slow and allows overfit, since there are
        // so many options to consider.
        //continue;
      }

      // Distance.
      if (!(distance = cnEntityFunctionCreateDistance(function))) {
        cnFailTo(FAIL, "No distance %s.", cnStr(&function->name));
      }
      if (!cnListPush(functions, &distance)) {
        cnEntityFunctionDrop(distance);
        cnFailTo(FAIL, "Function %s not pushed.", cnStr(&distance->name));
      }

      // Difference.
      if (!(distance = cnEntityFunctionCreateDifference(function))) {
        cnFailTo(FAIL, "No distance %s.", cnStr(&function->name));
      }
      if (!cnListPush(functions, &distance)) {
        cnEntityFunctionDrop(distance);
        cnFailTo(FAIL, "Function %s not pushed.", cnStr(&distance->name));
      }

      // Reframe.
      if (!(distance = cnEntityFunctionCreateReframe(function))) {
        cnFailTo(FAIL, "No reframe %s.", cnStr(&function->name));
      }
      if (!cnListPush(functions, &distance)) {
        cnEntityFunctionDrop(distance);
        cnFailTo(FAIL, "Function %s not pushed.", cnStr(&distance->name));
      }
    }
  } cnEnd;

  // We winned!
  return cnTrue;

  FAIL:
  // TODO Remove all added functions?
  return cnFalse;
}


cnBool cnrProcess(
  cnrGame game,
  cnBool (*process)(cnList(cnBag)* holdBags, cnList(cnBag)* passBags)
) {
  cnList(cnBag) holdBags;
  cnList(cnList(cnEntity)*) entityLists;
  cnList(cnBag) passBags;
  cnBool result = cnFalse;

  // Init for safety.
  cnListInit(&holdBags, sizeof(cnBag));
  cnListInit(&passBags, sizeof(cnBag));
  cnListInit(&entityLists, sizeof(cnList(cnEntity)*));

  if (!cnrChooseHoldsAndPasses(game, &holdBags, &passBags, &entityLists)) {
    cnFailTo(DONE, "Choose failed.");
  }

  // Process the bags.
  if (!process(&holdBags, &passBags)) cnErrTo(DONE);

  // Winned.
  result = cnTrue;

  DONE:
  // Bags and entities. The entity lists get disposed with the first call,
  // leaving bogus pointers inside the passBags, but they'll be unused.
  cnBagListDispose(&holdBags, &entityLists);
  cnBagListDispose(&passBags, &entityLists);
  // Result.
  return result;
}


cnBool cnrProcessExport(cnList(cnBag)* holdBags, cnList(cnBag)* passBags) {
  // Export stuff.
  // TODO Allow specifying file names? Choose automatically by date?
  // TODO Option for learning in concuno rather than saving?
  if (!cnrSaveBags("keepaway-hold-bags.json", holdBags)) cnErrTo(FAIL);
  if (!cnrSaveBags("keepaway-pass-bags.json", passBags)) cnErrTo(FAIL);
  return cnTrue;

  FAIL:
  return cnFalse;
}


cnBool cnrProcessLearn(cnList(cnBag)* holdBags, cnList(cnBag)* passBags) {
  cnList(cnEntityFunction*) functions;
  cnBool initOkay = cnTrue;
  cnRootNode* learnedTree = NULL;
  cnLearner learner;
  cnBool result = cnFalse;
  cnSchema schema;

  // Inits.
  cnListInit(&functions, sizeof(cnEntityFunction*));
  initOkay &= cnrSchemaInit(&schema);
  initOkay &= cnLearnerInit(&learner, NULL);
  if (!initOkay) cnErrTo(DONE);
  cnrPickFunctions(&functions, *(cnType**)cnListGet(&schema.types, 1));

  // Learn something.
  // TODO How to choose pass vs. hold?
  cnListShuffle(passBags);
  learner.bags = passBags;
  learner.entityFunctions = &functions;
  learnedTree = cnLearnTree(&learner);
  if (!learnedTree) cnFailTo(DONE, "No learned tree.");

  // Display the learned tree.
  printf("\n");
  cnTreeWrite(learnedTree, stdout);
  printf("\n");

  // Winned.
  result = cnTrue;

  DONE:
  cnNodeDrop(&learnedTree->node);
  cnLearnerDispose(&learner);
  cnSchemaDispose(&schema);
  cnListEachBegin(&functions, cnEntityFunction*, function) {
    cnEntityFunctionDrop(*function);
  } cnEnd;
  cnListDispose(&functions);
  return result;
}


cnBool cnrSaveBags(char* name, cnList(cnBag)* bags) {
  const unsigned char* buffer;
  size_t bufferSize;
  FILE* file = NULL;
  yajl_gen gen = NULL;
  char* itemsKey = "items";
  char* locationKey = "location";
  cnBool result = cnFalse;

  printf("Writing %s\n", name);
  if (!(gen = yajl_gen_alloc(NULL))) cnFailTo(DONE, "No json formatter.");
  if (!(file = fopen(name, "w"))) cnFailTo(DONE, "Failed to open: %s", name);

  if (!(
    yajl_gen_config(gen, yajl_gen_beautify, cnTrue) &&
    yajl_gen_config(gen, yajl_gen_indent_string, "  ")
  )) cnErrTo(DONE);
  if (yajl_gen_array_open(gen) || yajl_gen_array_open(gen)) cnErrTo(DONE);
  cnListEachBegin(bags, cnBag, bag) {
    if (yajl_gen_map_open(gen)) cnErrTo(DONE);
    if (!cnrGenStr(gen, itemsKey)) cnErrTo(DONE);
    if (yajl_gen_array_open(gen)) cnErrTo(DONE);
    // Write each item (ball or player).
    cnListEachBegin(bag->entities, cnEntity, entity) {
      cnIndex depth = 0;
      cnrItem item = *entity;
      if (yajl_gen_map_open(gen)) cnErrTo(DONE);
      // Type as ball, left (team), or right (team).
      if (!cnrGenStr(gen, "type")) cnErrTo(DONE);
      if (item->type == cnrTypeBall) {
        if (!cnrGenStr(gen, "ball")) cnErrTo(DONE);
      } else {
        cnrPlayer player = (cnrPlayer)item;
        if (!cnrGenStr(gen, player->team ? "right" : "left")) cnErrTo(DONE);
      }
      // Location.
      if (!cnrGenStr(gen, locationKey)) cnErrTo(DONE);
      if (!cnrGenColumnVector(gen, 2, item->location)) cnErrTo(DONE);
      // Pinning.
      cnListEachBegin(&bag->participantOptions, cnList(cnEntity), options) {
        if (options->count > 1) {
          cnFailTo(
            DONE, "%ld > 1 options at depth %ld.", options->count, depth
          );
        }
        // Skip out if we found the player pinned.
        if (item == *(cnrItem*)options->items) break;
        depth++;
      } cnEnd;
      if (!cnrGenStr(gen, "__instantiable_depth__")) cnErrTo(DONE);
      if (yajl_gen_integer(gen, depth)) cnErrTo(DONE);
      // End item.
      if (yajl_gen_map_close(gen)) cnErrTo(DONE);
    } cnEnd;
    if (yajl_gen_array_close(gen)) cnErrTo(DONE);
    // Bag label.
    if (!cnrGenStr(gen, "__label__")) cnErrTo(DONE);
    if (yajl_gen_bool(gen, bag->label)) cnErrTo(DONE);
    if (yajl_gen_map_close(gen)) cnErrTo(DONE);
  } cnEnd;
  if (yajl_gen_array_close(gen) || yajl_gen_array_open(gen)) cnErrTo(DONE);
  if (!cnrGenStr(gen, locationKey)) cnErrTo(DONE);
  if (yajl_gen_array_close(gen) || yajl_gen_array_close(gen)) cnErrTo(DONE);

  // Output.
  // TODO Alternatively set the gen print function to print directly!
  if (yajl_gen_get_buf(gen, &buffer, &bufferSize)) cnErrTo(DONE);
  if (fwrite(buffer, sizeof(char), bufferSize, file) < bufferSize) {
    cnErrTo(DONE);
  }

  // Winned.
  result = cnTrue;

  DONE:
  if (file) fclose(file);
  if (gen) yajl_gen_free(gen);
  return result;
}
