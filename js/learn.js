// Imports.


var Gaussian = require('./pdf').Gaussian;
var dump = console.log;
var max = Math.max, min = Math.min;


// Exports.


exports.learn = learn;


// Functions.


// Mapping function helpers.

function difference(mapper) {
  var name = mapper.name;
  var map = mapper.map;
  if (map.length != 1) throw name + " arity " + map.length + " != 1";
  return {
    name: "difference" + name[0].toUpperCase() + name.substring(1),
    map: function(a, b) {return vectorDiff(map(a), map(b))},
    pdfType: mapper.pdfType
  };
}


// Mapping functions.
var mappers = [];
(function() {
  var mappersObj = {};
  function putMapper(mapper) {
    mappersObj[mapper.name] = mapper;
  }
  putMapper({
    name: 'location',
    map: function(entity) {return entity.location},
    pdfType: Gaussian
  });
  putMapper(difference(mappersObj.location));
  // TODO Move mappers to a separate file, and just make the list here.
  for (var m in mappersObj) {
    mappers.push(mappersObj[m]);
  }
  // Sort mappers by arity. We prefer simpler models.
  mappers.sort(function(a, b) {return a.map.length - b.map.length});
})();


function expandLeaf(leaf) {
  // 1. Choose function.
  // 2. Initial kernel.
  // 3. DD for initial guesses.
  // 4. Weighted MLE center and shape.
  // 5. KS (or other?) Threshold.
  // 6. Leaf probs.
  dump("Expanding leaf " + leaf.id + " ...");
  var minArity = mappers[0].map.length;
  var maxArity = mappers[mappers.length - 1].map.length;
  // Compare that to the number of vars available in this branch.
  var varDepth = leaf.varDepth();
  var minNewVarCount = max(0, minArity - varDepth);
  dump("Min new var count: " + minNewVarCount);
  // Clone the tree, and start adding vars.
  // We prefer simpler trees, so the fewer nodes the better.
  var clone = leaf.root().clone();
  var node = clone.getNode(leaf.id);
  for (
    var addedVarCount = minNewVarCount;
    addedVarCount <= maxArity;
    addedVarCount++
  ) {
    dump("Adding up to var " + addedVarCount);
    var $var = clone.newVar();
    node.replaceWith($var);
    node = $var.kid;
    $var.parent.propagate();
    if (false) {
      // Poor man's benchmark.
      for (var i = 0; i < 1000; i++) {
        clone.propagate();
      }
      var bindingCount = 0;
      var bindings = clone.leaves()[0].bindings;
      for (var b = 0; b < bindings.length; b++) {
        bindingCount += bindings[b].entityLists.length;
      }
      dump("Bindings at leaf after 1000 props: " + bindingCount);
    }
    dump("New var " + $var.id);
    dump("New leaf " + node.id);
    varDepth++;
    for (var m = 0; m < mappers.length; m++) {
      var mapper = mappers[m];
      // End our loop once we don't have enough vars.
      if (mapper.map.length > varDepth) break;
      // And skip mappers for which we've added superfluous vars.
      if (mapper.map.length < addedVarCount) continue;
      split(node, mapper);
    }
  }
  // TODO Return the clone that scored highest, if any.
  return clone;
}


function getValueBags(bindingBags, mapper, indices) {
  // TODO Provide the error bindings and bags immediately here, too?
  // TODO Or reloop later to pull those out?
  var bags = [];
  for (var b = 0; b < bindingBags.length; b++) {
    var binding = bindingBags[b];
    var values = [];
    var entityLists = binding.entityLists;
    for (var e = 0; e < entityLists.length; e++) {
      var entitiesAll = entityLists[e];
      var entities = [];
      for (var i = 0; i < indices.length; i++) {
        entities.push(entitiesAll[indices[i]]);
      }
      if (entities.indexOf(null) >= 0) continue; // error case
      values.push(mapper.map.apply(undefined, entities));
    }
    if (!values.length) continue; // no actual values
    bags.push({bag: binding.bag, values: values});
  }
  return bags;
}


