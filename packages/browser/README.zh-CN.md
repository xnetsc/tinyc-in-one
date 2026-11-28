# tinyc-web

[English](README.md) | **简体中文**

浏览器版 TinyC（TCC）：编译器编成 WebAssembly，配 JavaScript 胶水层、虚拟文件系统与 Web Worker 接口。

## 构建前提

产出 WASM 模块需要已激活的 Emscripten SDK（`emcc` 在 `PATH` 上）。`./build.sh` 会检查 `emcc`，缺失时报错停下；它不安装任何东西。

TCC 的一份构建只有一个代码生成目标，所以本包每个目标带一份 WASM 模块、每个目标跑一次 `build.sh`：

```
TARGET=x86_64-linux-gnu ./build.sh   # 产出 tcc-x86_64-linux-gnu.tinyc.js（出货的资产信封）
TARGET=aarch64-linux-gnu ./build.sh  # 产出 tcc-aarch64-linux-gnu.tinyc.js
```

每个模块导出 libtcc 的 `tcc_*` API 加一个主入口（内核提供时是 `tinyc_main`，否则是 `main`；胶水优先用 `tinyc_main`、回退到 `main`）。八个通用目标编上游 TCC 树。`wasm32-wasi` 模块由项目自己的 WebAssembly 代码生成后端构建（仓库根的 `wasm-backend/` 树）：`build.sh` 的该分支用后端的 `wasm-gen.c` / `wasm-link.c` 源码与其补丁组装上游树，然后与其它目标一样用 emcc 编译。

胶水层（`index.mjs`、`worker.mjs`）不依赖 WASM 构建即完整：编译器模块来自按目标分的资产信封——直接取回，或经异步的 `loadModule(target, files)` 加载器返回该信封的内容（文本或字节）、由包自身解析信封并构建模块；测试套件经内部测试接缝喂入 mock 原生模块来跑胶水层。

## 目标

```js
import { loadTinyc } from 'tinyc-web';

const tinyc = await loadTinyc({ target: 'x86_64-linux-gnu', sysroot: true });
```

支持的目标——与原生目标矩阵同一集合，外加 `wasm32-wasi`（浏览器自身的宿主形式）。没有浏览器专属目标，也没有原生专属目标：

| 目标 | 产物 | 在浏览器里 run |
|---|---|---|
| `wasm32-wasi`（缺省） | WebAssembly（WASI） | 能 |
| `x86_64-linux-gnu` | Linux x86-64 ELF | 不能 |
| `aarch64-linux-gnu` | Linux AArch64 ELF | 不能 |
| `riscv64-linux-gnu` | Linux RISC-V 64 ELF | 不能 |
| `x86_64-windows` | Windows x86-64 PE | 不能 |
| `aarch64-linux-android` | Android AArch64 ELF（bionic） | 不能 |
| `x86_64-linux-android` | Android x86-64 ELF（bionic） | 不能 |
| `arm64-apple-darwin` | macOS arm64 Mach-O | 不能 |
| `x86_64-apple-darwin` | macOS x86-64 Mach-O | 不能 |

目标的模块以一份资产信封文件出货（`tcc-<target>.tinyc.js`：胶水源码加模块字节），按需加载——只取请求目标的那一份信封，每个页面至多取一次。不同目标的实例可共存；每个实例有自己的虚拟文件系统与自己的 sysroot 清单（`sysroot-manifests/<target>.json`；顶层的 `sysroot-manifest.json` 是缺省目标的清单）。未知目标会被拒绝，并列出支持的清单。

`sysroot: true` 把该目标自带的头文件集合（TCC 自带头——`float.h`、`stdarg.h`、`stddef.h`、`stdbool.h`、`tcclib.h`——加上该目标的 libc 头树，即原生构建随包的那棵树）加载进虚拟文件系统的 `/sysroot/include` 并注册为 include 目录。自定义清单可以以 `{ manifest, baseUrl }` 传入。

sysroot 还在 `/sysroot/lib` 下带该目标的链接期输入：`libtcc1.a`（编译器运行时），以及按目标而定的平台 C 运行时启动对象与每个系统库的 stub 共享库（ELF 目标）、导入定义文件（Windows）或文本 stub（Darwin）——来源与许可见 `sysroot-lib/README.md`。因此 hosted 链接（除 `wasm32-wasi` 外每个目标的缺省形态）产出的是动态链接程序：产物按名引用平台自身的库——`libc.so.6`/`libm.so.6`、bionic 的库、`msvcrt.dll`/`kernel32.dll`、`libSystem`——运行时由平台的加载器与库服务。

