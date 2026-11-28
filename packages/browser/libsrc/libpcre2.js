'use strict';
/*
 * PCRE2 (8-bit) — JS implementation for the wasm32 sysroot,
 * shipped as the js-kind unit libpcre2.tinyc.js.
 *
 * Patterns are translated to JS RegExp at compile time and
 * matched with it. The translation is a real parser, not a
 * pass-through: the PCRE constructs RegExp cannot express —
 * atomic groups, possessive quantifiers, \K, conditionals,
 * recursion and subroutine calls, callouts, (*VERBS), \G,
 * \C, \X, unicode properties — fail compilation with an error
 * code whose message (through pcre2_get_error_message_8)
 * names the construct. Nothing is silently mistranslated.
 *
 * Subjects are handled as byte strings: the subject bytes are
 * decoded latin1, so string indices are byte offsets, and the
 * ovector is written in bytes, as the C interface specifies.
 *
 * Compiled-pattern and match-data state lives in guest memory
 * (allocated through ctx.alloc): a compiled pattern is a JSON
 * descriptor block (the translated source, flags, group table)
 * and a match data block carries its ovector in the open, so
 * state is the same in every context that touches it.
 *
 * Known semantic edges of the RegExp backing, for the record:
 * ^ also matches after \r (PCRE2's default newline convention
 * matches it only after \n), and $ matches before a single
 * trailing newline where PCRE2 also allows a trailing CRLF.
 * PCRE2_NOTBOL / PCRE2_NOTEOL are refused (PCRE2_ERROR_BADOPTION).
 */

// ---- guest memory helpers ------------------------------------

function dv(ctx) { return new DataView(ctx.memory().buffer); }
function mem(ctx) { return ctx.u8(); }
function getU32(ctx, a) { return dv(ctx).getUint32(a, true); }
function setU32(ctx, a, v) { dv(ctx).setUint32(a, v >>> 0, true); }

function bytesToLatin1(ctx, ptr, len) {
  const m = mem(ctx);
  let s = '';
  const CHUNK = 8192;
  for (let i = 0; i < len; i += CHUNK) {
    s += String.fromCharCode(...m.subarray(ptr + i, ptr + Math.min(len, i + CHUNK)));
  }
  return s;
}
function cstrLen(ctx, ptr) {
  const m = mem(ctx);
  let n = 0;
  while (m[ptr + n] !== 0) n++;
  return n;
}

// ---- errors ----------------------------------------------------
// Compile errors are positive codes, runtime errors negative,
// as upstream. Codes 900+ are this implementation's: each names
// one unsupported construct. Standard codes carry the upstream
// texts for the ones this code emits.

const ERR_NOMATCH = -1;
const ERR_BADMAGIC = -31;
const ERR_BADOPTION = -34;
const ERR_NOSUBSTRING = -49;
const ERR_UNSET = -55;
const ERR_MISSING_BRACKET = 106;
const ERR_UNSUPPORTED_ESCAPE = 137;

const ERRMSG = new Map([
  [ERR_NOMATCH, 'no match'],
  [ERR_BADMAGIC, 'bad magic number / wrong layout in compiled pattern or match data'],
  [ERR_BADOPTION, 'bad option: an option this implementation does not support was passed ' +
    '(PCRE2_NOTBOL/PCRE2_NOTEOL have no RegExp equivalent)'],
  [ERR_NOSUBSTRING, 'substring number out of range'],
  [ERR_UNSET, 'substring is unset'],
  [ERR_MISSING_BRACKET, 'missing terminating ] for character class'],
  [ERR_UNSUPPORTED_ESCAPE, 'unsupported escape sequence'],
  [900, 'atomic groups (?>...) are not supported by the RegExp-backed implementation'],
  [901, 'possessive quantifiers are not supported by the RegExp-backed implementation'],
  [902, '\\K is not supported by the RegExp-backed implementation'],
  [903, 'conditional patterns (?(...)...) are not supported by the RegExp-backed implementation'],
  [904, 'recursion and subroutine calls are not supported by the RegExp-backed implementation'],
  [905, 'callouts are not supported by the RegExp-backed implementation'],
  [906, '(*VERB) constructs are not supported by the RegExp-backed implementation'],
  [907, '\\G is not supported by the RegExp-backed implementation'],
  [908, 'unicode property escapes (\\p, \\P) are not supported by the RegExp-backed implementation'],
  [909, 'an unsupported option letter was used in a (?...) group'],
  [910, 'PCRE2_ANCHORED together with PCRE2_MULTILINE is not supported by the RegExp-backed implementation'],
  [911, 'unknown POSIX character class'],
  [912, 'reference to a non-existent subpattern'],
  [913, 'duplicate group name'],
  [914, 'missing closing parenthesis'],
  [915, '\\x{...} value is outside the 8-bit range'],
  [917, 'the pattern could not be translated for the RegExp-backed implementation'],
]);
let lastErrorDetail = null;   // precise text for code 917, per evaluation

