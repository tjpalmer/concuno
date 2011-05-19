#include "chooser.h"
#include <cmath>
#include <iostream>

using namespace cuncuno;
using namespace std;

namespace stackiter {

void Chooser::chooseDropWhereLandOnOtherTrue(
  const vector<State>& states, vector<Sample>& samples
) {
  bool formerHadGrasp(false);
  int graspedId(-1);
  const State* ungraspState(0);
  vector<const Item*> graspedItems;
  for (
    vector<State>::const_iterator s(states.begin()); s != states.end(); s++
  ) {
    const State& state(*s);
    if (ungraspState) {
      if (state.cleared) {
        // The world was cleared before the block settled. Just move on with
        // life.
        continue;
      }
      // Look for stable state.
      bool done(false);
      bool label;
      const Item* item(state.findItem(graspedId));
      if (!item) {
        // It fell away. This is a negative state.
        done = true;
        label = false;
      } else if (norm(item->velocity, 2) < 0.01) {
        // Settled down. See if it is above the ground.
        done = true;
        label = !onGround(*item);
      }
      if (done) {
        ungraspState = 0;
        // TODO How to allocate in place in the vector?
        samples.push_back(Sample());
        Sample& sample(samples.back());
        placeLiveItems(state.items, sample.entities);
        sample.label = label;
      }
    } else {
      bool hasGrasp(findGraspedItems(state, &graspedItems));
      if (hasGrasp) {
        // TODO Deal with sets of grasped items? This would easily fail if
        // TODO more than one.
        // TODO What if more than one ungrasp occurs at the same state and
        // TODO each has a different result?
        graspedId = graspedItems[0]->id;
        graspedItems.clear();
      } else if (formerHadGrasp) {
        ungraspState = &state;
      }
      formerHadGrasp = hasGrasp;
    }
  }
}

bool Chooser::findGraspedItems(
  const State& state, vector<const Item*>* items
) {
  bool anyGrasped = false;
  for (
    vector<Item>::const_iterator i(state.items.begin());
    i != state.items.end();
    i++
  ) {
    const Item& item(*i);
    if (item.grasped) {
      anyGrasped = true;
      if (items) {
        items->push_back(&item);
      }
    }
  }
  return anyGrasped;
}

double norm(const double* values, size_t count) {
  double total(0);
  for (size_t v(0); v < count; v++) {
    double value(values[v]);
    total += value * value;
  }
  return sqrt(total);
}

bool onGround(const Item& item) {
  double angle(item.orientation);
  int dim;
  // Angles go from -1 to 1.
  if ((-0.25 < angle && angle < 0.25) || (angle < -0.75 || angle > 0.75)) {
    // Upright.
    dim = 1;
  } else {
    // Sideways.
    dim = 0;
  }
  return abs(item.extent[dim] - item.location[1]) < 0.01;
}

void placeLiveItems(
  const vector<Item>& items, vector<const void*>& entities
) {
  for (vector<Item>::const_iterator i = items.begin(); i != items.end(); i++) {
    const Item& item = *i;
    if (item.alive) {
      entities.push_back(&item);
    }
  }
}

}