各目标清单的特有事实：

- Android 清单描述的是原生 Android 构建的 bionic sysroot 布局：`libc: "bionic"`、`androidApi: 30`、`dynamicLinker: "/system/bin/linker64"`（64 位 Android ELF 产物的解释器）。
- Darwin 清单带 TCC 自带头加上本包的 Darwin 头集——与原生构建随包的头相同。完整的 macOS SDK 头受许可限制、不随包分发；清单以 `systemHeaders: "user-sdk"` 标记这一点，随包头集之外的 SDK 头由使用者加进虚拟文件系统并注册其目录来提供（见*虚拟文件系统与添加文件*）。Darwin 目标只能编译成文件；Mach-O 产物要在 Apple Silicon 上运行需要代码签名——至少是 ad-hoc 签名——无论它在哪里编译。

## 输出格式

每个目标能产出的输出文件格式在每个包、每个平台上都相同。各目标的清单在 `outputs` 字段里带这份表，全表如下：

| 族（目标） | exe | obj | shared | ar |
|---|---|---|---|---|
| ELF（`*-linux-gnu`、`*-linux-android`） | ELF 可执行文件，无扩展名 | ELF 可重定位 `.o` | ELF 共享对象 `.so` | ar 归档 `.a` |
| PE（`x86_64-windows`） | PE 可执行文件 `.exe` | COFF 目标文件 `.o` | PE DLL `.dll` | ar 归档 `.a` |
| Mach-O（`*-apple-darwin`） | Mach-O 可执行文件，无扩展名 | Mach-O 目标文件 `.o` | Mach-O dylib `.dylib` | ar 归档 `.a` |
| WebAssembly（`wasm32-wasi`） | — | — | — | 模块：WebAssembly `.wasm` |

## 运行：只有 wasm32

编译成文件对每个目标都成立。运行不成立：编译器在浏览器里的宿主平台就是 WebAssembly 本身，所以只有 `wasm32-wasi` 目标的产物能在这里跑——产出的模块由浏览器实例化并执行。

对其它每个目标，胶水层在请求到达原生模块之前就先行拒绝 run 请求，抛 `TinycRunNotSupportedError`：

- `tinyc.callMain(['-run', ...])` 抛错；
- `state.run(...)` 抛错。

`instance.canRun` 报告实例的目标能不能 run。

### wasm32 的运行方式

WebAssembly 模块不能从自身内部实例化另一个 WebAssembly 模块——实例化是宿主能力。所以运行从不经过编译器模块：胶水层拿编译产物的字节，用 WebAssembly API 自己实例化它：

- `state.run(args, options)` 把产物写进虚拟文件系统的文件（`options.outputPath`，缺省 `/a.out`），读回字节并运行；WASI argv 是 `[outputPath, ...args]`。
- `tinyc.callMain(['-run', src, ...progArgs])` 先经导出的 main 把 `src` 编到 `/a.out`（原生 main 从不见到 `-run`），然后以 WASI argv `[src, ...progArgs]` 运行产物。
- `runWasmModule(bytes, options)` / `runWasmModuleSync(bytes, options)` 直接运行产物字节（前者是异步 `WebAssembly.compile` 版，后者是同步 `WebAssembly.Module`/`Instance` 版，Worker 内与上面两条路径用的都是它）。
- `tinyc.runWasmFile(path, options)` 运行已经存在虚拟 FS 里的产物——先前编译成文件的、或用 `addFile` 放进去的——不重新编译；它用 `exportFile` 读出字节，经同一个运行器与垫片栈运行，并应用实例注册的垫片。

入口约定：不给 `options.entry` 时，调导出的 `_start`（命令式），结果是 `{ exitCode }`；给 `options.entry` 时，以 `options.callArgs`（缺省 `options.args`）调该导出，返回值原样返回（函数式）。`state.run` 返回退出码，或入口的返回值。

