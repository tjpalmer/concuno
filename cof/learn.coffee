{log} = console
{max} = Math

exports.learn = (tree) ->
  log "Learning ..."
  # TODO Better leaf picking.
  other = tree.clone()
  log other is tree
  log tree.leaves().length
  log other.leaves().length
  log tree.leaves()[0].bindings.length
  log other.leaves()[0].bindings.length
  log tree.leaves()[0] is other.leaves()[0]
  log tree.leaves()[0].bindings is other.leaves()[0].bindings
  log tree.leaves()[0].parent.constructor
  log other.leaves()[0].parent.constructor
  log tree.leaves()[0].parent is other.leaves()[0].parent
  leaves = tree.leaves()
  leaf = leaves[0]
  expandLeaf leaf

# Mapping function helpers.
difference = (getter) -> (a, b) -> getter(a) - getter(b)

# Mapping functions.
identityLocation = (entity) -> entity.location
differenceLocation = difference identityLocation

expandLeaf = (leaf) ->
  log "Expanding leaf ..."
  mappers = [identityLocation, differenceLocation]
  arities = (mapper.length for mapper in mappers)
  arities.sort()
  minArity = arities[0]
  maxArity = arities[arities.length - 1]
  log "Arities: #{arities.join ', '}"
  minNewVarCount = max 0, minArity - leaf.varDepth()
  log "Min new var count: #{minNewVarCount}"
  # TODO Clone tree.
  # TODO Add var nodes.
