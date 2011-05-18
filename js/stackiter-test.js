// Imports.

var log = console.log;
var learn = require('./learn').learn;
var abs = Math.abs, sqrt = Math.sqrt;
var load = require('./stackiter-loader').load;
var emptyBindings, startTree; (function() {
  emptyBindings = this.emptyBindings;
  startTree = this.startTree;
}).apply(require('./tree'));
//{startProfiling, stopProfiling} = require 'v8-profiler'


go();


// Functions.


function chooseDropWhereLandOnOther(states) {
  // TODO Automate empty constructor call for non-null!
  var samples = [];
  var formerHadGrasp = false;
  var graspedId = -1;
  var ungraspState = null;
  for (var s = 0; s < states.length; s++) {
    var state = states[s];
    if (ungraspState) {
      // Item released. We're now trying to find when it settles.
      var settled = false;
      var label = null;
      if (state.cleared) {
        // World cleared. Say it's settled, but don't assign a label.
        settled = true;
      } else {
        var item = findItem(state, graspedId);
        if (item) {
          // Still here. See if it's moving.
          if (norm(item.velocity) < 0.01) {
            // Stopped. See where it is.
            // TODO If the block bounced up, we might be catching it at the
            // TODO peak. Maybe we should check for that (by accel or
            // TODO location?).
            settled = true;
            label = !onGround(item);
          }
        } else {
          // It fell off the table, so it didn't land on another block.
          settled = true;
          label = false;
        }
      }
      if (settled) {
        ungraspState = null;
        if (label != null) {
          // Labeled, so record it.
          var entities = [];
          var items = state.items;
          for (var i = 0; i < items.length; i++) {
            var item = items[i];
            if (item.alive) {
              entities.push(item);
            }
          }
          samples.push({entities: entities, label: label});
        }
      }
    } else {
      var graspedItems = findGraspedItems(state);
      var hasGrasp = Boolean(graspedItems.length);
      if (hasGrasp) {
        // TODO Support multiple grasped items intelligently?
        graspedId = graspedItems[0].id;
      } else if (formerHadGrasp) {
        ungraspState = state;
      }
      formerHadGrasp = hasGrasp;
    }
  }
  return samples;
}


function findGraspedItems(state) {
  var graspedItems = [];
  var items = state.items;
  for (var i = 0; i < items.length; i++) {
    var item = items[i];
    if (item.grasped) {
      graspedItems.push(item);
    }
  }
  return graspedItems;
}


function findItem(state, id) {
  var items = state.items;
  for (var i = 0; i < items.length; i++) {
    var item = items[i];
    if (item.id == id) {
      return item;
    }
  }
  return null;
}


function go() {
  // TODO Determine project root or let file be passed in.
  //startProfiling();
  load('temp/stackiter-20101105-171808-671_drop-from-25.log', {
    stateLimit: 1000,
    ready: function(states) {
      report(states);
      var samples = chooseDropWhereLandOnOther(states);
      var trueCount = 0;
      for (var s = 0; s < samples.length; s++) {
        trueCount += Number(samples[s].label);
      }
      log(trueCount + " true of " + samples.length + " samples");
      var tree = startTree();
      tree.propagate(emptyBindings(samples));
      var leaf = tree.leaves()[0];
      log(
        "Leaf with " + leaf.prob + " prob and " +
        leaf.bindings.length + " bindings"
      );
      // TODO Or just 'learn(tree);' for simplicity?
      tree = learn(tree);
      leaf = tree.leaves()[0];
      log(
        "Leaf with " + leaf.prob + " prob and " +
        leaf.bindings.length + " bindings"
      );
      // stopProfiling();
      // debugger;
    }
  });
}


function norm(vector) {
  var sum = 0;
  for (var v = 0; v < vector.length; v++) {
    var val = vector[v];
    sum += val * val;
  }
  return sqrt(sum);
}


function onGround(item) {
  var angle = item.orientation;
  // Angles go from -1 to 1.
  // Here, dim 1 is upright, and 0 sideways.
  var dim =
    Number(angle < -0.75 || (-0.25 < angle && angle < 0.25) || 0.75 < angle);
  return abs(item.extent[dim] - item.location[1]) < 0.01;
}


function report(states) {
  log("At end:");
  log(states[states.length - 1].items.length + " items");
  //log(states[102]);
  //log(states[102].items[9].extent);
  //log(states[102].items.length + " here");
  //log(states[700].items.length + " there");
  log(states.length + " states");
}
