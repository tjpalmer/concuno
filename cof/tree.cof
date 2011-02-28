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


class LeafNode extends Node

  constructor: (@prob = 0) ->
    super()

  propagate: (@bindings) -> # That's all?


class RootNode extends Node

  constructor: (@kid = new LeafNode) ->
    super()
    @bags = []

  propagate: (@bindings) ->
    @kid.propagate @bindings


class SplitNode extends Node

  constructor:
    (@$true = new LeafNode, @$false = new LeafNode, @error = new LeafNode) ->
      super()


class VarNode extends Node

  constructor: (@kid = new LeafNode) ->
    super()
