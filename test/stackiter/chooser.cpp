#include "chooser.h"

using namespace std;

namespace stackiter {

void Chooser::chooseDropWhereLandOnOtherTrue(
  const vector<State>& states,
  vector<const State*>& pos,
  vector<const State*>& neg
) {
  bool formerHadGrasp(false);
  int graspedId(-1);
  const State* ungraspState(0);
  vector<const Item*> graspedItems;
  for (
    vector<State>::const_iterator s(states.begin());
    s != states.end();
    s++
  ) {
    const State& state(*s);
    if (ungraspState) {
      // TODO Look for stable state.
      // TODO Only unset state once that's found.
      ungraspState = 0;
    } else {
      bool hasGrasp(findGraspedItems(state, &graspedItems));
      if (hasGrasp) {
        // TODO Deal with sets of grasped items? This would easily fail if more
        // TODO than one.
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
    vector<Item>::const_iterator i = state.items.begin();
    i != state.items.end();
    i++
  ) {
    const Item& item = *i;
    if (item.grasped) {
      anyGrasped = true;
      if (items) {
        items->push_back(&item);
      }
    }
  }
  return anyGrasped;
}

}