class CompileError extends Error {
  constructor(code, offset) {
    super(ERRMSG.get(code) ?? `error ${code}`);
    this.code = code;
    this.offset = offset;
  }
}

// ---- compile options ------------------------------------------

const OPT_CASELESS = 0x00000008;
const OPT_DOTALL = 0x00000020;
const OPT_DUPNAMES = 0x00000040;
const OPT_EXTENDED = 0x00000080;
const OPT_MULTILINE = 0x00000400;
const OPT_UNGREEDY = 0x00040000;
const OPT_LITERAL = 0x02000000;
const OPT_ENDANCHORED = 0x20000000;
const OPT_ANCHORED = 0x80000000;

// ---- the translator --------------------------------------------

const POSIX_CLASSES = {
  alnum: 'A-Za-z0-9', alpha: 'A-Za-z', blank: ' \\t', cntrl: '\\x00-\\x1f\\x7f',
  digit: '0-9', graph: '!-~', lower: 'a-z', print: ' -~', punct: "!-/:-@\\[-`{-~",
  space: '\\t\\n\\x0b\\f\\r ', upper: 'A-Z', word: 'A-Za-z0-9_', xdigit: 'A-Fa-f0-9',
};

function escapeLiteralChar(ch) {
  return /[.*+?^${}()|[\]\\]/.test(ch) ? '\\' + ch : ch;
}

function translatePattern(src, options) {
  const st = {
    src, i: 0,
    groupCount: 0,
    names: new Map(),
    maxBackref: 0,
    caseless: !!(options & OPT_CASELESS),
    multiline: !!(options & OPT_MULTILINE),
    dotall: !!(options & OPT_DOTALL),
    extended: !!(options & OPT_EXTENDED),
    ungreedy: !!(options & OPT_UNGREEDY),
    dupnames: !!(options & OPT_DUPNAMES),
  };
  const body = parseAlternation(st, { ...optsOf(st) }, true);
  if (st.i < src.length) {
    // Stopped early: an unmatched ')'.
    throw new CompileError(914, st.i);
  }
  if (st.maxBackref > st.groupCount) {
    throw new CompileError(912, 0);
  }
  let flags = 'gd';
  if (st.caseless) flags += 'i';
  if (st.multiline) flags += 'm';
  if (st.dotall) flags += 's';
  let source = body;
  if (options & OPT_ANCHORED) {
    if (options & OPT_MULTILINE) throw new CompileError(910, 0);
    source = '^(?:' + source + ')';
  }
  if (options & OPT_ENDANCHORED) source = '(?:' + source + ')(?![\\s\\S])';
  // Validate through the real engine now, so a translation the
  // engine rejects is a compile error here, not a surprise at
  // match time.
  try {
    new RegExp(source, flags);
  } catch (err) {
    lastErrorDetail = `pattern rejected by the RegExp engine after translation: ${err.message}`;
    throw new CompileError(917, 0);
  }
  return { source, flags, groupCount: st.groupCount, names: Object.fromEntries(st.names) };
}

function optsOf(st) {
  return {
    caseless: st.caseless, multiline: st.multiline, dotall: st.dotall,
    extended: st.extended, ungreedy: st.ungreedy,
  };
}

// Parse branches until ')' or end of input. Returns the emitted
// source. `top` is true at the pattern's outermost level.
function parseAlternation(st, opts, top) {
  const branches = [parseSequence(st, opts, top)];
  while (st.src[st.i] === '|') {
    st.i++;
    branches.push(parseSequence(st, opts, top));
  }
  return branches.join('|');
}

function skipExtended(st, opts) {
  if (!opts.extended) return;
  for (;;) {
    const ch = st.src[st.i];
    if (ch === ' ' || ch === '\t' || ch === '\n' || ch === '\r' || ch === '\f' || ch === '\v') {
      st.i++;
    } else if (ch === '#') {
      while (st.i < st.src.length && st.src[st.i] !== '\n') st.i++;
    } else {
      return;
    }
  }
}

