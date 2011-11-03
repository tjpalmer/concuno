#include "kdtree.h"


namespace concuno {


RootNode* cnKdSplit(cnKdSplitter* splitter) {
  if (splitter->pointMatrix->topology != Topology::Euclidean) {
    cnErrTo(DONE, "Only Euclidean for now.");
  }
  DONE:
  return NULL;
}


cnKdSplitter* cnKdSplitterCreate(Random random) {
  cnKdSplitter* splitter = new cnKdSplitter;
  if (!splitter) cnErrTo(DONE, "No splitter.");
  // Init.
  splitter->pointMatrix = NULL;
  splitter->random = random;
  splitter->randomOwned = false;
  // See if we need more.
  if (!random) {
    splitter->random = cnRandomCreate();
    if (!splitter->random) {
      free(splitter);
      splitter = NULL;
      cnErrTo(DONE, "No random.");
    }
    splitter->randomOwned = true;
  }
  DONE:
  return splitter;
}


void cnKdSplitterDestroy(cnKdSplitter* splitter) {
  if (splitter->randomOwned) cnRandomDestroy(splitter->random);
  delete splitter;
}


}
