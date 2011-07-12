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
 * Considering an opaque multinomial type here for storing prepared info for
 * fast sampling.
 *
 * TODO Maybe expose some things after all? Just not the inner workings.
 */
typedef struct{}* cnMultinomial;


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
 * Calculates the mahalanobis distance from the mean of the gaussian to the
 * given point.
 *
 * TODO Just Euclidean distance so far!!!
 */
cnFloat cnMahalanobisDistance(cnGaussian* gaussian, cnFloat* point);


/**
 * Generates a sample from the k-outcome multinomal distribution given by n and
 * k values of p, the probability of each outcome. The results are stored in the
 * space provided by out, which should have enough space for k counts.
 *
 * TODO Random stream state management?
 */
void cnMultinomialSample(cnCount k, cnCount* out, cnCount n, cnFloat* p);


/**
 * Generates a sample from a uniform distribution between 0 inclusive and 1
 * exclusive.
 *
 * TODO Random stream state management?
 */
cnFloat cnUnitRand();


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
