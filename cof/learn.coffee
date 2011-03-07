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
  mappers = [identityLocation, differenceLocation]
  arities = (mapper.length for mapper in mappers)
  arities.sort()
  minArity = arities[0]
  maxArity = arities[arities.length - 1]
  log "Arities: #{arities.join ', '}"
  varDepth = leaf.varDepth()
  minNewVarCount = max 0, minArity - varDepth
  log "Min new var count: #{minNewVarCount}"
  node = leaf
  for v in [minNewVarCount..maxArity]
    log "Adding up to var #{v}"
    clone = node.root().clone()
    clonedNode = clone.getNode node.id
    log "Cloned leaf #{clonedNode.id}"
    # TODO Add var node.
