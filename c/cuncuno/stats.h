#ifndef cuncuno_stats_h
#define cuncuno_stats_h


#include "core.h"


// TODO Probability distributions go here, too.


/**
 * Put the max of count vectors of the given size from in into out.
 *
 * Returns out.
 */
cnFloat* cnVectorMax(cnCount size, cnFloat* out, cnCount count, cnFloat* in);


/**
 * Put the mean of count vectors of the given size from in into out.
 *
 * Returns out.
 */
cnFloat* cnVectorMean(cnCount size, cnFloat* out, cnCount count, cnFloat* in);


/**
 * Put the min of count vectors of the given size from in into out.
 *
 * Returns out.
 */
cnFloat* cnVectorMin(cnCount size, cnFloat* out, cnCount count, cnFloat* in);


#endif
