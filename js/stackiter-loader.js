// Imports.

var log = console.log;
var readFile = require('fs').readFile;


// Exports.
// TODO Auto-export.

exports.load = load;


// Functions.


// TODO Move clone out to some utility area.
// Cloning technique based on examples and discussions here:
// http://stackoverflow.com/questions/122102/
//   what-is-the-most-efficient-way-to-clone-a-javascript-object
function clone(obj) {
  // This could be more sophisticated about more types, but eh.
  if (obj == null || typeof(obj) != 'object') {
    return obj;
  }
  var copied = new obj.constructor;
  for (var key in obj) {
    var val = obj[key];
    copied[key] = clone(val);
  }
  return copied;
}


// TODO Options schema.
function load(file, options) {
  readFile(file, 'utf8', function(err, content) {
    if (err) throw err;
    log("Parsing " + file + " ...");
    var states = new Loader().parseAll(content.split(/\n/), options.stateLimit);
    if (options.ready) {
      options.ready(states);
    }
  });
}


// TODO Change this to a class??? Or leave it as an alternative?
function Loader() {

  var indexes = [];
  var state = {cleared: false, items: [], time: 0};
  var states = [];

  var parsers = {
    alive: parseAlive,
    clear: parseClear,
    destroy: parseDestroy,
    extent: parseExtent,
    grasp: parseGrasp,
    item: parseItem,
    pos: parsePos,
    posvel: parsePosVel,
    release: parseRelease,
    rot: parseRot,
    time: parseTime,
    type: parseType,
  };

  this.parseAll = parseAll;

  function findIndex(args, argIndex) {
    if (argIndex === undefined) argIndex = 1;
    var id = Number(args[argIndex]);
    var index = indexes[id];
    if (index == null) throw "no such item";
    return index;
  }

  function findItem(args, argIndex) {
    if (argIndex == null) argIndex = 1;
    return state.items[findIndex(args, argIndex)];
  }

  function parseAlive(args) {
    var item = findItem(args);
    item.alive = args[2] === 'true';
  }

  function parseAll(lines, stateLimit) {
    if (stateLimit === undefined) stateLimit = Infinity;
    for (var l = 0; l < lines.length; l++) {
      var line = lines[l];
      parseLine(line);
      if (states.length >= stateLimit) {
        break;
      }
    }
    if (states.length < stateLimit) {
      states.push(clone(state));
    }
    return states;
  }

  function parseClear(args) {
    state.cleared = true;
  }

  function parseDestroy(args) {
    var index = findIndex(args);
    // Remove the item and update the indexes.
    state.items.splice(index, 1);
    for (var i = index; i < state.items.length; i++) {
      indexes[state.items[i].id]--;
    }
  }

  function parseExtent(args) {
    var item = findItem(args);
    item.extent[0] = Number(args[2]);
    item.extent[1] = Number(args[3]);
  }

  function parseGrasp(args) {
    var tool = findItem(args);
    var item = findItem(args, 2);
    tool.grasping = true;
    item.grasped = true;
  }

  function parseItem(args) {
    var id = Number(args[1]);
    indexes[id] = state.items.length;
    state.items.push({
      alive: false,
      color: [0, 0, 0],
      extent: [0, 0],
      grasped: false,
      grasping: false,
      id: id,
      location: [0, 0],
      orientation: 0,
      type: null,
      velocity: [0, 0],
    });
  }

  function parseLine(line) {
    var args = line.trim().split(/\s+/);
    var parser = parsers[args[0]];
    if (parser) {
      parser(args);
    }
  }

  function parsePos(args) {
    item = findItem(args);
    item.location[0] = Number(args[2]);
    item.location[1] = Number(args[3]);
  }

  function parsePosVel(args) {
    // TODO Unify these that are similar form?
    item = findItem(args);
    item.velocity[0] = Number(args[2]);
    item.velocity[1] = Number(args[3]);
  }

  function parseRelease(args) {
    tool = findItem(args);
    item = findItem(args, 2);
    tool.grasping = false;
    item.grasped = false;
  }

  function parseRot(args) {
    var item = findItem(args);
    // TODO Angle is rats. Convert to radians or not?
    item.orientation = Number(args[2]);
  }

  function parseTime(args) {
    if (args[1] == 'sim') {
      states.push(clone(state));
      state.cleared = false;
      state.time = Number(args[3]);
    }
  }

  function parseType(args) {
    var item = findItem(args);
    item.type = args[2];
  }

};