function parseSequence(st, opts, top) {
  let out = '';
  for (;;) {
    skipExtended(st, opts);
    const ch = st.src[st.i];
    if (ch === undefined || ch === '|' || ch === ')') return out;
    const atom = parseAtom(st, opts, top);
    out += atom;
    out += parseQuantifier(st, opts);
  }
}

function parseQuantifier(st, opts) {
  const ch = st.src[st.i];
  let q = null;
  if (ch === '*' || ch === '+' || ch === '?') {
    q = ch;
    st.i++;
  } else if (ch === '{') {
    const m = /^\{(\d+)(?:,(\d*))?\}/.exec(st.src.slice(st.i, st.i + 24));
    if (m) {
      q = m[0];
      st.i += m[0].length;
    }
  }
  if (q === null) return '';
  const next = st.src[st.i];
  if (next === '+') throw new CompileError(901, st.i);
  let lazy = false;
  if (next === '?') { lazy = true; st.i++; }
  if (opts.ungreedy) lazy = !lazy;
  return q + (lazy ? '?' : '');
}

function parseAtom(st, opts, top) {
  const ch = st.src[st.i];
  if (ch === '(') return parseGroup(st, opts, top);
  if (ch === '[') return parseClass(st, opts);
  if (ch === '\\') return parseEscape(st, opts, false);
  if (ch === '.') {
    st.i++;
    return opts.dotall ? '[\\s\\S]' : '[^\\n]';
  }
  if (ch === '^') { st.i++; return '^'; }
  if (ch === '$') { st.i++; return '$'; }
  if (ch === '*' || ch === '+' || ch === '?') {
    // A quantifier with nothing to repeat: PCRE2 errors; the
    // RegExp engine would too — surface it as a compile error.
    lastErrorDetail = 'quantifier does not follow a repeatable item';
    throw new CompileError(917, st.i);
  }
  st.i++;
  return escapeLiteralChar(ch);
}

function parseGroup(st, opts, top) {
  const start = st.i;
  st.i++; // consume '('
  if (st.src[st.i] !== '?') {
    st.groupCount++;
    const inner = parseAlternation(st, opts, false);
    expectClose(st);
    return '(' + inner + ')';
  }
  st.i++; // consume '?'
  const c = st.src[st.i];
  if (c === ':' || c === '=' || c === '!') {
    st.i++;
    const inner = parseAlternation(st, opts, false);
    expectClose(st);
    return '(?' + c + inner + ')';
  }
  if (c === '<') {
    const c2 = st.src[st.i + 1];
    if (c2 === '=' || c2 === '!') {
      st.i += 2;
      const inner = parseAlternation(st, opts, false);
      expectClose(st);
      return '(?<' + c2 + inner + ')';
    }
    // (?<name>...)
    const end = st.src.indexOf('>', st.i);
    if (end < 0) throw new CompileError(917, start);
    const name = st.src.slice(st.i + 1, end);
    return finishNamedGroup(st, opts, name, end + 1);
  }
  if (c === 'P') {
    const c2 = st.src[st.i + 1];
    if (c2 === '<') {
      const end = st.src.indexOf('>', st.i);
      if (end < 0) throw new CompileError(917, start);
      const name = st.src.slice(st.i + 2, end);
      return finishNamedGroup(st, opts, name, end + 1);
    }
    if (c2 === '=') {
      const end = st.src.indexOf(')', st.i);
      if (end < 0) throw new CompileError(917, start);
      const name = st.src.slice(st.i + 2, end);
      st.i = end + 1;
      return '\\k<' + name + '>';
    }
    throw new CompileError(904, start); // (?P>...) subroutine call
  }
  if (c === "'") {
    const end = st.src.indexOf("'", st.i);
    if (end < 0) throw new CompileError(917, start);
    const name = st.src.slice(st.i + 1, end);
    return finishNamedGroup(st, opts, name, end + 1);
  }
  if (c === '>') throw new CompileError(900, start);       // atomic group
  if (c === '(') throw new CompileError(903, start);       // conditional
  if (c === 'R' || (c >= '0' && c <= '9') || c === '&') {
    throw new CompileError(904, start);                    // recursion / subroutine
  }
  if (c === 'C') throw new CompileError(905, start);       // callout
  if (c === '#') {
    const end = st.src.indexOf(')', st.i);
    if (end < 0) throw new CompileError(914, start);
    st.i = end + 1;
    return '';
  }
  if (c === '*') throw new CompileError(906, start);       // (*VERB)
  // Option letters: (?flags), (?flags:), (?flags-flags:).
  const m = /^([A-Za-z]*)(?:-([A-Za-z]*))?([):])/.exec(st.src.slice(st.i, st.i + 16));
  if (m && /^[imsxuU]*$/.test(m[1]) && /^[imsxuU]*$/.test(m[2] ?? '')) {
    const on = m[1], off = m[2] ?? '';
    st.i += m[0].length;
    const sub = { ...opts };
    for (const L of on) {
      if (L === 'x') sub.extended = true;
      else if (L === 'U') sub.ungreedy = true;
    }
    for (const L of off) {
      if (L === 'x') sub.extended = false;
      else if (L === 'U') sub.ungreedy = false;
    }
    const jsFlags = jsModifierLetters(on, off);
    if (m[3] === ':') {
      const inner = parseAlternation(st, sub, false);
      expectClose(st);
      return '(?' + jsFlags + ':' + inner + ')';
    }
    // Bare (?flags): applies to the rest of the enclosing
    // group. Emit it as a modifier group wrapping the rest of
    // this group, parsed now.
    const rest = parseAlternation(st, sub, top);
    return '(?' + jsFlags + ':' + rest + ')';
  }
  lastErrorDetail = `unrecognized group construct "(?${c}..."`;
  throw new CompileError(917, start);
}

