{log} = console
{max, min} = Math


exports.learn = (tree) ->
  log "Learning ..."
  # TODO Better leaf picking.
  leaves = tree.leaves()
  leaf = leaves[0]
  expandLeaf leaf


# Mapping function helpers.
difference = (mapper) ->
  name = mapper.name
  map = mapper.map
  throw "#{name} arity #{map.length} != 1" if map.length != 1
  name: "difference#{name[0].toUpperCase()}#{name[1..]}"
  map: (a, b) -> vectorDiff(map(a), map(b))


# Mapping functions.
mappers = {}
putMapper = (mapper) ->
  mappers[mapper.name] = mapper
putMapper name: 'location', map: (entity) -> entity.location
putMapper difference mappers.location
# TODO Move mappers to a separate file, and just make the list here.
mappers = (mapper for name, mapper of mappers)
# Sort mappers by arity. We prefer simpler models.
mappers.sort (a, b) -> a.map.length - b.map.length


expandLeaf = (leaf) ->
  log "Expanding leaf #{leaf.id} ..."
  minArity = mappers[0].map.length
  maxArity = mappers[mappers.length - 1].map.length
  # Compare that to the number of vars available in this branch.
  varDepth = leaf.varDepth()
  minNewVarCount = max 0, minArity - varDepth
  log "Min new var count: #{minNewVarCount}"
  # Clone the tree, and start adding vars.
  # We prefer simpler trees, so the fewer nodes the better.
  clone = leaf.root().clone()
  node = clone.getNode leaf.id
  for addedVarCount in [minNewVarCount..maxArity]
    log "Adding up to var #{addedVarCount}"
    $var = clone.newVar()
    node.replaceWith $var
    node = $var.kid
    $var.parent.propagate()
    if false
      # Poor man's benchmark.
      for i in [0...1000]
        clone.propagate()
      bindingCount = 0
      bindingCount += binding.entityLists.length for binding in clone.leaves()[0].bindings
      log "Bindings at leaf after 1000 props: #{bindingCount}"
    log "New var #{$var.id}"
    log "New leaf #{node.id}"
    varDepth++
    for mapper in mappers
      # End our loop once we don't have enough vars.
      break if mapper.map.length > varDepth
      # And skip mappers for which we've added superfluous vars.
      continue if mapper.map.length < addedVarCount
      split node, mapper


getValueBags = (bindingBags, mapper, indexes) ->
  for binding in bindingBags
    values = []
    for entities in binding.entityLists
      entities = (entities[index] for index in indexes)
      continue if null in entities # error case
      value = mapper.map entities...
      values.push value
    values


split = (leaf, mapper) ->
  log "Splitting node #{leaf.id} on #{mapper.name} ..."
  # TODO Figure out which vars to use.
  # TODO Could figure out how many vars immediately above the current leaf.
  # TODO We should commit all those, but we could mix and match earliers.
  # TODO   committed = 0
  # TODO   node = leaf.parent
  # TODO   while node? and node.isVar()
  # TODO     committed++
  # TODO     node = node.parent
  # For now, just use the most recent vars.
  arity = mapper.map.length
  varDepth = leaf.varDepth()
  splitWithIndexes leaf, mapper, [varDepth-arity ... varDepth]


splitWithIndexes = (leaf, mapper, indexes) ->
  log "Using indexes #{indexes} ..."
  valueBags = getValueBags leaf.bindings, mapper, indexes
  # TODO Manual loops probably faster than list building here.
  mins = vectorMin (vectorMin valueBag for valueBag in valueBags)
  maxes = vectorMax (vectorMax valueBag for valueBag in valueBags)
  if true
    valueCount = 0
    valueCount += values.length for values in valueBags
    log "Built #{valueCount} values in #{valueBags.length} bags"
    log "Limits: #{mins} #{maxes}"
    #log values.join ' '


vectorDiff = (x, y) -> x[i] - y[i] for i in [0 ... x.length]


vectorMax = (vectors) ->
  if vectors.length and dim = vectors[0].length
    # This manual looping is substantially faster (3x?) than building a array
    # on the spot for each dim then using Math.max.
    # Slower version:
    # max (vectors[v][d] for v in [0...vectors.length])... for d in [0...dim]
    for d in [0...dim]
      maxVal = -Infinity
      for vector in vectors
        val = vector[d]
        if val > maxVal
          maxVal = val
      maxVal
  else
    []


vectorMin = (vectors) ->
  if vectors.length and dim = vectors[0].length
    # This manual looping is substantially faster (3x?) than building a array
    # on the spot for each dim then using Math.min.
    # Slower version:
    # min (vectors[v][d] for v in [0...vectors.length])... for d in [0...dim]
    for d in [0...dim]
      minVal = Infinity
      for vector in vectors
        val = vector[d]
        if val < minVal
          minVal = val
      minVal
  else
    []