function split(leaf, mapper) {
  dump("Splitting node " + leaf.id + " on " + mapper.name + " ...");
  // TODO Figure out which vars to use.
  // TODO Could figure out how many vars immediately above the current leaf.
  // TODO We should commit all those, but we could mix and match earliers.
  // TODO   committed = 0
  // TODO   node = leaf.parent
  // TODO   while node? and node.isVar()
  // TODO     committed++
  // TODO     node = node.parent
  // For now, just use the most recent vars.
  var arity = mapper.map.length;
  var varDepth = leaf.varDepth();
  var indices = [];
  for (var index = varDepth - arity; index < varDepth; index++) {
    indices.push(index);
  }
  splitWithIndexes(leaf, mapper, indices);
}


function learn(tree) {
  dump("Learning ...");
  // TODO Better leaf picking.
  var leaves = tree.leaves();
  var leaf = leaves[0];
  tree = expandLeaf(leaf);
  updateLeafProbs(tree);
  return tree;
}


function splitWithIndexes(leaf, mapper, indices) {
  // Create split.
  var clone = leaf.root().clone();
  var node = clone.getNode(leaf.id);
  var split = clone.newSplit();
  node.replaceWith(split);
  split.parent.propagate();
  split.setMapping(mapper, indices);
  // Update/optimize the question.
  updateModel(split);
}


function updateLeafProbs(root) {
  var leaves = root.leaves();
  // TODO 1. Find the leaf with the highest prob.
  // TODO 2. Remove all the bags in that leaf from others.
  // TODO 2a. Actually need a separate list of bags to avoid nixing bindings.
  // TODO 3. Repeat.
  for (var L = 0; L < leaves.length; L++) {
    var leaf = leaves[L];
    var bindingBags = leaf.bindings;
    var sumPos = 0;
    for (var b = 0; b < bindingBags.length; b++) {
      sumPos += Number(bindingBags[b].bag.label);
    }
    leaf.prob = sumPos / bindingBags.length;
  }
}


function updateModel(split) {
  var indices = split.indices;
  var mapper = split.mapper;
  dump("Using indices " + indices + " ...");
  var initBagLimit = 4;
  var posCount = 0;
  var valueBags = getValueBags(split.bindings, mapper, indices);
  for (var b = 0; b < valueBags.length; b++) {
    var valueBag = valueBags[b];
    if (valueBag.bag.label) {
      if (++posCount > initBagLimit) break;
      var values = valueBag.values;
      for (var v = 0; v < values.length; v++) {
        // TODO Calculate kernel or whatnot against all other bags.
        var value = values[v];
      }
    }
  }
  // TODO Manual loops probably faster than list building here.
  if (true) {
    var valueCount = 0;
    var minsInner = [];
    var maxesInner = [];
    for (var b = 0; b < valueBags.length; b++) {
      var valueBag = valueBags[b];
      valueCount += valueBag.values.length;
      minsInner.push(vectorMin(valueBag.values));
      maxesInner.push(vectorMax(valueBag.values));
    }
    var mins = vectorMin(minsInner);
    var maxes = vectorMax(maxesInner);
    dump("Built " + valueCount + " values in " + valueBags.length + " bags");
    dump("Limits: " + mins + " " + maxes);
    //dump(values.join(' '));
  }
}


function vectorDiff(x, y) {
  var diff = Array(x.length);
  for (var i = 0; i < diff.length; i++) {
    diff[i] = x[i] - y[i];
  }
  return diff;
}


function vectorMax(vectors) {
  var dim;
  if (vectors.length && (dim = vectors[0].length)) {
    var maxVals = Array(dim);
    for (var d = 0; d < dim; d++) {
      var maxVal = -Infinity;
      for (var v = 0; v < vectors.length; v++) {
        var vector = vectors[v];
        var val = vector[d];
        if (val > maxVal) {
          maxVal = val;
        }
      }
      maxVals[d] = maxVal;
    }
    return maxVals;
  } else {
    return [];
  }
}


function vectorMin(vectors) {
  var dim;
  if (vectors.length && (dim = vectors[0].length)) {
    var minVals = Array(dim);
    for (var d = 0; d < dim; d++) {
      var minVal = Infinity;
      for (var v = 0; v < vectors.length; v++) {
        var vector = vectors[v];
        var val = vector[d];
        if (val < minVal) {
          minVal = val;
        }
      }
      minVals[d] = minVal;
    }
    return minVals;
  } else {
    return [];
  }
}