实例得到一个最小 WASI 垫片（`wasi_snapshot_preview1`）：`fd_write`、`fd_read`、`fd_close`、`fd_seek`（stdio 描述符不可 seek：`ESPIPE`）、`fd_fdstat_get`、`args_get`、`args_sizes_get`、`environ_get`、`environ_sizes_get`、`clock_time_get`、`random_get`、`sched_yield` 与 `proc_exit`。`fd_write` 写到 fd 1/2 的输出以 `Uint8Array` 块路由到该次运行的 IO。`proc_exit` 从不结束宿主：垫片抛错、运行器接住它，代码成为该次运行的退出码。

往模块实例里传数据：`writeModuleData(instance, data, options)` 把字符串/`Uint8Array` 写进实例导出的内存——模块有导出的 `malloc` 时经它分配（结果带一个 `free()`），否则写到 `options.offset`（数字，或 `(byteLength) => address`）。`readModuleData` / `readModuleCString` 读回。

## 运行时垫片：缺省 libc

编到 wasm32 的 C 程序会导入它用到的 libc 函数，而本包里没有可链接的 libc 归档。所以每次运行还会得到缺省垫片库（`runtime-shims.mjs`）作为 `env` 名下的导入，用 JS 对着实例的线性内存实现：

- 内存/字符串：`memcpy`、`memmove`、`memset`、`memcmp`、`strlen`、`strcpy`、`strncpy`、`strcat`、`strncat`、`strcmp`、`strncmp`、`strchr`、`strrchr`、`strstr`、`strdup`
- 分配：`malloc`、`calloc`、`realloc`、`free`
- stdio：`printf`、`fprintf`、`vprintf`、`vfprintf`、`sprintf`、`vsprintf`、`snprintf`、`vsnprintf`、`puts`、`putchar`、`fputs`、`putc`、`fputc`、`fwrite`、`fread`、`fgetc`、`getchar`、`ungetc`
- stdlib：`exit`、`abort`、`atexit`、`atoi`、`atol`、`atoll`、`atof`、`strtol`、`strtoul`、`strtoll`、`strtoull`、`strtod`、`strtof`、`abs`、`labs`、`llabs`、`rand`、`srand`、`getenv`（返回 NULL）、`system`（返回 -1）
- ctype：`isalpha`、`isdigit`、`isspace`、`isalnum`、`isupper`、`islower`、`ispunct`、`iscntrl`、`isxdigit`、`isblank`、`isprint`、`isgraph`、`tolower`、`toupper`
- math：`sin`、`cos`、`tan`、`asin`、`acos`、`atan`、`atan2`、`exp`、`log`、`log10`、`pow`、`sqrt`、`floor`、`ceil`、`fabs`、`round`、`trunc`、`fmod`（即 JS 的 `Math` 函数）
- time：`time`（由 `Date.now()` 来的秒数）、`clock`、`difftime`

值得注意的细节：

- **时机。** 导入函数在实例存在之前就已创建，所以它们从不捕获内存：每次调用都经一个 holder 读内存。胶水在实例化之后、任何入口运行之前立即填好 holder，所以垫片被调用时内存必然已就位。
- **分配器。** `malloc` 一家是首次适配空闲链表分配器（带块分裂），管辖自 `heapBase` 起的区域（缺省 65536，可按次运行覆盖）；不够时它扩内存。模块若导出自己的 `malloc`，垫片把分配调用转发给模块的分配器，内部的那个从不碰堆——任何时候只有一个分配器在工作。
- **stdio。** printf 一家在 JS 里解析格式串，从线性内存读格式串与参数。变参从 `va_list` 指向的区域按顺序读，每个按其自然对齐：i32/指针占 4 字节槽，i64/f64 占 8 字节槽并对齐到 8。支持：标志 `- + space # 0`、宽度与精度（含 `*`）、长度修饰 `hh h l ll z j t`、转换 `d i u x X o c s p f F e E g G a A n %`。输出以字节路由到该次运行的 IO；`FILE*` 流值 2 走 `err`，其它值走 `out`。`fread`/`fgetc` 读该次运行的 stdin（`options.stdin`，其次是 `io.in()` 的块）。
- **exit。** `exit(code)` 在模块导出间接函数表（`__indirect_function_table`）时运行 `atexit` 处理器——处理器是表索引、只能经表调用——然后以 `{ exitCode: code }` 展开退出，与 `proc_exit` 相同。`abort()` 以退出码 134 展开退出。

### libtcc1 辅助函数