function jsModifierLetters(on, off) {
  let out = '';
  for (const L of on) if (L === 'i' || L === 'm' || L === 's') out += L;
  const offJs = [...off].filter((L) => L === 'i' || L === 'm' || L === 's');
  if (offJs.length) out += '-' + offJs.join('');
  return out;
}

function finishNamedGroup(st, opts, name, afterName) {
  if (!/^[A-Za-z_][A-Za-z0-9_]*$/.test(name)) {
    lastErrorDetail = `invalid group name "${name}"`;
    throw new CompileError(917, st.i);
  }
  if (st.names.has(name) && !st.dupnames) {
    throw new CompileError(913, st.i);
  }
  st.groupCount++;
  if (!st.names.has(name)) st.names.set(name, st.groupCount);
  st.i = afterName;
  const inner = parseAlternation(st, opts, false);
  expectClose(st);
  return '(?<' + name + '>' + inner + ')';
}

function expectClose(st) {
  if (st.src[st.i] !== ')') throw new CompileError(914, st.i);
  st.i++;
}

function parseClass(st, opts) {
  const start = st.i;
  st.i++; // consume '['
  let negate = false;
  if (st.src[st.i] === '^') { negate = true; st.i++; }
  let body = '';
  let first = true;
  for (;;) {
    const ch = st.src[st.i];
    if (ch === undefined) throw new CompileError(ERR_MISSING_BRACKET, start);
    if (ch === ']' && !first) { st.i++; break; }
    first = false;
    if (ch === '[' && st.src[st.i + 1] === ':') {
      // POSIX class [:name:] or [:^name:]
      const end = st.src.indexOf(':]', st.i);
      if (end < 0) throw new CompileError(ERR_MISSING_BRACKET, start);
      let name = st.src.slice(st.i + 2, end);
      let neg = false;
      if (name.startsWith('^')) { neg = true; name = name.slice(1); }
      const expansion = POSIX_CLASSES[name];
      if (expansion === undefined) throw new CompileError(911, st.i);
      if (neg) {
        // A negated POSIX class is only expressible when it is
        // the whole class: [[:^alpha:]].
        if (body !== '' || st.src[end + 2] !== ']') {
          lastErrorDetail = `negated POSIX class [:^${name}:] inside a larger class is not supported`;
          throw new CompileError(917, st.i);
        }
        st.i = end + 2;
        return '[^' + expansion + ']';
      }
      body += expansion;
      st.i = end + 2;
      continue;
    }
    if (ch === '\\') {
      body += parseEscape(st, opts, true);
      continue;
    }
    if (ch === '-') {
      body += '-';
      st.i++;
      continue;
    }
    // A '[' that does not open a POSIX class is a literal in
    // both engines.
    body += (ch === ']' || ch === '^' || ch === '\\') ? '\\' + ch : ch;
    st.i++;
  }
  return '[' + (negate ? '^' : '') + body + ']';
}

