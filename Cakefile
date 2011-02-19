{basename} = require 'path'
{compile}  = require 'coffee-script'
log = console.log
{mkdir, readdir, readFile, stat, writeFile} = require 'fs'
{spawn} = require 'child_process'

outDir = 'temp'

task 'build', "build cuncuno from source", ->
  log "Building ..."
  # TODO Helper mkdir-if-needed function, or is mkdir alone good enough?
  stat outDir, (err, stats) ->
    if err
      throw err if err.code isnt 'ENOENT'
      mkdir outDir, 0777, (err) ->
        throw err if err
        compileAll()
    else if stats.isDirectory()
      compileAll()
    else
      throw "no good" 

task 'test', "test stackiter learning", ->
  log "Testing ..."
  # When running cake build test, I always get the old code before the build.
  # This happens even if I use sync IO in compileFile.
  require './temp/stackiter-test'

compileAll = ->
  dir = 'cof'
  files = readdir dir, (err, files) ->
    throw err if err
    compileFile "#{dir}/#{file}" for file in files when file.match /\.cof$/

compileFile = (name) ->
  readFile name, 'utf8', (err, input) ->
    throw err if err
    output = compile input, filename: name
    base = basename(name, '.cof')
    writeFile "#{outDir}/#{base}.js", output, (err) -> throw err if err
