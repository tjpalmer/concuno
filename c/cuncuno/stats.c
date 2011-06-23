//#include <cblas.h>
//#include <clapack.h>
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
  return distance;
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
