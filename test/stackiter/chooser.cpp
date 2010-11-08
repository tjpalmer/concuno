#include "chooser.h"

using namespace std;

namespace stackiter {

void Chooser::chooseDropWhereLandOnOtherTrue(
  const vector<State>& states, vector<State*>& pos, vector<State*>& neg
) {
  bool formerHadGrasp = false;
  for (
    vector<State>::const_iterator s = states.begin();
    s != states.end();
    s++
  ) {
    const State& state = *s;
    bool hasGrasp = findGraspedItems(state);
    //if (formerHadGrasp && )
  }
}

bool Chooser::findGraspedItems(const State& state, vector<Item*>* items) {
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
