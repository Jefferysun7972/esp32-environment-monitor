module.exports = (function() {
var __MODS__ = {};
var __DEFINE__ = function(modId, func, req) { var m = { exports: {}, _tempexports: {} }; __MODS__[modId] = { status: 0, func: func, req: req, m: m }; };
var __REQUIRE__ = function(modId, source) { if(!__MODS__[modId]) return require(source); if(!__MODS__[modId].status) { var m = __MODS__[modId].m; m._exports = m._tempexports; var desp = Object.getOwnPropertyDescriptor(m, "exports"); if (desp && desp.configurable) Object.defineProperty(m, "exports", { set: function (val) { if(typeof val === "object" && val !== m._exports) { m._exports.__proto__ = val.__proto__; Object.keys(val).forEach(function (k) { m._exports[k] = val[k]; }); } m._tempexports = val }, get: function () { return m._tempexports; } }); __MODS__[modId].status = 1; __MODS__[modId].func(__MODS__[modId].req, m, m.exports); } return __MODS__[modId].m.exports; };
var __REQUIRE_WILDCARD__ = function(obj) { if(obj && obj.__esModule) { return obj; } else { var newObj = {}; if(obj != null) { for(var k in obj) { if (Object.prototype.hasOwnProperty.call(obj, k)) newObj[k] = obj[k]; } } newObj.default = obj; return newObj; } };
var __REQUIRE_DEFAULT__ = function(obj) { return obj && obj.__esModule ? obj.default : obj; };
__DEFINE__(1788499835135, function(require, module, exports) {
/*
The MIT License (MIT)

Copyright (c) 2014-2022 Matteo Collina

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/



const leven = require('./leven')

function commist (opts) {
  opts = opts || {}
  const commands = []
  const maxDistance = opts.maxDistance || Infinity

  function lookup (array) {
    if (typeof array === 'string') { array = array.split(' ') }

    let res = commands.map(function (cmd) {
      return cmd.match(array)
    })

    res = res.filter(function (match) {
      if (match.partsNotMatched !== 0) {
        return false
      }
      return match.distances.reduce(function (acc, curr) {
        return acc && curr <= maxDistance
      }, true)
    })

    res = res.sort(function (a, b) {
      if (a.inputNotMatched > b.inputNotMatched) { return 1 }

      if (a.inputNotMatched === b.inputNotMatched && a.totalDistance > b.totalDistance) { return 1 }

      return -1
    })

    res = res.map(function (match) {
      return match.cmd
    })

    return res
  }

  function parse (args) {
    const matching = lookup(args)

    if (matching.length > 0) {
      matching[0].call(args)

      // return null to indicate there is nothing left to do
      return null
    }

    return args
  }

  async function parseAsync (args) {
    const matching = lookup(args)

    if (matching.length > 0) {
      await matching[0].call(args)
      // return null to indicate there is nothing left to do
      return null
    }

    return args
  }

  function register (inputCommand, func) {
    let commandOptions = {
      command: inputCommand,
      strict: false,
      func
    }

    if (typeof inputCommand === 'object') {
      commandOptions = Object.assign(commandOptions, inputCommand)
    }

    const matching = lookup(commandOptions.command)

    matching.forEach(function (match) {
      if (match.string === commandOptions.command) { throw new Error('command already registered: ' + commandOptions.command) }
    })

    commands.push(new Command(commandOptions))

    return this
  }

  return {
    register,
    parse,
    parseAsync,
    lookup
  }
}

function Command (options) {
  this.string = options.command
  this.strict = options.strict
  this.parts = this.string.split(' ')
  this.length = this.parts.length
  this.func = options.func
}

Command.prototype.call = function call (argv) {
  return this.func(argv.slice(this.length))
}

Command.prototype.match = function match (string) {
  return new CommandMatch(this, string)
}

function CommandMatch (cmd, array) {
  this.cmd = cmd
  this.distances = cmd.parts.map(function (elem, i) {
    if (array[i] !== undefined) {
      if (cmd.strict) {
        return elem === array[i] ? 0 : undefined
      } else {
        return leven(elem, array[i])
      }
    } else { return undefined }
  }).filter(function (distance, i) {
    return distance !== undefined && distance < cmd.parts[i].length
  })

  this.partsNotMatched = cmd.length - this.distances.length
  this.inputNotMatched = array.length - this.distances.length
  this.totalDistance = this.distances.reduce(function (acc, i) { return acc + i }, 0)
}

module.exports = commist

}, function(modId) {var map = {"./leven":1788499835136}; return __REQUIRE__(map[modId], modId); })
__DEFINE__(1788499835136, function(require, module, exports) {
/*
 * MIT License
 *
 * Copyright (c) Sindre Sorhus <sindresorhus@gmail.com> (https://sindresorhus.com)
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial
 * portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
 * NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

const array = []
const characterCodeCache = []

module.exports = function leven (first, second) {
  if (first === second) {
    return 0
  }

  const swap = first

  // Swapping the strings if `a` is longer than `b` so we know which one is the
  // shortest & which one is the longest
  if (first.length > second.length) {
    first = second
    second = swap
  }

  let firstLength = first.length
  let secondLength = second.length

  // Performing suffix trimming:
  // We can linearly drop suffix common to both strings since they
  // don't increase distance at all
  // Note: `~-` is the bitwise way to perform a `- 1` operation
  while (firstLength > 0 && (first.charCodeAt(~-firstLength) === second.charCodeAt(~-secondLength))) {
    firstLength--
    secondLength--
  }

  // Performing prefix trimming
  // We can linearly drop prefix common to both strings since they
  // don't increase distance at all
  let start = 0

  while (start < firstLength && (first.charCodeAt(start) === second.charCodeAt(start))) {
    start++
  }

  firstLength -= start
  secondLength -= start

  if (firstLength === 0) {
    return secondLength
  }

  let bCharacterCode
  let result
  let temporary
  let temporary2
  let index = 0
  let index2 = 0

  while (index < firstLength) {
    characterCodeCache[index] = first.charCodeAt(start + index)
    array[index] = ++index
  }

  while (index2 < secondLength) {
    bCharacterCode = second.charCodeAt(start + index2)
    temporary = index2++
    result = index2

    for (index = 0; index < firstLength; index++) {
      temporary2 = bCharacterCode === characterCodeCache[index] ? temporary : temporary + 1
      temporary = array[index]
      // eslint-disable-next-line no-multi-assign
      result = array[index] = temporary > result ? (temporary2 > result ? result + 1 : temporary2) : (temporary2 > temporary ? temporary + 1 : temporary2)
    }
  }

  return result
}

}, function(modId) { var map = {}; return __REQUIRE__(map[modId], modId); })
return __REQUIRE__(1788499835135);
})()
//miniprogram-npm-outsideDeps=[]
//# sourceMappingURL=index.js.map