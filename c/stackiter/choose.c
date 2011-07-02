#include <math.h>

#include "choose.h"


/**
 * Finds items grasped in the state, and returns whether any were found.
 */
cnBool stFindGraspedItems(const stState* state, cnList(stItem*)* items);


cnBool stOnGround(const stItem* item);


cnFloat stNorm(const cnFloat* values, cnCount count);


/**
 * Place pointers to alive items into the entities vector.
 */
cnBool stPlaceLiveItems(
  const cnList(stItem)* items, cnList(stItem*)* entities
);


cnBool stAllBagsFalse(cnList(stState)* states, cnList(cnBag)* bags) {
  cnBool result = cnTrue;
  cnListEachBegin(states, stState, state) {
    // Every state gets a bag.
    cnBag* bag;
    if (!(bag = cnListExpand(bags))) {
      printf("Failed to push bag.\n");
      result = cnFalse;
      break;
    }
    cnBagInit(bag);
    // Each bag gets the live items.
    if (!stPlaceLiveItems(&state->items, &bag->entities)) {
      printf("Failed to push entities.\n");
      result = cnFalse;
      break;
    }
  } cnEnd;
  // All done.
  return result;
}


cnBool stChooseDropWhereLandOnOther(
  const cnList(stState)* states, cnList(cnBag)* bags
) {
  cnBool result = cnTrue;
  cnBool formerHadGrasp = cnFalse;
  stId graspedId = -1;
  const stState* ungraspState = NULL;
  cnList(stItem*) graspedItems;
  cnListInit(&graspedItems, sizeof(stItem*));
  cnListEachBegin(states, stState, state) {
    if (ungraspState) {
      // Look for stable state.
      cnBool label = -1;
      cnBool settled = cnFalse;
      if (state->cleared) {
        // World cleared. Say it's settled, but don't assign a label.
        settled = cnTrue;
      } else {
        const stItem* item = stStateFindItem(state, graspedId);
        if (item) {
          // Still here. See if it's moving.
          if (stNorm(item->velocity, 2) < 0.01) {
            // Stopped. See where it is.
            // TODO If the block bounced up, we might be catching it at the
            // TODO peak. Maybe we should check for that (by accel or
            // TODO location?).
            settled = cnTrue;
            label = !stOnGround(item);
          }
        } else {
          // It fell off the table, so it didn't land on another block.
          settled = cnTrue;
          label = cnFalse;
        }
      }
      if (settled) {
        if (label == cnFalse || label == cnTrue) {
          cnBag* bag;
          if (!(bag = cnListExpand(bags))) {
            printf("Failed to push bag.\n");
            result = cnFalse;
            break;
          }
          // Now init the bag in the list.
          cnBagInit(bag);
          bag->label = label;
          // If we defer placing entity pointers until after we've stored the
          // bag itself, then cleanup from failure is easier.
          if (!stPlaceLiveItems(&ungraspState->items, &bag->entities)) {
            printf("Failed to push entities.\n");
            result = cnFalse;
            break;
          }
        }
        ungraspState = NULL;
      }
    } else {
      cnBool hasGrasp;
      if (!stFindGraspedItems(state, &graspedItems)) {
        printf("Failed to push grasped items.\n");
        result = cnFalse;
        break;
      }
      hasGrasp = graspedItems.count;
      if (hasGrasp) {
        // TODO Deal with sets of grasped items? This would easily fail if
        // TODO more than one.
        // TODO What if more than one ungrasp occurs at the same state and
        // TODO each has a different result?
        graspedId = (((stItem**)graspedItems.items)[0])->id;
        cnListClear(&graspedItems);
      } else if (formerHadGrasp && !state->cleared) {
        ungraspState = state;
      }
      formerHadGrasp = hasGrasp;
    }
  } cnEnd;
  cnListDispose(&graspedItems);
  return result;
}


cnBool stFindGraspedItems(const stState* state, cnList(stItem*)* items) {
  cnListEachBegin(&state->items, stItem, item) {
    if (item->grasped) {
      if (items) {
        if (!cnListPush(items, &item)) {
          return cnFalse;
        }
      }
    }
  } cnEnd;
  return cnTrue;
}


cnFloat stNorm(const cnFloat* values, cnCount count) {
  cnCount v;
  cnFloat total = 0;
  for (v = 0; v < count; v++) {
    cnFloat value = values[v];
    total += value * value;
  }
  return sqrt(total);
}


cnBool stOnGround(const stItem* item) {
  cnFloat angle = item->orientation;
  // Angles go from -1 to 1.
  // Here, dim 1 is upright, and 0 sideways.
  int dim = angle < -0.75 || (-0.25 < angle && angle < 0.25) || 0.75 < angle;
  return fabs(item->extent[dim] - item->location[1]) < 0.01;
}


cnBool stPlaceLiveItems(
  const cnList(stItem)* items, cnList(stItem*)* entities
) {
  cnListEachBegin(items, stItem, item) {
    if (item->alive) {
      // Store the address of the item, not a copy.
      // And get a void pointer to it for consistency.
      void* entity = item;
      cnListPush(entities, &entity);
    }
  } cnEnd;
  return cnTrue;
}
