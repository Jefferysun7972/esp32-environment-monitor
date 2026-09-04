module.exports = (function() {
var __MODS__ = {};
var __DEFINE__ = function(modId, func, req) { var m = { exports: {}, _tempexports: {} }; __MODS__[modId] = { status: 0, func: func, req: req, m: m }; };
var __REQUIRE__ = function(modId, source) { if(!__MODS__[modId]) return require(source); if(!__MODS__[modId].status) { var m = __MODS__[modId].m; m._exports = m._tempexports; var desp = Object.getOwnPropertyDescriptor(m, "exports"); if (desp && desp.configurable) Object.defineProperty(m, "exports", { set: function (val) { if(typeof val === "object" && val !== m._exports) { m._exports.__proto__ = val.__proto__; Object.keys(val).forEach(function (k) { m._exports[k] = val[k]; }); } m._tempexports = val }, get: function () { return m._tempexports; } }); __MODS__[modId].status = 1; __MODS__[modId].func(__MODS__[modId].req, m, m.exports); } return __MODS__[modId].m.exports; };
var __REQUIRE_WILDCARD__ = function(obj) { if(obj && obj.__esModule) { return obj; } else { var newObj = {}; if(obj != null) { for(var k in obj) { if (Object.prototype.hasOwnProperty.call(obj, k)) newObj[k] = obj[k]; } } newObj.default = obj; return newObj; } };
var __REQUIRE_DEFAULT__ = function(obj) { return obj && obj.__esModule ? obj.default : obj; };
__DEFINE__(1788499835167, function(require, module, exports) {

var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || function (mod) {
    if (mod && mod.__esModule) return mod;
    var result = {};
    if (mod != null) for (var k in mod) if (k !== "default" && Object.prototype.hasOwnProperty.call(mod, k)) __createBinding(result, mod, k);
    __setModuleDefault(result, mod);
    return result;
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.v6 = exports.AddressError = exports.Address6 = exports.Address4 = void 0;
var ipv4_1 = require("./ipv4");
Object.defineProperty(exports, "Address4", { enumerable: true, get: function () { return ipv4_1.Address4; } });
var ipv6_1 = require("./ipv6");
Object.defineProperty(exports, "Address6", { enumerable: true, get: function () { return ipv6_1.Address6; } });
var address_error_1 = require("./address-error");
Object.defineProperty(exports, "AddressError", { enumerable: true, get: function () { return address_error_1.AddressError; } });
const helpers = __importStar(require("./v6/helpers"));
exports.v6 = { helpers };
//# sourceMappingURL=ip-address.js.map
}, function(modId) {var map = {"./address-error":1788499835170,"./v6/helpers":1788499835171}; return __REQUIRE__(map[modId], modId); })
__DEFINE__(1788499835170, function(require, module, exports) {

Object.defineProperty(exports, "__esModule", { value: true });
exports.AddressError = void 0;
class AddressError extends Error {
    constructor(message, parseMessage) {
        super(message);
        this.name = 'AddressError';
        this.parseMessage = parseMessage;
    }
}
exports.AddressError = AddressError;
//# sourceMappingURL=address-error.js.map
}, function(modId) { var map = {}; return __REQUIRE__(map[modId], modId); })
__DEFINE__(1788499835171, function(require, module, exports) {

Object.defineProperty(exports, "__esModule", { value: true });
exports.escapeHtml = escapeHtml;
exports.spanAllZeroes = spanAllZeroes;
exports.spanAll = spanAll;
exports.spanLeadingZeroes = spanLeadingZeroes;
exports.simpleGroup = simpleGroup;
function escapeHtml(s) {
    return s
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;')
        .replace(/"/g, '&quot;')
        .replace(/'/g, '&#39;');
}
/**
 * @returns {String} the string with all zeroes contained in a <span>
 */
function spanAllZeroes(s) {
    return escapeHtml(s).replace(/(0+)/g, '<span class="zero">$1</span>');
}
/**
 * @returns {String} the string with each character contained in a <span>
 */
function spanAll(s, offset = 0) {
    const letters = s.split('');
    return letters
        .map((n, i) => `<span class="digit value-${escapeHtml(n)} position-${i + offset}">${spanAllZeroes(n)}</span>`)
        .join('');
}
function spanLeadingZeroesSimple(group) {
    return escapeHtml(group).replace(/^(0+)/, '<span class="zero">$1</span>');
}
/**
 * @returns {String} the string with leading zeroes contained in a <span>
 */
function spanLeadingZeroes(address) {
    const groups = address.split(':');
    return groups.map((g) => spanLeadingZeroesSimple(g)).join(':');
}
/**
 * Groups an address
 * @returns {String} a grouped address
 */
function simpleGroup(addressString, offset = 0) {
    const groups = addressString.split(':');
    return groups.map((g, i) => {
        if (/group-v4/.test(g)) {
            return g;
        }
        return `<span class="hover-group group-${i + offset}">${spanLeadingZeroesSimple(g)}</span>`;
    });
}
//# sourceMappingURL=helpers.js.map
}, function(modId) { var map = {}; return __REQUIRE__(map[modId], modId); })
return __REQUIRE__(1788499835167);
})()
//miniprogram-npm-outsideDeps=["./ipv4","./ipv6"]
//# sourceMappingURL=index.js.map