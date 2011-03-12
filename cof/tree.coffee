log = console.log


exports.emptyBindings = (bags) -> new Binding bag for bag in bags


exports.startTree = -> new RootNode


class Binding

  constructor: (@bag) ->
    @entities = []

class Bindings

  constructor: (@formers, @indexes, @entities) ->
    if not @entities?
      @indexes = [0...@formers.length]
      @entities = []
    @length = @entities.length
    @depth = if @formers.depth? then @formers.depth + 1 else 0

  getEntities: (bindingIndex, varIndexes) ->
    entities = []
    bindings = this
    v = varIndexes.length - 1
    varIndex = varIndexes[v]
    for depth in [@depth..1]
      if varIndex is depth - 1
        entities.push bindings.entities[bindingIndex]
        varIndex = varIndexes[--v]
      bindings = bindings.formers[bindingIndex]
    entities

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

  isVar: -> false

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

  newLeaf: (kid) -> new LeadNode kid

  newSplit: (kid) -> new SplitNode kid

  newVar: (kid) -> new VarNode kid

  replaceWith: (node) ->
    if @parent?
      for k, kid of @parent.kids()
        if kid.id is @id
          @parent.setKid +k, node
    undefined

  varDepth: ->
    depth = 0
    node = this
    while node?
      depth += node.isVar()
      node = node.parent
    depth

  root: -> if @parent? then @parent.root() else this


class LeafNode extends Node

  constructor: ->
    super()
    @prob = 0

  kids: -> []

  propagate: (bindings) ->
    if bindings?
      # We got new bindings.
      @bindings = bindings


class RootNode extends Node

  constructor: (@kid = new LeafNode) ->
    # Set nextId before calling super, so kids will init correctly.
    @nextId = 1;
    super()
    @bags = []

  kids: -> [@kid]

  generateId: -> @nextId++

  propagate: (bindings) ->
    if bindings?
      # We got new bindings.
      @bindings = bindings
    else
      # We need to use the ones we already had.
      bindings = @bindings
    #log "RootNode to prop #{bindings.length} bindings"
    @kid.propagate bindings

  setKid: (k, kid) ->
    throw "bad kid index #{k}" if k
    @kid = kid
    initKids this

  setKids: (kids) ->
    [@kid] = kids
    initKids this


class SplitNode extends Node

  constructor:
    (@$true = new LeafNode, @$false = new LeafNode, @error = new LeafNode) ->
      super()

  kids: -> [@$true, @$false, @error]

  setKid: (k, kid) ->
    keys = ['$true', '$false', 'error']
    this[keys[k]] = kid
    initKids this

  setKids: (kids) ->
    [@$true, @$false, @error] = kids
    initKids this


class VarNode extends Node

  constructor: (@kid = new LeafNode) ->
    super()

  isVar: -> true

  kids: -> [@kid]

  propagate: (bindings) ->
    if bindings?
      # We got new bindings.
      outgoings = []
      for binding in bindings
        bind = (entity) ->
          outEntities = binding.entities.slice()
          outEntities.push entity
          outgoings.push {bag: binding.bag, entities: outEntities}
        any = false
        for entity in binding.bag.entities
          if entity not in binding.entities
            # We don't repeat bindings in SMRF.
            bind entity
            any = true
        if not any
          # Dummy entity here for when no new entities were available.
          bind null
      # TODO Need to bind some things first, eh?
      @bindings = outgoings
      #log "Build #{bindings.length} bindings into #{outgoings.length}"
    @kid.propagate @bindings

  setKid: (k, kid) ->
    throw "bad kid index #{k}" if k
    @kid = kid
    initKids this

  setKids: (kids) ->
    [@kid] = kids
    initKids this


initKids = (parent) ->
  kids = parent.kids()
  # Set parent.
  kid.parent = parent for kid in kids
  # Set ids if we're part of a tree already.
  root = parent.root()
  if root instanceof RootNode
    for kid in kids
      if not kid.id
        kid.id = root.generateId()
        initKids kid
  undefined
