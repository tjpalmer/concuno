#ifndef concuno_learn_h
#define concuno_learn_h


#include "stats.h"
#include "tree.h"


namespace concuno {


/**
 * The whole Learner thing is a placeholder. I can't imagine not wanting a few
 * configuration options at some point.
 */
typedef struct cnLearner {
  // TODO Learning options go here.

  /**
   * The training data to be used for learning. It could be subdivided into
   * separate training and validation data, if needed, but that's managed
   * internally.
   *
   * This should be preshuffled as the caller sees fit.
   * TODO Handle shuffling internally instead??
   *
   * These will not be disposed of with the learner. Outside code needs to take
   * care of that.
   */
  cnList(cnBag)* bags;

  /**
   * The entity functions to be used during the learning process.
   *
   * Managed separately from the learner itself. No disposal here.
   */
  cnList(cnEntityFunction)* entityFunctions;

  /**
   * The tree to work from for learning.
   *
   * TODO If a tree is provided, are there any side effects to it? For now,
   * TODO maybe at least in the form of bindings?
   *
   * If null, instead learn from a stub tree (one with just a root and a leaf
   * node). Any such tree will be kept internally in the learning algorithm
   * rather than being exposed here.
   *
   * Any tree provided here will _not_ be disposed with the learner.
   */
  cnRootNode* initialTree;

  /**
   * For maintaining random state.
   */
  cnRandom random;

  /**
   * Whether the random is owned by the learner. Defaults to true when the
   * random is created automatically for the learner.
   */
  bool randomOwned;

} cnLearner;


/**
 * Cleans up whatever might be needed.
 *
 * For now, it just sets bags to null, so the caller needs to have separate
 * access to them if they haven't yet done the cleanup.
 */
void cnLearnerDispose(cnLearner* learner);


/**
 * Inits default options.
 *
 * Provide a NULL random to have one automatically generated.
 *
 * Even on failure, the learner is safe to dispose.
 */
bool cnLearnerInit(cnLearner* learner, cnRandom random);


/**
 * Returns a new tree, expanded as much as possible, although possibly just a
 * clone of the initial tree, if such a tree is provided.
 *
 * TODO What's the easiest way to know if it learned something? Check the
 * TODO number leaves or information in the nodes?
 *
 * TODO Fill a list of trees for beam search purposes?
 */
cnRootNode* cnLearnTree(cnLearner* learner);


}


#endif