// Translate one escape sequence; st.i is at the backslash.
// inClass selects the character-class rules (\b is backspace
// there, \d etc. still expand).
function parseEscape(st, opts, inClass) {
  const start = st.i;
  st.i++; // consume '\'
  const c = st.src[st.i];
  if (c === undefined) {
    lastErrorDetail = 'pattern ends with a backslash';
    throw new CompileError(917, start);
  }
  st.i++;
  switch (c) {
    case 'd': case 'D': case 's': case 'S': case 'w': case 'W':
      return '\\' + c;
    case 'b':
      return inClass ? '\\x08' : '\\b';
    case 'B':
      if (inClass) break;
      return '\\B';
    case 'A':
      if (inClass) break;
      return '^';
    case 'z':
      if (inClass) break;
      return '(?![\\s\\S])';
    case 'Z':
      if (inClass) break;
      return '(?=\\n?$)';
    case 'n': return '\\n';
    case 'r': return '\\r';
    case 't': return '\\t';
    case 'f': return '\\f';
    case 'a': return '\\x07';
    case 'e': return '\\x1b';
    case 'v':
      // PCRE2 \v is the vertical-whitespace class; JS \v is
      // the single vertical-tab character.
      return inClass ? '\\x0a-\\x0d\\x85' : '[\\x0a-\\x0d\\x85]';
    case 'V':
      if (inClass) break;
      return '[^\\x0a-\\x0d\\x85]';
    case 'h':
      return inClass ? '\\x09\\x20\\xa0' : '[\\x09\\x20\\xa0]';
    case 'H':
      if (inClass) break;
      return '[^\\x09\\x20\\xa0]';
    case 'N':
      if (inClass) break;
      return '[^\\n]';
    case 'R':
      if (inClass) break;
      return '(?:\\r\\n|[\\n\\x0b\\f\\r\\x85])';
    case 'K': throw new CompileError(902, start);
    case 'G': throw new CompileError(907, start);
    case 'C': case 'X':
      lastErrorDetail = `\\${c} is not supported by the RegExp-backed implementation`;
      throw new CompileError(917, start);
    case 'p': case 'P': throw new CompileError(908, start);
    case 'Q': {
      // \Q...\E literal quoting.
      let out = '';
      while (st.i < st.src.length) {
        if (st.src[st.i] === '\\' && st.src[st.i + 1] === 'E') {
          st.i += 2;
          return out;
        }
        out += escapeLiteralChar(st.src[st.i]);
        st.i++;
      }
      return out;
    }
    case 'E':
      return '';   // a stray \E quotes nothing
    case 'x': {
      if (st.src[st.i] === '{') {
        const end = st.src.indexOf('}', st.i);
        if (end < 0) throw new CompileError(917, start);
        const value = parseInt(st.src.slice(st.i + 1, end), 16);
        if (!(value >= 0) || value > 0xff) throw new CompileError(915, start);
        st.i = end + 1;
        return '\\x' + value.toString(16).padStart(2, '0');
      }
      const hex = /^[0-9A-Fa-f]{1,2}/.exec(st.src.slice(st.i, st.i + 2));
      if (hex) {
        st.i += hex[0].length;
        return '\\x' + parseInt(hex[0], 16).toString(16).padStart(2, '0');
      }
      return 'x';
    }
    case 'c': {
      const cc = st.src[st.i];
      if (cc === undefined) throw new CompileError(917, start);
      st.i++;
      return '\\x' + (cc.toUpperCase().charCodeAt(0) & 0x1f).toString(16).padStart(2, '0');
    }
    case 'k': {
      // \k<name>, \k'name', \k{name}
      const open = st.src[st.i];
      const closers = { '<': '>', "'": "'", '{': '}' };
      if (closers[open]) {
        const end = st.src.indexOf(closers[open], st.i);
        if (end < 0) throw new CompileError(917, start);
        const name = st.src.slice(st.i + 1, end);
        st.i = end + 1;
        return '\\k<' + name + '>';
      }
      break;
    }
    case '0':
      return '\\x00';
    default:
      if (c >= '1' && c <= '9') {
        if (inClass) break;
        const n = c.charCodeAt(0) - 48;
        if (n > st.maxBackref) st.maxBackref = n;
        return '\\' + c;
      }
      if (/[A-Za-z]/.test(c)) throw new CompileError(ERR_UNSUPPORTED_ESCAPE, start);
      // Escaped punctuation is the literal character in both.
      return escapeLiteralChar(c);
  }
  lastErrorDetail = `\\${c} is not supported ${inClass ? 'in a character class ' : ''}by the RegExp-backed implementation`;
  throw new CompileError(917, start);
}

