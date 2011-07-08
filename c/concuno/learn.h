#ifndef concuno_learn_h
#define concuno_learn_h


#include "tree.h"


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
 */
void cnLearnerInit(cnLearner* learner);


/**
 * Expands the initial tree as much as possible.
 *
 * For now, it returns a new tree, as that's not very wasteful compared to
 * everything else going on, and I think it will be easier to do this way.
 *
 * There still might be side effects to the initial tree, however.
 *
 * TODO Be more precise about side effects might exist.
 */
cnRootNode* cnLearnTree(cnLearner* learner, cnRootNode* initial);


#endif
