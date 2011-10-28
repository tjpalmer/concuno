//#include <cblas.h>
//#include <clapack.h>
#include <math.h>
#include <string.h>

#include "io.h"
#include "mat.h"
#include "numpy-mtrand/distributions.h"
#include "numpy-mtrand/randomkit.h"
#include "stats.h"


namespace concuno {


typedef struct cnBinomialInfo {

  cnCount count;

  cnFloat prob;

  cnRandom random;

} cnBinomialInfo;


/**
 * Used for each class of a multinomial.
 */
typedef struct cnMultiBinomial {

  /**
   * We need to remember the index, because we sort by decreasing probability
   * and need to get back to where we were.
   */
  cnIndex index;

  /**
   * The conditional probability for this binomial given its order in the
   * multinomial.
   */
  cnFloat prob;

} cnMultiBinomial;


typedef struct cnMultinomialInfo {

  /**
   * Technically, we only need classCount - 1 of these, but having classCount of
   * them makes life easier.
   */
  cnMultiBinomial* binomials;

  cnCount classCount;

  cnCount sampleCount;

  cnRandom random;

} cnMultinomialInfo;


cnBinomial cnBinomialCreate(cnRandom random, cnCount count, cnFloat prob) {
  cnBinomialInfo* binomial;

  // Allocate space.
  if (!(binomial = cnAlloc(cnBinomialInfo, 1))) {
    cnErrTo(DONE, "No binomial.");
  }

  // Remember parameters.
  binomial->count = count;
  binomial->prob = prob;
  binomial->random = random;

  DONE:
  return (cnBinomial)binomial;
}


void cnBinomialDestroy(cnBinomial binomial) {
  free(binomial);
}


cnCount cnBinomialSample(cnBinomial binomial) {
  cnBinomialInfo* info = (cnBinomialInfo*)binomial;
  // TODO They only remember state for one binomial at a time in rk_state.
  // TODO Consider adding support on our end for retaining that state in our own
  // TODO cnBinomialInfo, to copy it into the rk_state.
  return cnRandomBinomial(info->random, info->count, info->prob);
}


bool cnFunctionEvaluateMahalanobisDistance(
  cnFunction* function, void* in, void* out
) {
  *((cnFloat*)out) = cnMahalanobisDistance(
    reinterpret_cast<cnGaussian*>(function->info),
    reinterpret_cast<cnFloat*>(in)
  );
  // Always good.
  return true;
}


cnFunction* cnFunctionCreateMahalanobisDistance_copy(cnFunction* function) {
  cnGaussian* other = reinterpret_cast<cnGaussian*>(function->info);
  cnGaussian* gaussian = cnAlloc(cnGaussian, 1);
  cnFunction* copy;
  if (!gaussian) return NULL;
  if (!cnGaussianInit(gaussian, other->dims, other->mean)) {
    free(gaussian);
    return NULL;
  }
  copy = cnFunctionCreateMahalanobisDistance(gaussian);
  if (!copy) {
    cnGaussianDispose(gaussian);
    free(gaussian);
  }
  return copy;
}

void cnFunctionCreateMahalanobisDistance_dispose(cnFunction* function) {
  cnGaussianDispose(reinterpret_cast<cnGaussian*>(function->info));
  free(function->info);
}

bool cnFunctionCreateMahalanobisDistance_write(
  cnFunction* function, FILE* file, cnString* indent
) {
  cnGaussian* gaussian = reinterpret_cast<cnGaussian*>(function->info);
  bool result = false;

  // TODO Check error state?
  fprintf(file, "{\n");
  cnIndent(indent);
  fprintf(file, "%s\"name\": \"MahalanobisDistance\",\n", cnStr(indent));
  fprintf(file, "%s\"center\": [", cnStr(indent));
  cnVectorPrintDelimited(file, gaussian->dims, gaussian->mean, ", ");
  fprintf(file, "]\n");
  // TODO Covariance!
  // TODO Strength (for use as later prior)!
  cnDedent(indent);
  fprintf(file, "%s}", cnStr(indent));

  // Winned!
  result = true;

  return result;
}

cnFunction* cnFunctionCreateMahalanobisDistance(cnGaussian* gaussian) {
  cnFunction* function = cnAlloc(cnFunction, 1);
  if (!function) return NULL;
  function->info = gaussian;
  function->copy = cnFunctionCreateMahalanobisDistance_copy;
  function->dispose = cnFunctionCreateMahalanobisDistance_dispose;
  function->evaluate = cnFunctionEvaluateMahalanobisDistance;
  function->write = cnFunctionCreateMahalanobisDistance_write;
  return function;
}


void cnGaussianDispose(cnGaussian* gaussian) {
  free(gaussian->cov);
  free(gaussian->mean);
  gaussian->cov = NULL;
  gaussian->mean = NULL;
  gaussian->dims = 0;
}


bool cnGaussianInit(cnGaussian* gaussian, cnCount dims, cnFloat* mean) {
  // Make space.
  gaussian->cov = cnAlloc(cnFloat, dims * dims);
  gaussian->mean = cnAlloc(cnFloat, dims);
  if (!(gaussian->cov && gaussian->mean)) goto FAIL;
  // Store values.
  gaussian->dims = dims;
  if (mean) {
    // Use the mean supplied.
    memcpy(gaussian->mean, mean, dims * sizeof(cnFloat));
  } else {
    // Just center at 0 for now.
    // TODO Vector fill function?
    int d;
    for (d = 0; d < dims; d++) {
      gaussian->mean[d] = 0.0;
    }
  }
  // Good to go.
  return true;
  // Or not.
  FAIL:
  cnGaussianDispose(gaussian);
  return false;
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


int cnMultinomialCreate_compareBinomials(const void *a, const void *b) {
  cnMultiBinomial* binA = (cnMultiBinomial*)a;
  cnMultiBinomial* binB = (cnMultiBinomial*)b;
  // Higher probability means earlier in sequence for this.
  return binA->prob > binB->prob ? -1 : binA->prob != binB->prob;
}

cnMultinomial cnMultinomialCreate(
  cnRandom random, cnCount sampleCount, cnCount classCount, cnFloat* probs
) {
  cnIndex i;
  cnMultinomialInfo* info;
  cnFloat probLeft;

  // Allocate and init basics.
  if (!(info = cnAlloc(cnMultinomialInfo, 1))) {
    cnErrTo(FAIL, "No multinomial.");
  }
  info->binomials = NULL;
  info->classCount = classCount;
  info->sampleCount = sampleCount;
  info->random = random;

  // Allocate binomial info.
  if (!(info->binomials = cnAlloc(cnMultiBinomial, classCount))) {
    cnErrTo(FAIL, "No multibinomials.");
  }

  // Init (to original probs), and sort them descending by prob.
  probLeft = 0.0;
  for (i = 0; i < classCount; i++) {
    info->binomials[i].index = i;
    info->binomials[i].prob = probs[i];
    probLeft += probs[i];
  }
  // TODO What's a good epsilon?
  if (fabs(probLeft - 1.0) > 1e-6) {
    cnErrTo(FAIL, "Probs sum to %lg, not 1.", probLeft);
  }
  qsort(
    info->binomials, classCount, sizeof(cnMultiBinomial),
    cnMultinomialCreate_compareBinomials
  );

  // With that done, let's now create the conditional probabilities. Could skip
  // both first and last classes, if we want to be clever, but don't worry about
  // that for now.
  probLeft = 1.0;
  for (i = 0; i < classCount; i++) {
    cnFloat multiProb = info->binomials[i].prob;
    // Keep it at 0 instead of going NaN for 0-prob classes.
    if (multiProb) {
      info->binomials[i].prob /= probLeft;
    }
    probLeft -= multiProb;
  }

  // Winned!
  goto DONE;

  FAIL:
  cnMultinomialDestroy((cnMultinomial)info);
  info = NULL;

  DONE:
  return (cnMultinomial)info;
}


void cnMultinomialDestroy(cnMultinomial multinomial) {
  cnMultinomialInfo* info = (cnMultinomialInfo*)multinomial;
  if (info) {
    free(info->binomials);
    free(info);
  }
}


void cnMultinomialSample(cnMultinomial multinomial, cnCount* out) {
  cnCount samplesLeft;
  cnIndex i;
  cnMultinomialInfo* info = (cnMultinomialInfo*)multinomial;

  // Binomial sample all but the last. Speed matters more for sampling.
  samplesLeft = info->sampleCount;
  for (i = 0; i < info->classCount - 1; i++) {
    cnCount successCount = info->binomials[i].prob ?
      cnRandomBinomial(info->random, samplesLeft, info->binomials[i].prob) : 0;
    out[info->binomials[i].index] = successCount;
    samplesLeft -= successCount;
  }

  // The last class gets all the remaining.
  out[info->binomials[info->classCount - 1].index] = samplesLeft;
}


bool cnPermutations(
  cnCount options, cnCount count,
  bool (*handler)(void* info, cnCount count, cnIndex* permutation),
  void* data
) {
  bool result = false;
  cnIndex c, o;
  cnIndex* permutation = cnAlloc(cnIndex, count);
  bool* used = cnAlloc(bool, options);
  if (!(permutation && used)) cnErrTo(DONE, "No permutation or used.");

  // Sanity checks.
  if (count < 0) cnErrTo(DONE, "count %ld < 0", count);
  if (options < 0) cnErrTo(DONE, "options %ld < 0", options);
  if (count > options) {
    cnErrTo(DONE, "count %ld > options %ld", count, options);
  }

  // Clear out the info.
  for (c = 0; c < count; c++) permutation[c] = -1;
  for (o = 0; o < options; o++) used[o] = false;

  // Loop until we've gone past the last option in the first spot.
  c = 0;
  while (c >= 0) {
    // Mark our current option free before moving on.
    if (permutation[c] >= 0) {
      used[permutation[c]] = false;
    } else {
      permutation[c] = -1;
    }

    // Find an available option, if any.
    do {
      permutation[c]++;
    } while (permutation[c] < options && used[permutation[c]]);

    // See where we are.
    if (permutation[c] >= options) {
      // Used up options. Go back.
      permutation[c] = -1;
      c--;
    } else {
      // Found one. Move on.
      used[permutation[c]] = true;
      c++;
      if (c >= count) {
        // Past the end. Report the permutation, and move back.
        if (!handler(data, count, permutation)) {
          cnErrTo(DONE, "Handler failed.");
        }
        c--;
      }
    }
  }
  // We winned!
  result = true;

  DONE:
  free(permutation);
  free(used);
  return result;
}


cnRandom cnRandomCreate(void) {
  rk_state* state;

  if (!(state = cnAlloc(rk_state, 1))) cnErrTo(DONE, "No random.");
  // TODO Is 0 a particularly bad seed?
  rk_seed(0, state);

  DONE:
  return (cnRandom)state;
}


cnCount cnRandomBinomial(cnRandom random, cnCount count, cnFloat prob) {
  rk_state* state = (rk_state*)random;
  return rk_binomial(state, count, prob);
}


void cnRandomDestroy(cnRandom random) {
  free(random);
}


cnFloat cnScalarCovariance(
  cnCount count,
  cnCount skipA, cnFloat* inA,
  cnCount skipB, cnFloat* inB
) {
  cnIndex i;
  cnFloat sumA = 0.0;
  cnFloat sumB = 0.0;
  cnFloat sumAB = 0.0;
  for (i = 0; i < count; i++) {
    sumA += *inA;
    sumB += *inB;
    sumAB += *inA * *inB;
    inA += skipA;
    inB += skipB;
  }
  // From Wikipedia: E(A * B) - E(A) * E(B)
  // And subtract 1 from the denominator for "unbiased".
  return (sumAB - (sumA * sumB) / count) / (count - 1);
}


cnFloat cnScalarVariance(cnCount count, cnCount skip, cnFloat* in) {
  // TODO Is this right?
  return cnScalarCovariance(count, skip, in, skip, in);
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


}