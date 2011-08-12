#include "search.h"
#include <stdio.h>


typedef struct cnSearcherSelf {

  /**
   * The searcher is first.
   */
  struct cnSearcher searcher;

  /**
   * Prioritized options for search.
   */
  cnHeap(cnSearchOption) options;

  // TODO Threading and mutexes?

}* cnSearcherSelf;


cnBool cnSearch(cnSearcher searcher) {
  // TODO
  return cnFalse;
}


void cnSearcherCreate_destroyItem(void* info, void* item) {
  // Call destroy if we have one.
  cnSearcher searcher = info;
  if (searcher->destroyOption) searcher->destroyOption(searcher, item);
}

cnBool cnSearcherCreate_less(void* info, void* a, void* b) {
  cnSearcher searcher = info;
  // If it's better, it should bubble to the top.
  return searcher->better(searcher, a, b);
}

cnSearcher cnSearcherCreate(void) {
  cnSearcher searcher = NULL;
  cnSearcherSelf self = malloc(sizeof(struct cnSearcherSelf));
  if (!self) cnFailTo(DONE, "No searcher.");

  // Safety first.
  searcher = &self->searcher;
  searcher->bestOption = NULL;
  searcher->better = NULL;
  searcher->destroyInfo = NULL;
  searcher->destroyOption = NULL;
  searcher->finished = NULL;
  searcher->info = NULL;
  cnListInit(&searcher->initialOptions, sizeof(cnSearchOption));
  searcher->step = NULL;

  // Now possible failures.
  if (!(self->options = cnHeapCreate(cnSearcherCreate_less))) {
    cnFailTo(FAIL, "No options heap.");
  }
  self->options->info = self;
  self->options->destroyItem = cnSearcherCreate_destroyItem;

  FAIL:
  cnSearcherDestroy(searcher);
  searcher = NULL;

  DONE:
  return searcher;
}


void cnSearcherDestroy(cnSearcher searcher) {
  cnSearcherSelf self = (cnSearcherSelf)searcher;
  if (!searcher) return;

  // Options heap.
  cnHeapDestroy(self->options);
  self->options = NULL;

  // Best and initial options.
  // TODO Or is this a bad idea?
  if (searcher->destroyOption) {
    searcher->destroyOption(searcher, searcher->bestOption);
    cnListEachBegin(&searcher->initialOptions, cnSearchOption, option) {
      searcher->destroyOption(searcher, *option);
    } cnEnd;
  }

  // Info.
  if (searcher->destroyInfo) {
    searcher->destroyInfo(searcher->info);
  }

  // And free.
  free(self);
}