// ---- compiled pattern / match data blocks ----------------------
// Code block: magic u32 @0, JSON byte length u32 @4, JSON @8.
// The JSON descriptor: { re, flags, groupCount, names,
// options, blockSize, nameTable }.
// Match data block: magic u32 @0, pairs u32 @4, subject ptr
// @8, subject length @12, last count u32 @16, group count
// u32 @20, code ptr @24, ovector (pairs of u32) @28.

const CODE_MAGIC = 0x32435050;   // 'PCP2' little-endian-ish tag
const MD_MAGIC = 0x32444d50;     // 'PMD2'
const UNSET = 0xffffffff;

function writeDescriptor(ctx, desc) {
  // Serialize to a fixpoint: blockSize is a field of the JSON
  // and also determines its length.
  desc.blockSize = 0;
  let bytes = new TextEncoder().encode(JSON.stringify(desc));
  for (let pass = 0; pass < 4; pass++) {
    const size = 8 + bytes.length + 1;
    if (desc.blockSize === size) break;
    desc.blockSize = size;
    bytes = new TextEncoder().encode(JSON.stringify(desc));
  }
  const addr = ctx.alloc(8 + bytes.length + 1);
  setU32(ctx, addr, CODE_MAGIC);
  setU32(ctx, addr + 4, bytes.length);
  mem(ctx).set(bytes, addr + 8);
  mem(ctx)[addr + 8 + bytes.length] = 0;
  return addr;
}

function readDescriptor(ctx, addr) {
  if (!addr || getU32(ctx, addr) !== CODE_MAGIC) return null;
  const len = getU32(ctx, addr + 4);
  const bytes = mem(ctx).slice(addr + 8, addr + 8 + len);
  return JSON.parse(new TextDecoder().decode(bytes));
}

function buildNameTable(ctx, desc) {
  const names = Object.entries(desc.names);
  if (names.length === 0) { desc.nameTable = 0; return; }
  const entrySize = Math.max(...names.map(([n]) => n.length)) + 3;
  desc.nameEntrySize = entrySize;
  const addr = ctx.alloc(entrySize * names.length);
  const m = mem(ctx);
  names.forEach(([name, idx], i) => {
    const at = addr + i * entrySize;
    m[at] = idx & 0xff;
    m[at + 1] = (idx >> 8) & 0xff;
    for (let j = 0; j < name.length; j++) m[at + 2 + j] = name.charCodeAt(j);
    m[at + 2 + name.length] = 0;
  });
  desc.nameTable = addr;
}

// ---- the C-facing functions -------------------------------------

function pcre2Compile(ctx, patternPtr, length, options, errCodePtr, errOffsetPtr, _cctx) {
  lastErrorDetail = null;
  const len = length < 0 ? cstrLen(ctx, patternPtr) : length;
  const pattern = bytesToLatin1(ctx, patternPtr, len);
  try {
    if (options & ~(OPT_CASELESS | OPT_DOTALL | OPT_DUPNAMES | OPT_EXTENDED |
                    OPT_MULTILINE | OPT_UNGREEDY | OPT_LITERAL | OPT_ENDANCHORED |
                    OPT_ANCHORED | 0x40000000 /* NO_UTF_CHECK: a no-op here */)) {
      lastErrorDetail = `compile option 0x${(options >>> 0).toString(16)} is not supported`;
      throw new CompileError(917, 0);
    }
    let desc;
    if (options & OPT_LITERAL) {
      const escaped = [...pattern].map(escapeLiteralChar).join('');
      let flags = 'gd';
      if (options & OPT_CASELESS) flags += 'i';
      desc = { re: escaped, flags, groupCount: 0, names: {}, options };
    } else {
      const t = translatePattern(pattern, options);
      desc = { re: t.source, flags: t.flags, groupCount: t.groupCount, names: t.names, options };
    }
    buildNameTable(ctx, desc);
    return writeDescriptor(ctx, desc);
  } catch (err) {
    if (err instanceof CompileError) {
      if (errCodePtr) setU32(ctx, errCodePtr, err.code);
      if (errOffsetPtr) setU32(ctx, errOffsetPtr, err.offset);
      return 0;
    }
    throw err;
  }
}

