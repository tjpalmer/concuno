{log} = console
{max} = Math

exports.learn = (tree) ->
  log "Learning ..."
  # TODO Better leaf picking.
  leaves = tree.leaves()
  leaf = leaves[0]
  expandLeaf leaf

# Mapping function helpers.
difference = (getter) -> (a, b) -> getter(a) - getter(b)

# Mapping functions.
identityLocation = (entity) -> entity.location
differenceLocation = difference identityLocation

expandLeaf = (leaf) ->
  log "Expanding leaf #{leaf.id} ..."
  # Sort mappers by arity. We prefer simpler models.
  mappers = [identityLocation, differenceLocation]
  mappers.sort (a, b) -> a.length - b.length
  minArity = mappers[0].length
  maxArity = mappers[mappers.length - 1].length
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
    log "New var #{$var.id}"
    log "New leaf #{node.id}"
    varDepth++
    for mapper in mappers
      # End our loop once we don't have enough vars.
      break if mapper.length > varDepth
      # And skip mappers for which we've added superfluous vars.
      continue if mapper.length < addedVarCount
      log "Trying #{mapper}"