TCC 的运行时支持库（`vendor-tinycc/lib/`）已逐符号对照过 wasm32 目标能发出的东西。其中几乎没有需要 JS 垫片的，原因如下；唯一的例外是 `libtcc1.c` 里的一族辅助函数，它们确实会以导入形式出现。

- `libtcc1.c` 的转换——浮点转换 `__floatundisf/df/xf`、`__fixunssfdi`、`__fixsfdi`、`__fixunsdfdi`、`__fixdfdi`、`__fixunsxfdi`、`__fixxfdi`：wasm32 有原生 float↔i64 转换指令（且该目标上 long double 是 f64），所以代码生成器发的是指令——float→整数用饱和的 `trunc_sat`——不是调用。本版本的 `lib/` 树里没有 `__tcc_cvt_*` 符号；转换辅助就是上面的 `__fix*`/`__float*` 一族。
- `libtcc1.c` 的除法与移位——`__divdi3`、`__moddi3`、`__udivdi3`、`__umoddi3`（长长整除与取模）与 `__ashldi3`、`__ashrdi3`、`__lshrdi3`（变量计数的长长移位）：这些代码生成器确实发成调用，在编译出的模块里以 `env` 导入出现。该目标不带 libtcc1，本包也不为它们提供缺省垫片，所以用了长长整除、取模或变量计数长长移位的程序会实例化失败，抛 `TinycMissingImportError` 并指名该辅助函数；把 JS 实现以该名字注册为自定义垫片（见下）即可补上。常量计数的长长移位由代码生成器内联，不需要辅助函数。
- `builtin.c`——`__tcc_builtin_*` 一族（`ffs`、`clz`、`ctz`、`popcount`、`parity`，……）：wasm 有原生 `clz`/`ctz`/`popcnt` 指令；代码生成处理它们。
- `va_list.c`——`__va_arg`：wasm 目标上 va_list 是指向内存参数区的指针，`va_arg` 是代码生成器做的指针推进。
- `alloca.S`——`alloca`/`__alloca`：wasm 的 alloca 在生成的代码里调整栈指针全局量。
- `stdatomic.c` / `atomic.S`——`__atomic_*` 一族：程序用到 `<stdatomic.h>` 时，C 源码由 wasm 后端像任何 C 文件一样编译；在单线程 wasm 里这些操作归结为普通 load 与 store。不涉及宿主导入。
- `bcheck.c`——`__bound_*` 一族：只在 `-b` 边界检查模式下发出，且实现依赖 mmap、信号与 pthread。不提供。
- `runmain.c`——`_runmain`、`__run_on_exit`、`on_exit`、`atexit`、`exit`：原生 `-run` 的支持文件。浏览器路径里胶水直接调 `_start`，`exit`/`atexit` 语义来自上面的 libc 垫片。
- `armeabi.c`（`__aeabi_*`）、`lib-arm64.c`、`tcov.c`：ARM 目标的辅助与覆盖率运行时；与 wasm 无关。

## 自定义垫片

导入 WASI 垫片与缺省 libc 之外符号的程序——宿主 API、外设、任何东西——由注册为垫片的 JS 函数提供它们。垫片以 `fn(ctx, ...cArgs)` 形式被调用：

- `ctx.memory()` / `ctx.u8()`——实例的线性内存与它的一个 `Uint8Array` 视图；
- `ctx.readCString(ptr)`、`ctx.writeCString(ptr, str)`（写入 UTF-8 字节加一个 NUL 终止符，返回含 NUL 的字节数）、`ctx.readBytes(ptr, len)`、`ctx.writeBytes(ptr, bytes)`——内存辅助；
- `ctx.io`——该次运行的字节 IO（`out`/`err` 收 `Uint8Array`，可选的 `in()`）。

C 参数以其 wasm 类型到达：i32/f32/f64 是数字，i64 是 `BigInt`；返回值同路返回，所以 i64 结果必须以 `BigInt` 返回。JS 函数不声明签名——引擎把参数与返回值强制转换成模块自己声明的导入类型。

注册有三个层级。同一个导入，层级高者胜；同一层级内，后注册的替换先注册的：

1. **单次运行**——`runWasmModule(bytes, { shims: { name: fn } })`；`state.run(args, { shims })` 与 `callMain(['-run', ...], { shims })` 会转发它。
2. **实例**——`tinyc.registerShim(name, fn)`、`tinyc.registerShims({ ... })`、`tinyc.unregisterShim(name)`；适用于经该实例的每一次后续运行。
3. **包级全局**——`registerGlobalShim(name, fn)`、`registerGlobalShims({ ... })`、`unregisterGlobalShim(name)`；适用于每一个后续实例与 `runWasmModule` 调用。

