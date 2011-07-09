//#include <cblas.h>
//#include <clapack.h>
#include <math.h>
#include <string.h>

#include "stats.h"


cnBool cnFunctionEvaluateMahalanobisDistance(
  cnFunction* function, void* in, void* out
) {
  *((cnFloat*)out) = cnMahalanobisDistance(function->data, in);
  // Always good.
  return cnTrue;
}


void cnFunctionCreateMahalanobisDistance_Dispose(cnFunction* function) {
  cnGaussianDispose(function->data);
  free(function->data);
}

cnFunction* cnFunctionCreateMahalanobisDistance(cnGaussian* gaussian) {
  cnFunction* function = malloc(sizeof(cnFunction));
  if (!function) return NULL;
  function->data = gaussian;
  function->dispose = cnFunctionCreateMahalanobisDistance_Dispose;
  function->evaluate = cnFunctionEvaluateMahalanobisDistance;
  return function;
}


void cnGaussianDispose(cnGaussian* gaussian) {
  free(gaussian->cov);
  free(gaussian->mean);
  gaussian->cov = NULL;
  gaussian->mean = NULL;
  gaussian->dims = 0;
}


cnBool cnGaussianInit(cnGaussian* gaussian, cnCount dims, cnFloat* mean) {
  // Make space.
  gaussian->cov = malloc(dims * dims * sizeof(cnFloat));
  gaussian->mean = malloc(dims * sizeof(cnFloat));
  if (!(gaussian->cov && gaussian->mean)) goto FAIL;
  // Store values.
  gaussian->dims = dims;
  memcpy(gaussian->mean, mean, dims * sizeof(cnFloat));
  // Good to go.
  return cnTrue;
  // Or not.
  FAIL:
  cnGaussianDispose(gaussian);
  return cnFalse;
}


cnFloat cnMahalanobisDistance(cnGaussian* gaussian, cnFloat* point) {
  cnFloat distance = 0;
  cnCount dims = gaussian->dims;
  cnFloat* mean = gaussian->mean;
  cnFloat* meanEnd = mean + dims;
  // TODO Covariance.
  cnFloat *meanValue, *pointValue;
  for (
    meanValue = mean, pointValue = point;
    meanValue < meanEnd;
    meanValue++, pointValue++
  ) {
    cnFloat diff = *pointValue - *meanValue;
    // TODO Apply covariance scaling.
    distance += diff * diff;
  }
  // Provide actual distance instead of squared, for intuition.
  distance = sqrt(distance);
  return distance;
}


void cnMultinomialSample(cnCount k, cnCount* out, cnCount n, cnFloat* p) {
  // TODO Consider Kemp's modal binomial algorithm as seen in Davis, 1993:
  // TODO "The computer generation of multinomial random variates". This is
  // TODO also the technique used by the GNU Scientific Library, from which I
  // TODO got the reference.
  // TODO
  // TODO The summary is to generate k binomial variables then convert these
  // TODO into a multinomial sample. Generating a binomial should take only
  // TODO O(log n), instead of O(n) for naive multinomial sampling. One sample
  // TODO from a binomial can be done with binary search on the CDF.

  cnIndex i;

  // TODO Assert that p sums to 1?

  // Zero out the counts.
  for (i = 0; i < k; i++) {
    out[i] = 0;
  }

  // Select n samples.
  for (i = 0; i < n; i++) {
    cnIndex outcome;
    cnFloat sample = cnUnitRand();
    cnFloat sum = 0;
    // Another option is to precompute a CDF instead of summing each time, but
    // eh.
    for (outcome = 0; outcome < k; outcome++) {
      sum += p[outcome];
      if (sample <= sum) {
        break;
      }
    }
    // Just a failsafe on outcome, in case oddities happened.
    if (outcome == k) outcome--;
    // Add the sample.
    out[outcome]++;
  }
}


cnFloat cnUnitRand() {
  // TODO This is a horrible implementation right now. Do much improvement!
  cnCount value = rand();
  // Despite the abuse, subtract 1 instead of adding to RAND_MAX, in case
  // RAND_MAX is already at the cap before rollover.
  if (value == RAND_MAX) value--;
  return value / (cnFloat)RAND_MAX;
}


void cnVectorCov(void) {
  // TODO
  //cblas_dgemm(
  //  CblasColMajor, CblasNoTrans, CblasTrans, size, count, size,
  //  1, in, size, in, count, 0, NULL, 0
  //);
}


cnFloat* cnVectorMax(cnCount size, cnFloat* out, cnCount count, cnFloat* in) {
  cnFloat *inBegin = in, *inEnd = in + size * count;
  cnFloat* outEnd = out + size;
  if (!count) {
    // I don't know the max of nothing.
    cnFloat nan = cnNaN();
    for (; out < outEnd; out++) {
      *out = nan;
    }
  } else {
    // TODO Go one vector at a time for nicer memory access?
    // Sane situation now.
    for (; out < outEnd; out++, inBegin++) {
      *out = *inBegin;
      for (in = inBegin + size; in < inEnd; in += size) {
        if (*out < *in) {
          *out = *in;
        }
      }
    }
  }
  return out - size;
}


cnFloat* cnVectorMean(cnCount size, cnFloat* out, cnCount count, cnFloat* in) {
  cnFloat *inBegin = in, *inEnd = in + size * count;
  cnFloat* outEnd = out + size;
  // TODO Go one vector at a time for nicer memory access?
  // Do one element of out at a time, starting in from an equivalent offset.
  for (; out < outEnd; out++, inBegin++) {
    *out = 0;
    for (in = inBegin; in < inEnd; in += size) {
      *out += *in;
    }
    // Automatic NaN for no count.
    *out /= count;
  }
  return out - size;
}


cnFloat* cnVectorMin(cnCount size, cnFloat* out, cnCount count, cnFloat* in) {
  cnFloat *inBegin = in, *inEnd = in + size * count;
  cnFloat* outEnd = out + size;
  if (!count) {
    // I don't know the max of nothing.
    cnFloat nan = cnNaN();
    for (; out < outEnd; out++) {
      *out = nan;
    }
  } else {
    // TODO Go one vector at a time for nicer memory access?
    // Sane situation now.
    for (; out < outEnd; out++, inBegin++) {
      *out = *inBegin;
      for (in = inBegin + size; in < inEnd; in += size) {
        if (*out > *in) {
          *out = *in;
        }
      }
    }
  }
  return out - size;
}
