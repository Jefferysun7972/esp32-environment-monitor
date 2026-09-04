module.exports = (function() {
var __MODS__ = {};
var __DEFINE__ = function(modId, func, req) { var m = { exports: {}, _tempexports: {} }; __MODS__[modId] = { status: 0, func: func, req: req, m: m }; };
var __REQUIRE__ = function(modId, source) { if(!__MODS__[modId]) return require(source); if(!__MODS__[modId].status) { var m = __MODS__[modId].m; m._exports = m._tempexports; var desp = Object.getOwnPropertyDescriptor(m, "exports"); if (desp && desp.configurable) Object.defineProperty(m, "exports", { set: function (val) { if(typeof val === "object" && val !== m._exports) { m._exports.__proto__ = val.__proto__; Object.keys(val).forEach(function (k) { m._exports[k] = val[k]; }); } m._tempexports = val }, get: function () { return m._tempexports; } }); __MODS__[modId].status = 1; __MODS__[modId].func(__MODS__[modId].req, m, m.exports); } return __MODS__[modId].m.exports; };
var __REQUIRE_WILDCARD__ = function(obj) { if(obj && obj.__esModule) { return obj; } else { var newObj = {}; if(obj != null) { for(var k in obj) { if (Object.prototype.hasOwnProperty.call(obj, k)) newObj[k] = obj[k]; } } newObj.default = obj; return newObj; } };
var __REQUIRE_DEFAULT__ = function(obj) { return obj && obj.__esModule ? obj.default : obj; };
__DEFINE__(1788499835159, function(require, module, exports) {


Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.generateUniqueNumber = exports.addUniqueNumber = void 0;
var _addUniqueNumber = require("./factories/add-unique-number");
var _cache = require("./factories/cache");
var _generateUniqueNumber = require("./factories/generate-unique-number");
const LAST_NUMBER_WEAK_MAP = new WeakMap();
const cache = (0, _cache.createCache)(LAST_NUMBER_WEAK_MAP);
const generateUniqueNumber = exports.generateUniqueNumber = (0, _generateUniqueNumber.createGenerateUniqueNumber)(cache, LAST_NUMBER_WEAK_MAP);
const addUniqueNumber = exports.addUniqueNumber = (0, _addUniqueNumber.createAddUniqueNumber)(generateUniqueNumber);
}, function(modId) {var map = {"./factories/add-unique-number":1788499835160,"./factories/cache":1788499835161,"./factories/generate-unique-number":1788499835162}; return __REQUIRE__(map[modId], modId); })
__DEFINE__(1788499835160, function(require, module, exports) {


Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.createAddUniqueNumber = void 0;
const createAddUniqueNumber = generateUniqueNumber => {
  return set => {
    const number = generateUniqueNumber(set);
    set.add(number);
    return number;
  };
};
exports.createAddUniqueNumber = createAddUniqueNumber;
}, function(modId) { var map = {}; return __REQUIRE__(map[modId], modId); })
__DEFINE__(1788499835161, function(require, module, exports) {


Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.createCache = void 0;
const createCache = lastNumberWeakMap => {
  return (collection, nextNumber) => {
    lastNumberWeakMap.set(collection, nextNumber);
    return nextNumber;
  };
};
exports.createCache = createCache;
}, function(modId) { var map = {}; return __REQUIRE__(map[modId], modId); })
__DEFINE__(1788499835162, function(require, module, exports) {


Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.createGenerateUniqueNumber = void 0;
/*
 * The value of the constant Number.MAX_SAFE_INTEGER equals (2 ** 53 - 1) but it
 * is fairly new.
 */
const MAX_SAFE_INTEGER = Number.MAX_SAFE_INTEGER === undefined ? 9007199254740991 : Number.MAX_SAFE_INTEGER;
const TWO_TO_THE_POWER_OF_TWENTY_NINE = 536870912;
const TWO_TO_THE_POWER_OF_THIRTY = TWO_TO_THE_POWER_OF_TWENTY_NINE * 2;
const createGenerateUniqueNumber = (cache, lastNumberWeakMap) => {
  return collection => {
    const lastNumber = lastNumberWeakMap.get(collection);
    /*
     * Let's try the cheapest algorithm first. It might fail to produce a new
     * number, but it is so cheap that it is okay to take the risk. Just
     * increase the last number by one or reset it to 0 if we reached the upper
     * bound of SMIs (which stands for small integers). When the last number is
     * unknown it is assumed that the collection contains zero based consecutive
     * numbers.
     */
    let nextNumber = lastNumber === undefined ? collection.size : lastNumber < TWO_TO_THE_POWER_OF_THIRTY ? lastNumber + 1 : 0;
    if (!collection.has(nextNumber)) {
      return cache(collection, nextNumber);
    }
    /*
     * If there are less than half of 2 ** 30 numbers stored in the collection,
     * the chance to generate a new random number in the range from 0 to 2 ** 30
     * is at least 50%. It's benifitial to use only SMIs because they perform
     * much better in any environment based on V8.
     */
    if (collection.size < TWO_TO_THE_POWER_OF_TWENTY_NINE) {
      while (collection.has(nextNumber)) {
        nextNumber = Math.floor(Math.random() * TWO_TO_THE_POWER_OF_THIRTY);
      }
      return cache(collection, nextNumber);
    }
    // Quickly check if there is a theoretical chance to generate a new number.
    if (collection.size > MAX_SAFE_INTEGER) {
      throw new Error('Congratulations, you created a collection of unique numbers which uses all available integers!');
    }
    // Otherwise use the full scale of safely usable integers.
    while (collection.has(nextNumber)) {
      nextNumber = Math.floor(Math.random() * MAX_SAFE_INTEGER);
    }
    return cache(collection, nextNumber);
  };
};
exports.createGenerateUniqueNumber = createGenerateUniqueNumber;
}, function(modId) { var map = {}; return __REQUIRE__(map[modId], modId); })
return __REQUIRE__(1788499835159);
})()
//miniprogram-npm-outsideDeps=[]
//# sourceMappingURL=index.js.map