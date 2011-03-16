{log} = console
{learn} = require './learn'
{abs, sqrt} = Math
{load} = require './stackiter-loader'
{emptyBindings, startTree} = require './tree'
#{startProfiling, stopProfiling} = require 'v8-profiler'


chooseDropWhereLandOnOther = (states) ->
  samples = []
  formerHadGrasp = false
  graspedId = -1
  ungraspState = null
  for state in states
    if ungraspState
      # Item released. We're now trying to find when it settles.
      settled = false
      label = null
      if state.cleared
        # World cleared. Say it's settled, but don't assign a label.
        settled = true
      else
        item = findItem state, graspedId
        if item
          # Still here. See if it's moving.
          if norm(item.velocity) < 0.01
            # Stopped. See where it is.
            # TODO If the block bounced up, we might be catching it at the peak.
            # TODO Maybe we should check for that (by accel or location?).
            settled = true
            label = not onGround item
        else
          # It fell off the table, so it didn't land on another block.
          settled = true
          label = false
      if settled
        ungraspState = null
        if label?
          # Labeled, so record it.
          samples.push {
            entities: item for item in state.items when item.alive
            label
          }
    else
      graspedItems = findGraspedItems state
      hasGrasp = Boolean graspedItems.length
      if hasGrasp
        # TODO Support multiple grasped items intelligently?
        graspedId = graspedItems[0].id
      else if formerHadGrasp
        ungraspState = state
      formerHadGrasp = hasGrasp
  samples


findGraspedItems = (state) -> item for item in state.items when item.grasped


findItem = (state, id) ->
  for item in state.items
    if item.id is id
      return item
  null


go = ->
  # TODO Determine project root or let file be passed in.
  #startProfiling()
  load 'temp/stackiter-20101105-171808-671_drop-from-25.log',
    stateLimit: 1000
    ready: (states) ->
      report(states)
      samples = chooseDropWhereLandOnOther(states)
      trues = (sample for sample in samples when sample.label)
      log "#{trues.length} true of #{samples.length} samples"
      tree = startTree()
      tree.propagate emptyBindings samples
      leaf = tree.leaves()[0]
      log "Leaf with #{leaf.prob} prob and #{leaf.bindings.length} bindings"
      # TODO Or 'tree = learn tree' to avoid modification?
      learn tree
      leaf = tree.leaves()[0]
      log "Leaf with #{leaf.prob} prob and #{leaf.bindings.length} bindings"
      #stopProfiling()
      #debugger


norm = (vector) ->
  sum = 0
  for val in vector
    sum += val * val
  sqrt sum


onGround = (item) ->
  angle = item.orientation
  # Angles go from -1 to 1.
  # Here, dim 1 is upright, and 0 sideways.
  dim = Number angle < -0.75 or -0.25 < angle < 0.25 or 0.75 < angle
  abs(item.extent[dim] - item.location[1]) < 0.01


report = (states) ->
  log "At end:"
  # TODO What's the best way to say 'last item' in JS/Coffee?
  log "#{states[states.length - 1].items.length} items"
  #log states[102]
  #log states[102].items[9].extent
  #log "#{states[102].items.length} here"
  #log "#{states[700].items.length} there"
  log "#{states.length} states"


go()
