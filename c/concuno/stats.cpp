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

  Count count;

  Float prob;

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
  Index index;

  /**
   * The conditional probability for this binomial given its order in the
   * multinomial.
   */
  Float prob;

} cnMultiBinomial;


typedef struct cnMultinomialInfo {

  /**
   * Technically, we only need classCount - 1 of these, but having classCount of
   * them makes life easier.
   */
  cnMultiBinomial* binomials;

  Count classCount;

  Count sampleCount;

  cnRandom random;

} cnMultinomialInfo;


cnBinomial cnBinomialCreate(cnRandom random, Count count, Float prob) {
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


Count cnBinomialSample(cnBinomial binomial) {
  cnBinomialInfo* info = (cnBinomialInfo*)binomial;
  // TODO They only remember state for one binomial at a time in rk_state.
  // TODO Consider adding support on our end for retaining that state in our own
  // TODO cnBinomialInfo, to copy it into the rk_state.
  return cnRandomBinomial(info->random, info->count, info->prob);
}


bool cnFunctionEvaluateMahalanobisDistance(
  cnFunction* function, void* in, void* out
) {
  *((Float*)out) = cnMahalanobisDistance(
    reinterpret_cast<Gaussian*>(function->info),
    reinterpret_cast<Float*>(in)
  );
  // Always good.
  return true;
}


cnFunction* cnFunctionCreateMahalanobisDistance_copy(cnFunction* function) {
  Gaussian* other = reinterpret_cast<Gaussian*>(function->info);
  Gaussian* gaussian = cnAlloc(Gaussian, 1);
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
  cnGaussianDispose(reinterpret_cast<Gaussian*>(function->info));
  free(function->info);
}

bool cnFunctionCreateMahalanobisDistance_write(
  cnFunction* function, FILE* file, cnString* indent
) {
  Gaussian* gaussian = reinterpret_cast<Gaussian*>(function->info);
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

cnFunction* cnFunctionCreateMahalanobisDistance(Gaussian* gaussian) {
  cnFunction* function = cnAlloc(cnFunction, 1);
  if (!function) return NULL;
  function->info = gaussian;
  function->copy = cnFunctionCreateMahalanobisDistance_copy;
  function->dispose = cnFunctionCreateMahalanobisDistance_dispose;
  function->evaluate = cnFunctionEvaluateMahalanobisDistance;
  function->write = cnFunctionCreateMahalanobisDistance_write;
  return function;
}


void cnGaussianDispose(Gaussian* gaussian) {
  free(gaussian->cov);
  free(gaussian->mean);
  gaussian->cov = NULL;
  gaussian->mean = NULL;
  gaussian->dims = 0;
}


bool cnGaussianInit(Gaussian* gaussian, Count dims, Float* mean) {
  // Make space.
  gaussian->cov = cnAlloc(Float, dims * dims);
  gaussian->mean = cnAlloc(Float, dims);
  if (!(gaussian->cov && gaussian->mean)) goto FAIL;
  // Store values.
  gaussian->dims = dims;
  if (mean) {
    // Use the mean supplied.
    memcpy(gaussian->mean, mean, dims * sizeof(Float));
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


Float cnMahalanobisDistance(Gaussian* gaussian, Float* point) {
  Float distance = 0;
  Count dims = gaussian->dims;
  Float* mean = gaussian->mean;
  Float* meanEnd = mean + dims;
  // TODO Covariance.
  Float *meanValue, *pointValue;
  for (
    meanValue = mean, pointValue = point;
    meanValue < meanEnd;
    meanValue++, pointValue++
  ) {
    Float diff = *pointValue - *meanValue;
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
  cnRandom random, Count sampleCount, Count classCount, Float* probs
) {
  Index i;
  cnMultinomialInfo* info;
  Float probLeft;

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
    Float multiProb = info->binomials[i].prob;
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


void cnMultinomialSample(cnMultinomial multinomial, Count* out) {
  Count samplesLeft;
  Index i;
  cnMultinomialInfo* info = (cnMultinomialInfo*)multinomial;

  // Binomial sample all but the last. Speed matters more for sampling.
  samplesLeft = info->sampleCount;
  for (i = 0; i < info->classCount - 1; i++) {
    Count successCount = info->binomials[i].prob ?
      cnRandomBinomial(info->random, samplesLeft, info->binomials[i].prob) : 0;
    out[info->binomials[i].index] = successCount;
    samplesLeft -= successCount;
  }

  // The last class gets all the remaining.
  out[info->binomials[info->classCount - 1].index] = samplesLeft;
}


bool cnPermutations(
  Count options, Count count,
  bool (*handler)(void* info, Count count, Index* permutation),
  void* data
) {
  bool result = false;
  Index c, o;
  Index* permutation = cnAlloc(Index, count);
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


Count cnRandomBinomial(cnRandom random, Count count, Float prob) {
  rk_state* state = (rk_state*)random;
  return rk_binomial(state, count, prob);
}


void cnRandomDestroy(cnRandom random) {
  free(random);
}


Float cnScalarCovariance(
  Count count,
  Count skipA, Float* inA,
  Count skipB, Float* inB
) {
  Index i;
  Float sumA = 0.0;
  Float sumB = 0.0;
  Float sumAB = 0.0;
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


Float cnScalarVariance(Count count, Count skip, Float* in) {
  // TODO Is this right?
  return cnScalarCovariance(count, skip, in, skip, in);
}


Float cnUnitRand() {
  // TODO This is a horrible implementation right now. Do much improvement!
  Count value = rand();
  // Despite the abuse, subtract 1 instead of adding to RAND_MAX, in case
  // RAND_MAX is already at the cap before rollover.
  if (value == RAND_MAX) value--;
  return value / (Float)RAND_MAX;
}


void cnVectorCov(void) {
  // TODO
  //cblas_dgemm(
  //  CblasColMajor, CblasNoTrans, CblasTrans, size, count, size,
  //  1, in, size, in, count, 0, NULL, 0
  //);
}


Float* cnVectorMax(Count size, Float* out, Count count, Float* in) {
  Float *inBegin = in, *inEnd = in + size * count;
  Float* outEnd = out + size;
  if (!count) {
    // I don't know the max of nothing.
    Float nan = cnNaN();
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


Float* cnVectorMean(Count size, Float* out, Count count, Float* in) {
  Float *inBegin = in, *inEnd = in + size * count;
  Float* outEnd = out + size;
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


Float* cnVectorMin(Count size, Float* out, Count count, Float* in) {
  Float *inBegin = in, *inEnd = in + size * count;
  Float* outEnd = out + size;
  if (!count) {
    // I don't know the max of nothing.
    Float nan = cnNaN();
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
