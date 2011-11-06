#include <math.h>

#include "choose.h"

using namespace concuno;


namespace ccndomain {namespace stackiter {


/**
 * Finds items grasped in the state, and returns whether any were found.
 */
bool stFindGraspedItems(const State* state, List<Item*>* items);


bool stOnGround(const Item* item);


/**
 * Place pointers to alive items into the entities vector.
 */
bool stPlaceLiveItems(
  const List<Item>* items, List<Entity>* entities
);


bool allBagsFalse(
  List<State>* states, List<Bag>* bags,
  List<List<Entity>*>* entityLists
) {
  bool result = false;

  cnListEachBegin(states, State, state) {
    // Every state gets a bag.
    Bag* bag;
    if (!(bag = reinterpret_cast<Bag*>(cnListExpand(bags)))) {
      cnErrTo(DONE, "Failed to push bag.");
    }
    new(bag) Bag;
    // Each bag gets the live items.
    if (!stPlaceLiveItems(&state->items, bag->entities)) {
      cnErrTo(DONE, "Failed to push entities.");
    }
  } cnEnd;

  // Winned.
  result = true;

  DONE:
  return result;
}


bool chooseDropWhereLandOnOther(
  List<State>* states, List<Bag>* bags,
  List<List<Entity>*>* entityLists
) {
  bool result = false;
  bool formerHadGrasp = false;
  Id graspedId = -1;
  const State* ungraspState = NULL;
  List<Item*> graspedItems;
  cnListEachBegin(states, State, state) {
    if (ungraspState) {
      // Look for stable state.
      int label = -1;
      bool settled = false;
      if (state->cleared) {
        // World cleared. Say it's settled, but don't assign a label.
        settled = true;
      } else {
        Item* item = stateFindItem(state, graspedId);
        if (item) {
          // Still here. See if it's moving.
          if (cnNorm(2, item->velocity) < 0.01) {
            // Stopped. See where it is.
            // TODO If the block bounced up, we might be catching it at the
            // TODO peak. Maybe we should check for that (by accel or
            // TODO location?).
            settled = true;
            label = !stOnGround(item);
          }
        } else {
          // It fell off the table, so it didn't land on another block.
          settled = true;
          label = true;
        }
      }
      if (settled) {
        if (label == false || label == true) {
          Bag* bag;
          if (!(bag = reinterpret_cast<Bag*>(cnListExpand(bags)))) {
            cnErrTo(DONE, "Failed to push bag.");
          }
          // Now init the bag in the list.
          new(bag) Bag;
          bag->label = label;
          // If we defer placing entity pointers until after we've stored the
          // bag itself, then cleanup from failure is easier.
          if (!stPlaceLiveItems(&ungraspState->items, bag->entities)) {
            cnErrTo(DONE, "Failed to push entities.");
          }
        }
        ungraspState = NULL;
      }
    } else {
      if (!stFindGraspedItems(state, &graspedItems)) {
        cnErrTo(DONE, "Failed to push grasped items.");
      }
      bool hasGrasp = graspedItems.count;
      if (hasGrasp) {
        // TODO Deal with sets of grasped items? This would easily fail if
        // TODO more than one.
        // TODO What if more than one ungrasp occurs at the same state and
        // TODO each has a different result?
        graspedId = (((Item**)graspedItems.items)[0])->id;
        cnListClear(&graspedItems);
      } else if (formerHadGrasp && !state->cleared) {
        ungraspState = state;
      }
      formerHadGrasp = hasGrasp;
    }
  } cnEnd;

  // Winned!
  result = true;

  DONE:
  return result;
}


bool chooseWhereNotMoving(
  List<State>* states, List<Bag>* bags,
  List<List<Entity>*>* entityLists
) {
  Float epsilon = 1e-2;
  bool result = false;

  // Find bags.
  cnListEachBegin(states, State, state) {
    // Every state gets a bag.
    List<Entity>* entities = NULL;
    bool keep = true;
    // Assume bags have none moving by default.

    // Label based on whether any are moving.
    cnListEachBegin(&state->items, Item, item) {
      if (item->grasped) {
        // Ignore cases where items are grasped until we have support for
        // discrete topologies.
        // TODO Get back to this!
        keep = false;
        break;
      }
    } cnEnd;
    if (!keep) {
      // We don't want this state at all.
      continue;
    }

    // We want it.
    // First make an entity list usable for multiple bags.
    entities = new List<Entity>;

    // Push it on the list.
    if (!cnListPush(entityLists, &entities)) {
      // If it didn't get on the list, it won't be freed later. Free it now.
      delete entities;
      entities->dispose();
      free(entities);
      cnErrTo(DONE, "Failed to push entities list.");
    }

    // Each bag gets the live items.
    if (!stPlaceLiveItems(&state->items, entities)) {
      cnErrTo(DONE, "Failed to push entities.");
    }

    // Create a bag for each item, where we constrain the first binding and
    // label based on whether the item is moving.
    cnListEachBegin(entities, Entity, entity) {
      Bag* bag;
      List<Entity>* participant;
      Item* item = *(Item**)entity;
      Float speed;

      // Bag.
      if (!(bag = reinterpret_cast<Bag*>(cnListExpand(bags)))) {
        cnErrTo(DONE, "Failed to push bag.");
      }
      // With provided entities, bag init doesn't fail.
      new(bag) Bag(entities);

      // Participant.
      if (!(
        participant = reinterpret_cast<List<Entity>*>(
          cnListExpand(&bag->participantOptions))
      )) {
        // Hide the bag and fail.
        bags->count--;
        cnErrTo(DONE, "Failed to push participant list.");
      }
      participant->init();
      // Push the (pointer to the) item, after earlier safety init.
      if (!cnListPush(participant, entity)) cnErrTo(DONE, "No participant.");

      // See if this item is moving or not.
      // TODO Check orientation velocity, too?
      speed = sqrt(
        item->velocity[0] * item->velocity[0] +
        item->velocity[1] * item->velocity[1]
      );
      // True here is stationary.
      bag->label = speed < epsilon;
    } cnEnd;
  } cnEnd;

  // Winned!
  result = true;

  DONE:
  return result;
}


bool stFindGraspedItems(const State* state, List<Item*>* items) {
  cnListEachBegin(&state->items, Item, item) {
    if (item->grasped) {
      if (items) {
        if (!cnListPush(items, &item)) {
          return false;
        }
      }
    }
  } cnEnd;
  return true;
}


bool stOnGround(const Item* item) {
  Float angle = item->orientation;
  // Angles go from -1 to 1.
  // Here, dim 1 is upright, and 0 sideways.
  int dim = angle < -0.75 || (-0.25 < angle && angle < 0.25) || 0.75 < angle;
  return fabs(item->extent[dim] - item->location[1]) < 0.01;
}


bool stPlaceLiveItems(
  const List<Item>* items, List<Entity>* entities
) {
  cnListEachBegin(items, Item, item) {
    // Make sure it's alive and not the ground.
    if (item->alive && item->location[1] >= 0) {
      // Store the address of the item, not a copy.
      // And get a void pointer to it for consistency.
      void* entity = item;
      cnListPush(entities, &entity);
    }
  } cnEnd;
  return true;
}


}}
