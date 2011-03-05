log = console.log


exports.emptyBindings = (bags) -> new Binding bag for bag in bags


exports.startTree = ->
  new RootNode


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
    @parent = null
    setKidsParent this

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


class LeafNode extends Node

  clone: ->
    node = super()
    node.prob = @prob
    node

  constructor: ->
    super()
    @prob = 0

  kids: -> []

  propagate: (@bindings) -> # That's all?


class RootNode extends Node

  constructor: (@kid = new LeafNode) ->
    super()
    @bags = []

  kids: -> [@kid]

  propagate: (@bindings) ->
    @kid.propagate @bindings

  setKids: (kids) ->
    [@kid] = kids
    setKidsParent this


class SplitNode extends Node

  constructor:
    (@$true = new LeafNode, @$false = new LeafNode, @error = new LeafNode) ->
      super()

  kids: -> [@$true, @$false, @error]

  setKids: (kids) ->
    [@$true, @$false, @error] = kids
    setKidsParent this


class VarNode extends Node

  constructor: (@kid = new LeafNode) ->
    super()

  kids: -> [@kid]

  setKids: (kids) ->
    [@kid] = kids
    setKidsParent this


setKidsParent = (parent) ->
    kid.parent = parent for kid in parent.kids()
