# tinyc for Node.js

TinyC (TCC) as an embedded compiler in Node.js. The package bundles
one dual-form native file — the same file is the `tcc` command and
the libtcc library — together with the headers and libraries it
needs, and drives it with koffi.

## Install

```
npm install tinyc
```

The native distribution is inside the package (`native/`).
`TINYC_DIST` (distribution directory) and `TINYC_NATIVE` (the dual
file itself) override the bundled location.

## Command-line style

```js
const tcc = require('tinyc');

const code = tcc.runCli(['hello.c', '-o', 'hello']);  // exit code
const [code2, output] = tcc.runCliCapture(['--version']);
```

`runCli` fills in argv[0] with the native file's own path, so the
compiler finds its bundled headers and libraries. The host process is
never exited; the return value is the compiler's exit code.

## API style

```js
const s = new tcc.State();
s.addIncludePath('/path/to/headers');     // local paths, used as-is
s.addFile('/path/to/libfoo.a');
s.compileString('int add(int a, int b){ return a + b; }');
const add = s.function('add', 'int', ['int', 'int']);
console.log(add(19, 23)); // 42
s.close();
```

Compile errors throw `tcc.TinyCError` and are also collected in
`s.errors`; they are not printed.

## Output (IO)

Compiler output goes through the TCCIO interface. Default is the
process's real stdout/stderr. Alternatives:

```js
tcc.runCli(args, { io: 'capture' });                    // via runCliCapture
tcc.runCli(args, { io: tcc.IO.file('build.log') });     // to a file
tcc.runCli(args, { io: tcc.IO.callback({ out: fn }) }); // fn(Buffer)
tcc.runCli(args, { io: process.stdout });               // any Writable
```

IO callbacks run synchronously on the calling thread, matching the
compiler's synchronous IO calls.

## Tests

```
node bundle_native.js   # after ../../build.sh
npm test
```
