log = console.log
{readFile} = require 'fs'


exports.load = (file, options) ->
  readFile file, 'utf8', (err, content) ->
    throw err if err
    log "Parsing #{file} ..."
    states = new Loader().parseAll content.split(/\n/), options.stateLimit
    options.ready(states) if options.ready?

# TODO Move clone out to some utility area.
# Cloning technique based on examples and discussions here:
# http://stackoverflow.com/questions/122102/
#   what-is-the-most-efficient-way-to-clone-a-javascript-object
clone = (obj) ->
  # This could be more sophisticated about more types, but eh.
  return obj if not obj? or typeof obj isnt 'object'
  copied = new obj.constructor
  for key, val of obj
    copied[key] = clone(val)
  copied


class Loader

  constructor: ->
    @indexes = []
    @state = cleared: false, items: [], time: 0
    @states = []

  findIndex: (args, argIndex = 1) ->
    id = Number args[argIndex]
    index = @indexes[id]
    throw "no such item" if not index?
    index

  findItem: (args, argIndex = 1) -> @state.items[@findIndex args, argIndex]

  parseAlive: (args) ->
    item = @findItem args
    item.alive = args[2] is 'true'

  parseAll: (lines, stateLimit = Infinity) ->
    for line in lines
      @parseLine line
      if @states.length >= stateLimit
        break
    if @states.length < stateLimit
      @states.push clone @state
    @states

  parseClear: (args) ->
    @state.cleared = true

  parseDestroy: (args) ->
    index = @findIndex args
    # Remove the item and update the indexes.
    @state.items.splice index, 1
    for i in [index ... @state.items.length]
      @indexes[@state.items[i].id]--

  parseExtent: (args) ->
    item = @findItem args
    item.extent[0] = Number args[2]
    item.extent[1] = Number args[3]

  parseGrasp: (args) ->
    tool = @findItem args
    item = @findItem args, 2
    tool.grasping = true
    item.grasped = true

  parseItem: (args) ->
    id = Number args[1]
    @indexes[id] = @state.items.length
    @state.items.push {
      alive: false
      color: [0, 0, 0]
      extent: [0, 0]
      grasped: false
      grasping: false
      id
      location: [0, 0]
      orientation: 0
      type: null
      velocity: [0, 0]
    }

  parseLine: (line) ->
    args = line.trim().split /\s+/
    switch args[0]
      when 'alive'
        @parseAlive
      when 'clear'
        @parseClear args
      when 'destroy'
        @parseDestroy args
      when 'extent'
        @parseExtent args
      when 'grasp'
        @parseGrasp args
      when 'item'
        @parseItem args
      when 'pos'
        @parsePos args
      when 'posvel'
        @parsePosVel args
      when 'release'
        @parseRelease args
      when 'rot'
        @parseRot args
      when 'time'
        @parseTime args
      when 'type'
        @parseType args

  parsePos: (args) ->
    item = @findItem args
    item.location[0] = Number args[2]
    item.location[1] = Number args[3]

  parsePosVel: (args) ->
    # TODO Unify these that are similar form?
    item = @findItem args
    item.velocity[0] = Number args[2]
    item.velocity[1] = Number args[3]

  parseRelease: (args) ->
    tool = @findItem args
    item = @findItem args, 2
    tool.grasping = false
    item.grasped = false

  parseRot: (args) ->
    item = @findItem args
    # TODO Angle is rats. Convert to radians or not?
    item.orientation = Number args[2]

  parseTime: (args) ->
    if args[1] is 'sim'
      @states.push clone @state
      @state.cleared = false
      @state.time = Number args[3]

  parseType: (args) ->
    item = @findItem args
    item.type = args[2]
