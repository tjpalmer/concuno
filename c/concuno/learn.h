#ifndef concuno_learn_h
#define concuno_learn_h


#include "tree.h"


/**
 * The whole Learner thing is a placeholder. I can't imagine not wanting a few
 * configuration options at some point.
 */
typedef struct cnLearner {
  // TODO Learning options go here.
} cnLearner;


/**
 * Cleans up whatever might be needed.
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
