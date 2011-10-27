#ifndef concuno_kdtree_h
#define concuno_kdtree_h


#include "stats.h"
#include "tree.h"


cnCBegin;


struct cnKdSplitter {

  // TODO Modes such as random projection or PCA?
  // TODO Or use different top level split functions for that?

  /**
   * An packed matrix of points to be split.
   *
   * TODO Arbitrary entities with functions (splittable on any)?
   */
  cnPointMatrix pointMatrix;

  /**
   * For maintaining random state.
   */
  cnRandom random;

  /**
   * Whether the random is owned by the learner. Defaults to true when the
   * random is created automatically for the learner.
   *
   * TODO Switch random to always destroyed and require others to null it before
   * TODO destroying the splitter?
   */
  cnBool randomOwned;

};


cnRootNode* cnKdSplit(cnKdSplitter* splitter);


cnKdSplitter* cnKdSplitterCreate(cnRandom random);


void cnKdSplitterDestroy(cnKdSplitter* splitter);


cnCEnd;


#endif