这三层之下是缺省 libc 垫片与 WASI 垫片。三层之上是 `options.imports`，原样合并进导入对象（原始逃生口——也能提供内存这类非函数导入）。注册键是裸名字（模块 `env`），或 `"module.name"` 表示任何其它导入模块——包括 `wasi_snapshot_preview1`，其内置条目也能以同样方式覆盖。

**缺失的导入。** 实例化之前，运行器列出模块所需的导入（`WebAssembly.Module.imports`），与组装好的导入对象做差。缺任何一个都抛 `TinycMissingImportError`——完整的 `module.name` 清单在 `err.missing` 上、注册路径写在消息里——而不是引擎原始的 `LinkError`。把指名的符号注册上即可修复。

## 取出编译产物

编译产物是虚拟文件系统里的文件，对每个目标都成立（目标文件、可执行文件、库、Windows PE 文件）：

```js
const bytes = tinyc.exportFile('/out/prog.exe');  // Uint8Array
const blob = tinyc.exportBlob('/out/prog.exe');   // Blob, e.g. for a download link
```

## 用法

### script 标签

本包也能构建为单个经典脚本文件 `tinyc-web.js`，它定义全局 `TinycWeb`，API 与模块版相同；`TinycWeb.loadTinyc` 是入口点。从 npm 包经 CDN：

```html
<script src="https://cdn.jsdelivr.net/npm/tinyc-web@0.1.0/tinyc-web.js"></script>
<script>
  (async () => {
    const tinyc = await TinycWeb.loadTinyc();
    tinyc.compileString('int answer() { return 42; }');
  })();
</script>
```

按目标分的原生文件不在 bundle 里：对请求的目标，资产信封 `tcc-<target>.tinyc.js` 经本包的传输层取回（以文本 fetch，或在 `file:` 上下文里以脚本注入），相对 bundle 文件自身的 URL 解析——用上面的 CDN 地址时，它自动来自同一个 `tinyc-web@0.1.0` 包前缀。自托管时，把 `tcc-<target>.tinyc.js` 放在与 `tinyc-web.js` 相同的目录。`loadTinyc({ baseUrl })` 改对另一个目录解析。sysroot 清单已内联进 bundle，所以它们不需要取 JSON 文件。

`tinyc-web.js` 由模块源码以以下命令重新生成：

```
npm run build:web     # node build-web-bundle.mjs (esbuild)
```

### 命令行模式

```js
const exitCode = tinyc.callMain(['-o', '/out/a.o', '-c', '/src/main.c']);
```

`callMain` 把 `argv[0]` 填为虚拟二进制路径（`/sysroot/bin/tinyc`），并调导出的 `tinyc_main`（回退到 `main`）。它返回退出码。导出的 main 不可重入；嵌套的 `callMain` 抛错。

### 细粒度 API

```js
const state = tinyc.createState();
state.setOutputType(1); // TCC_OUTPUT.MEMORY
state.addIncludePath('/deps/include');
state.compileString('int answer() { return 42; }');
state.delete();
```

State 的方法镜像它们调用的 libtcc 函数（`tcc_compile_string`、`tcc_add_include_path`、`tcc_add_file`，……）。某个方法所需的导出在原生模块里缺失时，抛 `TinycMissingExportError` 并指名该导出。

### 虚拟文件系统与添加文件

浏览器里没有真实文件系统可供 TCC 指向，所以用户文件以内容进入虚拟文件系统。这是「动态添加本地文件」规则的浏览器形态：

```js
await tinyc.addFile('/src/main.c', 'int main() { return 0; }');   // string / Uint8Array / ArrayBuffer
await tinyc.addFile('/src/util.h', fileFromAnInputElement);       // File or Blob
await tinyc.addFileFromUrl('/deps/sqlite3.h', 'https://example.com/sqlite3.h');
tinyc.registerIncludeDir('/deps');                                // 对新 state 走 tcc_add_include_path
```

