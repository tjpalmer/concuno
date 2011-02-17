{basename} = require 'path'
{compile}  = require 'coffee-script'
log = console.log
{readdir, readFile, writeFile} = require 'fs'
{spawn} = require 'child_process'

task 'build', "build cuncuno from source", ->
  log "Building ..."
  dir = 'cof' 
  files = readdir dir, (err, files) ->
    # TODO Deal with err.
    compileFile "#{dir}/#{file}" for file in files when file.match /\.cof$/

task 'test', "test stackiter learning", ->
  log "Testing ..."
  # When running cake build test, I always get the old code before the build.
  # This happens even if I use sync IO in compileFile.
  require './temp/stackiter-test'

compileFile = (name) ->
  readFile name, 'utf8', (err, input) ->
    throw err if err
    output = compile input, filename: name
    base = basename(name, '.cof')
    writeFile "temp/#{base}.js", output, (err) -> throw err if err
