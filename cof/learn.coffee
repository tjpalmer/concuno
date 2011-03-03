{log} = console

exports.learn = (tree) ->
  log "Learning ..."
  # TODO Better leaf picking.
  leaves = tree.leaves()
  leaf = leaves[0]
  expandLeaf leaf

# Mapping functions.
difference = (getter) -> (a, b) -> getter(a) - getter(b)
identityLocation = (entity) -> entity.location
differenceLocation = difference identityLocation

expandLeaf = (leaf) ->
  log "Expanding leaf ..."
  # TODO Any standard way to get arity automatically?
  # TODO If not, improve the organization of arity here, at least.
  mappers = {identityLocation: 1, differenceLocation: 2}
  arities = (arity for mapper, arity of mappers)
  arities.sort()
  minArity = arities[0]
  maxArity = arities[arities.length - 1]
  log arities
  # TODO varDepth vs minArity