function pcre2CodeFree(ctx, addr) {
  const desc = readDescriptor(ctx, addr);
  if (!desc) return;
  if (desc.nameTable) ctx.free(desc.nameTable);
  ctx.free(addr);
}

function matchDataAlloc(ctx, pairs, codeAddr, groupCount) {
  const addr = ctx.alloc(28 + pairs * 8);
  setU32(ctx, addr, MD_MAGIC);
  setU32(ctx, addr + 4, pairs);
  setU32(ctx, addr + 8, 0);
  setU32(ctx, addr + 12, 0);
  setU32(ctx, addr + 16, 0);
  setU32(ctx, addr + 20, groupCount);
  setU32(ctx, addr + 24, codeAddr);
  for (let i = 0; i < pairs * 2; i++) setU32(ctx, addr + 28 + i * 4, UNSET);
  return addr;
}

const MATCH_NOTBOL = 0x00000001, MATCH_NOTEOL = 0x00000002,
  MATCH_NOTEMPTY = 0x00000004, MATCH_NOTEMPTY_ATSTART = 0x00000008;

function pcre2Match(ctx, codeAddr, subjectPtr, length, startOffset, options, mdAddr, _mctx) {
  const desc = readDescriptor(ctx, codeAddr);
  if (!desc) return ERR_BADMAGIC;
  if (!mdAddr || getU32(ctx, mdAddr) !== MD_MAGIC) return ERR_BADMAGIC;
  if (options & (MATCH_NOTBOL | MATCH_NOTEOL)) return ERR_BADOPTION;
  if (options & ~(MATCH_NOTEMPTY | MATCH_NOTEMPTY_ATSTART)) return ERR_BADOPTION;
  const subject = bytesToLatin1(ctx, subjectPtr, length);
  const re = new RegExp(desc.re, desc.flags);
  re.lastIndex = startOffset;
  const pairs = getU32(ctx, mdAddr + 4);
  for (;;) {
    const m = re.exec(subject);
    if (!m) return ERR_NOMATCH;
    const empty = m[0].length === 0;
    if (empty && (options & MATCH_NOTEMPTY)) {
      re.lastIndex = m.index + 1;
      if (m.index >= length) return ERR_NOMATCH;
      continue;
    }
    if (empty && (options & MATCH_NOTEMPTY_ATSTART) && m.index === startOffset) {
      re.lastIndex = m.index + 1;
      if (m.index >= length) return ERR_NOMATCH;
      continue;
    }
    // Fill the ovector.
    let highest = 0;
    const total = desc.groupCount + 1;
    for (let i = 0; i < total; i++) {
      const ind = m.indices[i];
      if (i < pairs) {
        if (ind) {
          setU32(ctx, mdAddr + 28 + i * 8, ind[0]);
          setU32(ctx, mdAddr + 28 + i * 8 + 4, ind[1]);
        } else {
          setU32(ctx, mdAddr + 28 + i * 8, UNSET);
          setU32(ctx, mdAddr + 28 + i * 8 + 4, UNSET);
        }
      }
      if (ind) highest = i + 1;
    }
    setU32(ctx, mdAddr + 8, subjectPtr);
    setU32(ctx, mdAddr + 12, length);
    const count = Math.min(highest, pairs);
    setU32(ctx, mdAddr + 16, count);
    setU32(ctx, mdAddr + 24, codeAddr);
    return total > pairs ? 0 : highest;
  }
}

function substringGet(ctx, mdAddr, number, bufPtrPtr, bufLenPtr, byName) {
  if (!mdAddr || getU32(ctx, mdAddr) !== MD_MAGIC) return ERR_BADMAGIC;
  let n = number;
  if (byName) {
    const codeAddr = getU32(ctx, mdAddr + 24);
    const desc = codeAddr ? readDescriptor(ctx, codeAddr) : null;
    const name = bytesToLatin1(ctx, number, cstrLen(ctx, number));
    if (!desc || !(name in desc.names)) return ERR_NOSUBSTRING;
    n = desc.names[name];
  }
  const groupCount = getU32(ctx, mdAddr + 20);
  if (n < 0 || n > groupCount) return ERR_NOSUBSTRING;
  const start = getU32(ctx, mdAddr + 28 + n * 8);
  const end = getU32(ctx, mdAddr + 28 + n * 8 + 4);
  if (start === UNSET) return ERR_UNSET;
  const subjectPtr = getU32(ctx, mdAddr + 8);
  const len = end - start;
  const buf = ctx.alloc(len + 1);
  mem(ctx).set(mem(ctx).subarray(subjectPtr + start, subjectPtr + end), buf);
  mem(ctx)[buf + len] = 0;
  setU32(ctx, bufPtrPtr, buf);
  setU32(ctx, bufLenPtr, len);
  return 0;
}

