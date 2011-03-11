{log} = console
{max} = Math


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


expandLeaf = (leaf) ->
  log "Expanding leaf #{leaf.id} ..."
  # Sort mappers by arity. We prefer simpler models.
  mappers = (mapper for name, mapper of mappers)
  mappers.sort (a, b) -> a.map.length - b.map.length
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
    log "New var #{$var.id}"
    log "New leaf #{node.id}"
    varDepth++
    for mapper in mappers
      # End our loop once we don't have enough vars.
      break if mapper.map.length > varDepth
      # And skip mappers for which we've added superfluous vars.
      continue if mapper.map.length < addedVarCount
      split node, mapper


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
  # TODO Split off the error cases immediately. No need to hang onto them.
  # TODO It's only the true vs. false we need to determine.
  values = []
  # TODO Bindings by bag, all the way down.
  for binding in leaf.bindings
    entities = binding.entities
    entities = (entities[index] for index in indexes)
    value = mapper.map entities...
    values.push value
  #log values.join ' '


vectorDiff = (x, y) -> x[i] - y[i] for i in [0 ... x.length]
