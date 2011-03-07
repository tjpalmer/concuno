log = console.log


exports.emptyBindings = (bags) -> new Binding bag for bag in bags


exports.startTree = -> new RootNode


class Binding

  constructor: (@bag) ->
    @entities = []


class Node

  clone: ->
    # Mostly, we're okay keeping things just as prototypes. A bit risky if we
    # don't track the consequences (such as for bindings lists), but we make
    # a lot of clones, and avoiding copying data seems wise and simple.
    Clone = ->
    Clone.prototype = this
    node = new Clone
    kids = @kids()
    if kids.length
      # We do want to copy the kids.
      node.setKids(kid.clone() for kid in kids)
    node

  constructor: ->
    @bindings = []
    @id = 0
    @parent = null
    initKids this

  getNode: (id) ->
    if @id is id
      this
    else
      for kid in @kids()
        node = kid.getNode id
        return node if node?
      null

  leaves: ->
    result = []
    pushLeaves = (node) ->
      kids = node.kids()
      if kids.length
        for kid in kids
          pushLeaves kid
      else
        # This is a leaf.
        result.push node
    pushLeaves this
    result

  varDepth: ->
    depth = 0
    node = this
    while node?
      depth += node.constructor is VarNode
      node = node.parent
    depth

  root: -> if @parent? then @parent.root() else this


class LeafNode extends Node

  constructor: ->
    super()
    @prob = 0

  kids: -> []

  propagate: (@bindings) -> # That's all?


class RootNode extends Node

  constructor: (@kid = new LeafNode) ->
    # Set nextId before calling super, so kids will init correctly.
    @nextId = 1;
    super()
    @bags = []

  kids: -> [@kid]

  generateId: -> @nextId++

  propagate: (@bindings) ->
    @kid.propagate @bindings

  setKids: (kids) ->
    [@kid] = kids
    initKids this


class SplitNode extends Node

  constructor:
    (@$true = new LeafNode, @$false = new LeafNode, @error = new LeafNode) ->
      super()

  kids: -> [@$true, @$false, @error]

  setKids: (kids) ->
    [@$true, @$false, @error] = kids
    initKids this


class VarNode extends Node

  constructor: (@kid = new LeafNode) ->
    super()

  kids: -> [@kid]

  setKids: (kids) ->
    [@kid] = kids
    initKids this


initKids = (parent) ->
  kids = parent.kids()
  # Set parent.
  kid.parent = parent for kid in kids
  # Set ids.
  root = parent.root()
  for kid in kids
    if not kid.id
      kid.id = root.generateId()
  undefined
