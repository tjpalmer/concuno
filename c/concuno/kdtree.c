#include "kdtree.h"


cnRootNode* cnKdSplit(cnKdSplitter splitter) {
  return NULL;
}


cnKdSplitter cnKdSplitterCreate(cnRandom random) {
  cnKdSplitter splitter = malloc(sizeof(struct cnKdSplitter));
  if (!splitter) cnFailTo(DONE, "No splitter.");
  // Init.
  splitter->pointMatrix = NULL;
  splitter->random = random;
  splitter->randomOwned = cnFalse;
  // See if we need more.
  if (!random) {
    splitter->random = cnRandomCreate();
    if (!splitter->random) {
      free(splitter);
      splitter = NULL;
      cnFailTo(DONE, "No random.");
    }
    splitter->randomOwned = cnTrue;
  }
  DONE:
  return splitter;
}


void cnKdSplitterDestroy(cnKdSplitter splitter) {
  if (splitter->randomOwned) cnRandomDestroy(splitter->random);
  free(splitter);
}