文件内容持久化在一个存储里（`vfs-store.mjs`），不留在内存：每个目标一个 IndexedDB 数据库——`tinyc-web-fs-<target>`，一张以路径为键的 `files` 表——在有 IndexedDB 的地方（浏览器与 Web Worker），没有的地方（Node、测试）或 IndexedDB 打开失败时用内存后端。自定义存储或存储工厂可按次加载注入（`loadTinyc({ fsStore })` / `loadTinyc({ storeFactory })`）。虚拟 FS 本身在内存里只留路径索引（路径 → 大小/mtime）加一个 LRU 内容缓存（至多 32 个文件、总共 16 MiB，`VFS_CACHE_MAX_ENTRIES` / `VFS_CACHE_MAX_BYTES`）。`loadTinyc` 在 resolve 之前打开存储并载入索引，所以实例启动时就带着同一目标先前会话留下的文件——其中 sysroot 文件会被认出、不再重新取。当原生模块是 Emscripten 构建时，文件还会额外镜像进模块的 `FS`（TCC 运行期间编译工作集必须在那里）；这层镜像被限定在 `VirtualFS.mountTo`/`syncFile`，即 Emscripten FS 的适配点。

原生模块的文件系统是 WasmFS（build.sh 传 `-sWASMFS=1`；`-sFORCE_FILESYSTEM=1` 是把镜像所走的 JS `FS` API 包含进来的开关），其根后端是内存。镜像之前，`loadTinyc` 尝试把 WasmFS 的 OPFS 后端——浏览器在磁盘上的源私有文件系统——挂到模块文件系统内的 `/opfs`，经构建导出的 `wasmfs_create_opfs_backend` / `wasmfs_create_directory` 函数。这个尝试只在 Web Worker 里运行：在浏览器主线程上创建 OPFS 后端可能死锁，而 OPFS 同步访问句柄只在 Worker 里存在。任何失败——主线程、没有 OPFS 的浏览器、没构建出这些导出的模块、挂载被拒——都静默地把内存后端留在原地，`tinyc.fsBackend` 报告发生了哪一种：`'opfs'` 或 `'memory'`。是 `'opfs'` 时，镜像的编译工作集以挂载点为根：虚拟 FS 路径 `P` 在模块里位于 `/opfs` + `P`，在磁盘上，且每个交给原生模块的路径都被重定位以与之匹配——镜像的写与删、`tcc_add_file`、include/library/lib 路径注册、`tcc_output_file`，以及 `callMain` argv 里的路径参数（argv[0]、`-o` 的值、输入文件）——编译产物从那里读回虚拟 FS。虚拟 FS 自己的路径从不变；变的只有模块侧的位置。是 `'memory'` 时，工作集就位于内存后端根里的虚拟 FS 路径处，与路径字面完全一致。

实例上（以及 `tinyc.fs` 上）有完整的文件生命周期：写用 `addFile` / `addFileSync`，读用 `readFile`（字节）/ `readTextFile`（字符串）——`exportFile` / `exportBlob` 是同一读操作对编译产物的叫法——列举用 `listFiles(prefix)`，删用 `removeFile(path)`（索引条目、持久记录与原生模块里的镜像副本）或 `clearFiles()`（除 `/sysroot/` 下挂载的 sysroot 外的全部）。

存储是异步的，由此有两条时机规则。`addFile` 在其内容持久化后 resolve；`addFileSync` 立即更新索引、缓存与原生镜像，但只把存储写排队（`removeFile` / `clearFiles` 的删除同样排队）——`await tinyc.whenIdle()` 是排队的一切都已落定的点。而同步读由内容缓存提供：会话期间写过或读过的都会缓存，但从存储恢复、尚未碰过的文件不会——用 `readFile` / `exportFile` 读它会抛错并指名异步形式 `readFileAsync` / `readTextFileAsync` / `exportFileAsync`，后者从存储读它并暖缓存。

`compileFile(path)` 经 `tcc_compile_string` 编译一个虚拟 FS 源码，带一个携带该路径的 `#line` 标记；`state.addFile(path)` 把路径传给 `tcc_add_file`，用于源码、目标文件与归档。

### IO

IO 就是普通 JS 函数——本包不需要跳板层：

```js
const lines = [];
tinyc.setIO({
  out: (text) => lines.push(text),
  err: (text) => console.error(text),
  in: () => null, // EOF
});
```

缺省：`out`/`err` 是行缓冲的写者，写到 `console.log`/`console.error`，`in` 返回 EOF。按 state 的 `state.setIO(...)` 经 `tcc_set_error_func` 把该 state 的错误通道改道；原生 stdout 的路由按模块走，跟随实例 IO。

