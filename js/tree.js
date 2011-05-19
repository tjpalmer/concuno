// Imports.


var $class = require('./etc').$class;
var dump = console.log;


// Exports.


exports.emptyBindings = emptyBindings;
exports.startTree = startTree;


// Classes.


function BindingBag(bag) {
  this.bag = bag;
  this.entityLists = [];
}


function Node() {

  this.bindings = [];
  this.id = 0;
  this.parent = null;
  initKids(this);

} $class(Node, null, function() {

  this.clone = function() {
    // Mostly, we're okay keeping things just as prototypes. A bit risky if we
    // don't track the consequences (such as for bindings lists), but we make
    // a lot of clones, and avoiding copying data seems wise and simple.
    var Clone = function() {};
    Clone.prototype = this;
    var node = new Clone;
    var kids = this.kids();
    if (kids.length) {
      // We do want to copy the kids.
      var clonedKids = [];
      for (var k = 0; k < kids.length; k++) {
        clonedKids.push(kids[k]);
      }
      node.setKids(clonedKids);
    }
    return node;
  };

  this.getNode = function(id) {
    if (this.id == id) {
      return this;
    } else {
      var kids = this.kids();
      for (var k = 0; k < kids.length; k++) {
        var node = kids[k].getNode(id);
        if (node) {
          return node;
        }
      }
      return null;
    }
  };

  this.isVar = function() {
    return false;
  };

  this.leaves = function() {
    var result = [];
    pushLeaves(this);
    return result;
    function pushLeaves(node) {
      var kids = node.kids();
      if (kids.length) {
        for (var k = 0; k < kids.length; k++) {
          pushLeaves(kids[k]);
        }
      } else {
        // This is a leaf.
        result.push(node);
      }
    }
  }

  this.newLeaf = function(kid) {
    return new LeafNode(kid);
  };

  this.newSplit = function(kid) {
    return new SplitNode(kid);
  };

  this.newVar = function(kid) {
    return new VarNode(kid);
  };

  this.replaceWith = function(node) {
    if (this.parent) {
      var kids = this.parent.kids();
      for (var k = 0; k < kids.length; k++) {
        var kid = kids[k];
        if (kid.id == this.id) {
          this.parent.setKid(Number(k), node);
        }
      }
    }
  };

  this.varDepth = function() {
    var depth = 0;
    var node = this;
    while (node) {
      depth += node.isVar();
      node = node.parent;
    }
    return depth;
  };

  this.root = function() {
    return this.parent ? this.parent.root() : this;
  }

});



function LeafNode() {

  Node.call(this);
  this.prob = 0;

} $class(LeafNode, Node, function() {

  this.kids = function() {
    return [];
  };

  this.propagate = function(bindings) {
    if (bindings) {
      // We got new bindings.
      this.bindings = bindings;
    }
  };

});


function RootNode(kid) {

  this.kid = kid || new LeafNode;
  // Set nextId before calling super, so kids will init correctly.
  this.nextId = 1;
  Node.call(this);
  this.bags = [];

} $class(RootNode, Node, function() {

  this.kids = function() {
    return [this.kid];
  };

  this.generateId = function() {
    return this.nextId++;
  };

  this.propagate = function(bindings) {
    if (bindings) {
      // We got new bindings.
      this.bindings = bindings;
    } else {
      // We need to use the ones we already had.
      bindings = this.bindings;
    }
    // dump("RootNode to prop " + bindings.length + " bindings");
    this.kid.propagate(bindings);
  };

  this.setKid = function(k, kid) {
    if (k) throw "bad kid index " + k;
    this.kid = kid;
    initKids(this);
  };

  this.setKids = function(kids) {
    this.kid = kids[0];
    initKids(this);
  };

});


function SplitNode($true, $false, error) {

  this.$true = $true || new LeafNode;
  this.$false = $false || new LeafNode;
  this.error = error || new LeafNode;
  Node.call(this);

} $class(SplitNode, Node, function() {

  this.kids = function() {
    return [this.$true, this.false, this.error];
  };

  this.setKid = function(k, kid) {
    var keys = ['$true', '$false', 'error'];
    this[keys[k]] = kid;
    initKids(this);
  };

  this.setKids = function(kids) {
    this.$true = kids[0];
    this.$false = kids[1];
    this.error = kids[2];
    initKids(this);
  };

});



function VarNode(kid) {

  this.kid = kid || new LeafNode;
  Node.call(this);

} $class(VarNode, Node, function() {

  this.isVar = function() {
    return true;
  };

  this.kids = function() {
    return [this.kid];
  };

  this.propagate = function(bindings) {

    if (bindings) {
      // We got new bindings.
      var outgoings = [];
      var inCount = 0;
      var outCount = 0;
      for (var b = 0; b < bindings.length; b++) {
        var binding = bindings[b];
        var bag = binding.bag;
        var outgoing = new BindingBag(bag);
        for (var L = 0; L < binding.entityLists.length; L++) {
          var entities = binding.entityLists[L];
          inCount++;
          var any = false;
          for (var e = 0; e < bag.entities.length; e++) {
            var entity = bag.entities[e];
            if (entities.indexOf(entity) < 0) {
              // We don't repeat bindings in SMRF.
              bind(entity);
              any = true;
            }
          }
          if (!any) {
            // Dummy entity here for when no new entities were available.
            // We send these down error branches later at split nodes.
            bind(null);
          }
        }
        outgoings.push(outgoing);
      }
      this.bindings = outgoings;
      //dump("Built " + inCount + " bindings into " + outCount);
    }

    // Continue propagation.
    this.kid.propagate(this.bindings);

    // Functions.
    function bind(entity) {
      var outEntities = entities.slice();
      outEntities.push(entity);
      outgoing.entityLists.push(outEntities);
      outCount++;
    }

  };

  this.setKid = function(k, kid) {
    if (k) throw "bad kid index " + k;
    this.kid = kid;
    initKids(this);
  };

  this.setKids = function(kids) {
    this.kid = kids[0];
    initKids(this);
  };

});


// Functions.


function emptyBindings(bags) {
  var bindings = [];
  for (var b = 0; b < bags.length; b++) {
    var binding = new BindingBag(bags[b]);
    // We start with one empty binding per bag.
    binding.entityLists.push([]);
    bindings.push(binding);
  }
  return bindings;
}


function initKids(parent) {
  var kids = parent.kids();
  // Set parent.
  for (var k = 0; k < kids.length; k++) {
    kids[k].parent = parent;
  }
  // Set ids if we're part of a tree already.
  var root = parent.root();
  if (root instanceof RootNode) {
    for (var k = 0; k < kids.length; k++) {
      var kid = kids[k];
      if (!kid.id) {
        kid.id = root.generateId();
        initKids(kid);
      }
    }
  }
}


function startTree() {
  return new RootNode;
}
