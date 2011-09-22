#include "domain.h"


void cnrItemInit(cnrItem item, cnrType type) {
  item->location[0] = 0.0;
  item->location[1] = 0.0;
  item->type = type;
}


void cnrPlayerInit(cnrPlayer player) {
  // Generic item stuff.
  cnrItemInit(&player->item, cnrTypePlayer);
  // Actual players start from 1, so 0 is a good invalid value.
  player->index = 0;
  // On the other hand, I expect 0 and 1 both for teams, so use -1 for bogus.
  player->team = -1;
}


void cnrStateInit(cnrState state) {
  cnrItemInit(&state.ball.item, cnrTypeBall);
  cnListInit(state->players, sizeof(struct cnrPlayer));
  // Seems first times are 0 1, so 0 0 should be an okay bogus.
  state->subtime = 0;
  state->time = 0;
}