### Web Worker

```js
import { TinycWorker } from 'tinyc-web/worker';

const worker = new TinycWorker({ target: 'x86_64-linux-gnu', sysroot: true });
await worker.compileString('int f() { return 1; }');
const exitCode = await worker.runCli(['-c', '/src/main.c', '-o', '/out/a.o']);
worker.terminate();
```

`TinycWorker` 把 `worker.mjs` 作为模块 Worker 拉起，为 `compileString`、`compileFile`、`addFile`、`registerIncludeDir` 与 `runCli` 返回 promise。`target` 选项选择 Worker 内的按目标模块（缺省 `wasm32-wasi`）；run 守卫在那里同样适用，所以非 wasm32 目标的 `runCli(['-run', ...])` 会被拒绝。Worker 内的原生输出被转发到主线程，并分发给 `io` 选项。

垫片函数不能从主线程注册到 `TinycWorker` 上——JS 函数过不了 `postMessage`，`worker.registerShim(...)` / `worker.registerShims(...)` 会抛错说明这一点。Worker 侧的路径是构造器的 `shimModules` 选项：一个模块 URL 数组。Worker 导入每个模块，把它的默认导出——一个垫片映射对象（`{ name: fn }`）——注册到它的实例上：

```js
const worker = new TinycWorker({ shimModules: ['./my-shims.mjs'] });
// my-shims.mjs:  export default { host_log: (ctx, ptr) => { ... } };
```

## SIMD 内建函数

各目标共用同一套 freestanding 头文件集，有两处例外。
`wasm32-wasi` 的 sysroot 另带只服务 wasm 目标的头：SIMD
兼容头——`#include <wasm_simd128.h>`（WebAssembly SIMD128
内建集），以及 wasm 形态的 `arm_neon.h`、`xmmintrin.h`、
`emmintrin.h`（NEON 与 SSE/SSE2 表面中映射到 v128 操作的
子集）——还有 `dlfcn.h` 与 `pthread.h`，其实现是 wasm 的
进程装载器与调度器；其余目标的 sysroot 不带这两个头。
`x86_64-windows` 的 sysroot 不带 `math.h`、`tgmath.h`、
`tcclib.h`，如上文「目标与模块」一节所述。逐个头的覆盖
明细见根 README 的「SIMD 内建函数」一节。

## 测试

```
npm test          # node --test test.mjs
npm run test:web  # node --test test-web-bundle.mjs
```

该套件在 Node 里对着一个实现 `tcc_*` 名字的 mock 模块跑胶水逻辑，mock 经本包内部测试接缝喂入：从字符串、`Blob`/`File` 与 URL 来源摄入虚拟 FS，include 目录注册到达 `tcc_add_include_path`，在 Emscripten 接线点与经错误回调两条路上的 IO 路由，`callMain` 的 argv 组装，缺失导出错误，目标注册表（两个目标共存、虚拟文件系统隔离、按目标懒加载），非 wasm32 目标上的 run 守卫与 wasm32 run 路径，产物导出，以及各目标 sysroot 清单。run 路径还用真实手工汇编的 wasm 模块额外测过（`test.mjs` 里的字节数组，每个都附 WAT 描述）：一个函数式的 `add` 导出，一个经 WASI `fd_write` 垫片写出并经 `proc_exit` 退出的 `_start` 模块，WASI argv 垫片，缺省 libc 垫片（`printf` 格式化进 IO、`memcpy`、`malloc`/`free` 堆复用、`options.imports` 覆盖一个缺省垫片），以及自定义垫片面——带确切清单的缺失导入诊断、带 `ctx.readCString` 的实例级注册、对实例注册的单次运行覆盖、`unregisterShim`、`callMain -run` 的垫片转发、包级注册，与 `TinycWorker.registerShim` 的边界错误。第二个套件（`test-web-bundle.mjs`）重新构建 `tinyc-web.js`，把它作为经典脚本加载进一个打扮成浏览器页面的 Node `vm` 上下文（假的 `document.currentScript` 与 `location`），并核对：`TinycWeb` 全局与模块构建的导出清单、脚本场景的 base-URL 规则（脚本目录、页面目录回退、`baseUrl` 覆盖）、内联的各目标 sysroot 清单，以及 bundle 内的 mock 模块与手写 wasm run 路径。
