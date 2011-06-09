#include <cblas.h>
#include "stats.h"


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