// ---- export table ------------------------------------------------

const I32 = 'i32';
function sig(params, results) { return { params, results }; }

const EXPORTS = [
  ['pcre2_compile_8', sig([I32, I32, I32, I32, I32, I32], [I32]),
    (ctx, p, l, o, ec, eo, cc) => pcre2Compile(ctx, p, l, o, ec, eo, cc)],
  ['pcre2_code_free_8', sig([I32], []), (ctx, a) => { pcre2CodeFree(ctx, a); }],
  ['pcre2_match_8', sig([I32, I32, I32, I32, I32, I32, I32], [I32]),
    (ctx, c, s, l, so, o, md, mc) => pcre2Match(ctx, c, s, l, so, o, md, mc)],
  ['pcre2_match_data_create_from_pattern_8', sig([I32, I32], [I32]), (ctx, codeAddr) => {
    const desc = readDescriptor(ctx, codeAddr);
    if (!desc) return 0;
    return matchDataAlloc(ctx, desc.groupCount + 1, codeAddr, desc.groupCount);
  }],
  ['pcre2_match_data_create_8', sig([I32, I32], [I32]), (ctx, ovectorSize) => {
    return matchDataAlloc(ctx, Math.max(1, (ovectorSize / 2) | 0), 0, 0);
  }],
  ['pcre2_match_data_free_8', sig([I32], []), (ctx, a) => { if (a) ctx.free(a); }],
  ['pcre2_get_ovector_pointer_8', sig([I32], [I32]), (ctx, md) => md + 28],
  ['pcre2_get_ovector_count_8', sig([I32], [I32]), (ctx, md) => getU32(ctx, md + 16)],
  ['pcre2_substring_get_bynumber_8', sig([I32, I32, I32, I32], [I32]),
    (ctx, md, n, bp, bl) => substringGet(ctx, md, n, bp, bl, false)],
  ['pcre2_substring_get_byname_8', sig([I32, I32, I32, I32], [I32]),
    (ctx, md, namePtr, bp, bl) => substringGet(ctx, md, namePtr, bp, bl, true)],
  ['pcre2_substring_free_8', sig([I32], []), (ctx, p) => { if (p) ctx.free(p); }],
  ['pcre2_get_error_message_8', sig([I32, I32, I32], [I32]), (ctx, code, buf, bufLen) => {
    const text = (code === 917 && lastErrorDetail) ? lastErrorDetail
      : (ERRMSG.get(code) ?? `unknown error ${code}`);
    const bytes = new TextEncoder().encode(text);
    if (bufLen < bytes.length + 1) return -48;   // PCRE2_ERROR_NOMEMORY
    mem(ctx).set(bytes, buf);
    mem(ctx)[buf + bytes.length] = 0;
    return bytes.length;
  }],
  ['pcre2_pattern_info_8', sig([I32, I32, I32], [I32]), (ctx, codeAddr, what, where) => {
    const desc = readDescriptor(ctx, codeAddr);
    if (!desc) return ERR_BADMAGIC;
    switch (what) {
      case 0: setU32(ctx, where, desc.options >>> 0); return 0;         // ALLOPTIONS
      case 4: setU32(ctx, where, desc.groupCount); return 0;            // CAPTURECOUNT
      case 17: setU32(ctx, where, Object.keys(desc.names).length); return 0;  // NAMECOUNT
      case 18: setU32(ctx, where, desc.nameEntrySize ?? 0); return 0;   // NAMEENTRYSIZE
      case 19: setU32(ctx, where, desc.nameTable ?? 0); return 0;       // NAMETABLE
      case 22: setU32(ctx, where, desc.blockSize ?? 0); return 0;       // SIZE
      default: return ERR_BADOPTION;
    }
  }],
];

const out = { __exports: EXPORTS.map(([name, s]) => ({ name, sig: s })) };
for (const [name, , fn] of EXPORTS) out[name] = fn;
module.exports = out;
