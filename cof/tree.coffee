log = console.log


exports.emptyBindings = (bags) -> new Binding bag for bag in bags


exports.startTree = ->
  new RootNode


class Binding

  constructor: (@bag) ->
    @entities = []


class Node

  constructor: ->
    @bindings = []

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


class LeafNode extends Node

  constructor: (@prob = 0) ->
    super()

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
