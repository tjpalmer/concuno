#ifndef concuno_stats_h
#define concuno_stats_h


#include "entity.h"


// TODO Probability distributions go here, too.


/**
 * A multivariate normal distribution.
 */
typedef struct cnGaussian {

  cnCount dims;

  cnFloat* mean;

  cnFloat* cov;

} cnGaussian;


/**
 * Provides a Mahalanobis distance metric based on a Gaussian distribution.
 */
cnFunction* cnFunctionCreateMahalanobisDistance(cnGaussian* gaussian);


/**
 * Frees the mean and cov, setting them to NULL and dims to zero.
 */
void cnGaussianDispose(cnGaussian* gaussian);


/**
 * Allocates space for the mean and covariance.
 *
 * TODO Init to mean zero and identity cov? Or just NaNs?
 */
cnBool cnGaussianInit(cnGaussian* gaussian, cnCount dims, cnFloat* mean);


/**
 * Calculates the
 */
cnFloat cnMahalanobisDistance(cnGaussian* gaussian, cnFloat* point);


void cnVectorCov(void);


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
