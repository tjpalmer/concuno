#include "search.h"


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
  cnList(cnSearchOption) nexts;
  cnBool result = cnFalse;
  cnSearcherSelf self = (cnSearcherSelf)searcher;

  // Init.
  cnListInit(&nexts, sizeof(cnSearchOption));

  // Push initial options on the heap.
  cnListEachBegin(&searcher->initialOptions, cnSearchOption, option) {
    if (!cnHeapPush(self->options, *option)) cnFailTo(DONE, "No push.");
  } cnEnd;

  // Keep looping while we have any options, and they don't say we're done.
  // TODO A second worst-first heap for pruning options too bad? If so, would
  // TODO need the ability to locate and prune them from the best-first heap,
  // TODO too.
  while (self->options->items.count && !searcher->finished(searcher)) {
    cnBool failed;

    // TODO Multithread out N at a time.

    // Find our next search point.
    cnSearchOption contender = cnHeapPull(self->options);

    // See if the contender is our new best.
    if (
      !searcher->bestOption ||
      searcher->better(searcher, contender, searcher->bestOption)
    ) {
      // The old best has been bested.
      if (searcher->destroyOption) {
        searcher->destroyOption(searcher, searcher->bestOption);
      }
      searcher->bestOption = contender;
    }

    // Search from the contender either way.
    failed = !searcher->step(searcher, contender, &nexts);
    // If the contender wasn't our best, destroy it now. It has served its
    // purpose in search.
    // TODO Actually, I'll need to hand all past some threshold to some second
    // TODO tier of comparison, I think. Ponder this.
    if (contender != searcher->bestOption && searcher->destroyOption) {
      searcher->destroyOption(searcher, contender);
    }
    // Defer failure check until after we've destroyed any failed contender.
    if (failed) cnFailTo(DONE, "No step.");

    // Push the next options.
    cnListEachBegin(&nexts, cnSearchOption, option) {
      if (!cnHeapPush(self->options, *option)) cnFailTo(DONE, "No push.");
    } cnEnd;
  }
  result = cnTrue;

  DONE:
  cnListDispose(&nexts);
  cnHeapClear(self->options);
  return result;
}


void cnSearcherCreate_destroyItem(void* info, void* item) {
  // Call destroy if we have one.
  cnSearcher searcher = info;
  if (searcher->destroyOption) searcher->destroyOption(searcher, item);
}

cnBool cnSearcherCreate_less(void* info, void* a, void* b) {
  cnSearcher searcher = info;
  // If it's better, it should bubble to the top. So better means less.
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
