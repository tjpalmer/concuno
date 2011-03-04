log = console.log


exports.emptyBindings = (bags) -> new Binding bag for bag in bags


exports.startTree = ->
  new RootNode


class Binding

  constructor: (@bag) ->
    @entities = []


class Node

  clone: ->
    node = new @constructor (kid.clone() for kid in @kids())...
    # By default, we get empty bindings.
    # We could leave them empty, or we could clone the array.
    # Instead default to using the same binds in the clone as the current.
    # Do this because we clone a lot, and it would be nice not to have lots of
    # big array copies going on.
    # TODO This is still wasteful creating lots of empty arrays.
    # TODO Consider how to avoid that.
    node.bindings = @bindings
    node

  constructor: ->
    @bindings = []
    @parent = null
    kid.parent = this for kid in @kids()

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


class SplitNode extends Node

  constructor:
    (@$true = new LeafNode, @$false = new LeafNode, @error = new LeafNode) ->
      super()

  kids: -> [@$true, @$false, @error]


class VarNode extends Node

  constructor: (@kid = new LeafNode) ->
    super()

  kids: -> [@kid]
