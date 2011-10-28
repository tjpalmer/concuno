#include <math.h>

#include "cluster.h"
#include "mat.h"
#include "stats.h"


namespace concuno {


/**
 * TODO A dissimilarity metric, too.
 */
bool cnCluster(cnCount size, cnCount count, cnFloat* points);


bool cnDensityEstimate(cnCount size, cnCount count, cnFloat* points);


void cnLogPoints(cnEntityFunction* function, cnCount count, cnFloat* points);


bool cnCluster(cnCount size, cnCount count, cnFloat* points) {
  cnFloat* point;
  cnFloat* pointsEnd = points + size * count;
  cnDensityEstimate(size, count, points);
  for (point = points; point < pointsEnd; point += size) {
    printf("Starting at: ");
    cnVectorPrint(stdout, size, point);
    printf("\n");
    // TODO Mean shift or something.
    break;
  }
  return true;
}


bool cnClusterOnFunction(cnList(Bag)* bags, cnEntityFunction* function) {
  bool result = false;
  cnFloat* point;
  cnFloat* points;
  cnCount pointCount = 0;

  // Check our limitations.
  if (function->inCount != 1) {
    printf("Only handle arity 1 for now, not %ld\n", function->inCount);
  }
  if (function->outType != function->outType->schema->floatType) {
    printf("Only handle float type, not %s\n", cnStr(&function->outType->name));
  }

  // Count the points, and allocate space.
  // TODO Do we need to be bag aware at all? Any bias from all together?
  cnListEachBegin(bags, Bag, bag) {
    pointCount += bag->entities->count;
  } cnEnd;
  printf("%ld points total\n", pointCount);
  points = cnAlloc(cnFloat, pointCount * function->outCount);
  if (!points) {
    return false;
  }

  // Get the point data.
  point = points;
  cnListEachBegin(bags, Bag, bag) {
    cnListEachBegin(bag->entities, void*, entity) {
      function->get(function, entity, point);
      point += function->outCount;
    } cnEnd;
  } cnEnd;

  // cnLogPoints(function, pointCount, points);
  result = cnCluster(function->outCount, pointCount, points);

  free(points);
  return result;
}


bool cnDensityEstimate(cnCount size, cnCount count, cnFloat* points) {

  // TODO Replace all this with precalculated kernel mask and array-based
  // TODO convolution? Or else more clever knn?

  cnIndex dim;
  cnFloat kernelFactor;
  cnFloat* pointsEnd = points + size * count;
  // TODO Parameterize step counts.
  // TODO Allow different step counts by dimension.
  cnCount stepCount = 31;
  cnFloat maxStepSize;
  // Hack these 4 together as a matrix, for convenience.
  cnFloat* max = cnStackAllocOf(cnFloat, 4 * size);
  cnFloat* min = max + size;
  cnFloat* point = min + size;
  cnFloat* stepSize = point + size;
  // And another we need of a different type.
  // We could try to avoid this, but being discrete feels safer.
  cnIndex* stepIndices = cnStackAllocOf(cnIndex, size);

  FILE *file = fopen("cnDensityEstimate.log", "w");

  // Check okay, find bounds, and determine step size.
  if (!(max && stepIndices)) return false;
  cnVectorMax(size, max, count, points);
  cnVectorMin(size, min, count, points);
  for (dim = 0; dim < size; dim++) {
    stepSize[dim] = (max[dim] - min[dim]) / stepCount;
    stepIndices[dim] = 0;
    point[dim] = min[dim];
  }
  // TODO Multivariate Gaussian in stats! Isotropic okay for now, though.
  cnVectorMin(1, &maxStepSize, size, stepSize);
  kernelFactor = 1 / pow(2 * cnPi * maxStepSize, 0.5 * size);
  //printf("Min: "); cnVectorPrint(stdout, size, min); printf("\n");
  //printf("Max: "); cnVectorPrint(stdout, size, max); printf("\n");
  //printf("Step: "); cnVectorPrint(stdout, size, step); printf("\n");

  // TODO Replace this with a loop on points and finding the right box for each
  // TODO point! This nonsense here is crazy slow!
  // TODO Current: O(points x grid size).
  // TODO Better: O(points x kernel size).

  // Loop through all dimensions, etc., where slowest loop is earliest dim.
  // Recursion is another option here, but this seems more convenient at the
  // moment, at least for C.
  while (*stepIndices < stepCount) {
    // Estimate density at this point.
    cnFloat density = 0;
    cnFloat* other;
    for (other = points; other < pointsEnd; other += size) {
      // TODO Finish general Gaussian implementation.
      cnFloat distance = cnSquaredEuclideanDistance(size, point, other);
      // Four standard deviations out should be more than enough.
      if (distance < 16 * maxStepSize) {
        cnFloat weight =
          kernelFactor * exp(-0.5 * distance / pow(maxStepSize, size));
        density += weight;
      }
    }
    cnVectorPrint(file, size, point);
    fprintf(file, " %.4le\n", density);
    // TODO Log the density to a file for now?
    // TODO Use a callback for when wanting just to use and not store a big
    // TODO matrix?

    // Advance to the next point.
    dim = size - 1;
    for (dim = size - 1; dim >= 0; dim--) {
      stepIndices[dim]++;
      if (stepIndices[dim] < stepCount) {
        // This dim still has space left, so we're good.
        point[dim] += stepSize[dim];
        break;
      }
      if (dim) {
        // Hit the limit before the earliest dim, so wrap around.
        printf(".");
        fflush(stdout);
        fprintf(file, "\n");
        point[dim] = min[dim];
        stepIndices[dim] = 0;
      }
    }
  }
  printf("\n");

  // Free and done.
  cnStackFree(max);
  cnStackFree(stepIndices);
  fclose(file);
  return true;
}


void cnLogPoints(cnEntityFunction* function, cnCount count, cnFloat* points) {
  FILE *file;
  cnString name;
  cnFloat* point;
  cnCount size = function->outCount;
  cnFloat* pointsEnd = points + size * count;

  // Prepare file name, and open/create output file.
  cnStringInit(&name);
  if (!cnListPushAll(&name, &function->name)) {
    cnStringDispose(&name);
    // TODO Error code.
    return;
  }
  if (!cnStringPushStr(&name, ".log")) {
    cnStringDispose(&name);
    return;
  }
  file = fopen(cnStr(&name), "w");
  cnStringDispose(&name);
  if (!file) {
    return;
  }

  // Print out the data.
  for (point = points; point < pointsEnd; point += size) {
    cnVectorPrint(file, size, point);
    fprintf(file, "\n");
  }

  // All done.
  fclose(file);
}


}
