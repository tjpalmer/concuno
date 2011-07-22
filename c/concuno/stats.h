#ifndef concuno_stats_h
#define concuno_stats_h


#include "entity.h"


typedef struct{}* cnBinomial;


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
 * Often count and prob are called n and p, respectively. There are count
 * samples drawn at a time, and prob is the probability of "success" for each.
 *
 * I have built this with the idea of allowing a setup process to prepare for
 * fast repeated sampling from the same distribution.
 *
 * Note that I'm experimenting with opaque types here.
 *
 * TODO Optional random stream.
 *
 * TODO Citation to the folks with the algorithm I use.
 */
cnBinomial cnBinomialCreate(cnCount count, cnFloat prob);


/**
 * TODO I don't like destroy because in some contexts, it is confusing, and the
 * TODO same holds for create. Do we create and destroy files with these things?
 * TODO I got "drop" from Irrlicht, I think, where it is used for reference
 * TODO counting. If I didn't use dispose for non-free disposal, I could use it
 * TODO here. Maybe "close" is okay. I'm not sure.
 */
void cnBinomialDestroy(cnBinomial binomial);


/**
 * Returns the number of successes for a draw of count samples, ranging between
 * 0 and count.
 */
cnCount cnBinomialSample(cnBinomial binomial);


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
 * Permute the options, taken count at a time. Values go from 0 to options - 1.
 * The handler receives the permutations of options in sequence, given the count
 * back again for convenience and the provided data pointer.
 */
cnBool cnPermutations(
  cnCount options, cnCount count,
  cnBool (*handler)(void* data, cnCount count, cnIndex* permutation),
  void* data
);


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
