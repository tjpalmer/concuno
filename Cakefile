{compile}  = require 'coffee-script'
log = console.log
{readdir, readFile} = require 'fs'
{spawn} = require 'child_process'

task 'build', "build cuncuno from source", ->
  dir = 'cof' 
  files = readdir 'cof', (err, files) ->
    # TODO Deal with err.
    compileFile "#{dir}/#{file}" for file in files when file.match /\.cof$/

task 'test', "test stackiter learning", ->
  require './temp/stackiter-test'

compileFileDirect = (name) ->
  ### Fails with:
  TypeError: In cof/test-stackiter.cof, Object {load} = require('./loader')
  
  load()
   has no method 'replace'
      at Lexer.tokenize (~/.local/lib/node/.npm/coffee-script/1.0.1/package/lib/lexer.js:21:19)
      at ~/.local/lib/node/.npm/coffee-script/1.0.1/package/lib/coffee-script.js:26:34
      at ~/Workspace/cuncuno/Cakefile:25:16
  ###
  readFile name, (err, input) ->
    # TODO Deal with err.
    output = compile input, filename: name

compileFileExec = (name) ->
  # Based on CoffeeScript's Cakefile.
  cof = spawn 'coffee', ['-o', 'temp'].concat name
  cof.stderr.on 'data', (buffer) -> log buffer.toString()
  cof.on 'exit', (status) -> throw status if status != 0

compileFile = compileFileExec
