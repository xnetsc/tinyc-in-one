# tinyc-embedded

[English](README.md) | **简体中文**

把 TinyC（TCC 0.9.28rc）做成可嵌入的编译器来分发：编译器本体加上它
的头文件与库文件，按目标产出一份双形态原生文件，并为 Node.js、
Python、Java、安卓与浏览器各做一个生态包。

## 双形态文件

`build/dist/x86_64-linux-gnu/bin/tinyc` 是同一个文件，同时是：

- 可执行文件——直接运行，行为与 `tcc` 命令一致；
- 动态库——`dlopen` 它，调 libtcc 的 `tcc_*` API，或调导出的
  `tinyc_main(argc, argv)` / `tinyc_main_io(argc, argv, io)` 在宿主
  进程内获得命令行语义。调 `tinyc_main` 返回退出码，不结束宿主
  进程。

构建方式（见 `build.sh`、`patches/apply_patches.py`）：

- `tcc.c`（包含 `libtcc.c` 的单体构建）以 `-fPIC` 编译、
  `-pie -Wl,--export-dynamic` 链接，其 `main` 改名为导出的
  `tinyc_main`，再由一个薄的真 `main` 转发给它。
- glibc 拒绝 dlopen 带 `DF_1_PIE` 标志的可执行文件，链接后用
  `tools/clear_pie_flag.py` 清掉该位；执行不受影响，清完的文件
  即可被 dlopen。
- `-dumpmachine` / `-dumpversion` 改为在 state 上置一个标志，不再
  调 `exit(0)`；`tinyc_main` 检查该标志并返回 0。

同一思路在各平台取该平台允许的形态：

- **Linux**——就是上面的 PIE 文件，可执行且可直接 dlopen。
- **Windows**——带导出表的 EXE。`LoadLibrary` 可以加载它；宿主
  胶水随后调一次导出的 `tinyc_init()`，再用 `tinyc_main` /
  `tcc_*`。
- **macOS**——PIE 可执行文件不能被 dlopen（Mach-O 的 `MH_PIE` 与
  `MH_DYLIB` 互斥），因此文件内预置一份 dylib 段，由包的胶水层
  解到缓存目录再 dlopen。对使用者仍是一个发行文件。

## 两种入口

每个包在同一份文件上提供相同的两种入口：

- **命令行式**——传参数数组，拿退出码。胶水把 `argv[0]` 填成原生
  文件自身的路径，使编译器能找到它自带的头与库。`tinyc_main`
  持有进程级全局状态：调用由胶水串行化，不可并发、不可重入。
- **会话式**——持有一个状态对象（封装 `tcc_new` / `TCCState`），
  逐步驱动 `tcc_*` API：加路径、编译字符串或文件、产出目标文件，
  或重定位进内存后直接调用编译出的函数。

## IO

编译器自身的输出走 `TCCIO` 接口（`core/tinyc_io.h`）：
`write_out` / `write_err` / `read_in` / `flush` 加一个上下文指针。
默认实现直接写进程的真实 stdio。现成实现：`tcc_io_stdio`、
`tcc_io_buffer`（分开或合并两种存法）、`tcc_io_file`、`tcc_io_fd`、
`tcc_io_FILE`、`tcc_io_callback`、`tcc_io_null`、`tcc_io_tee`。
编译器的报错、帮助、版本、搜索路径输出与 `-E` 预处理输出都经当前
IO；宿主进程的 fd 0/1/2 从不被替换。编译出的程序在进程内运行期间
（见下面的 run 规则），它的 stdin/stdout/stderr 在该次运行内接到
当前 IO 上。各包还可以传入自己的回调、流或文件对象，见各包小节。

## 宿主与目标

**宿主**是 TCC 自己运行的平台，**目标**是它编译产出的平台。一份
二进制只服务一个目标；包内每个目标放一份二进制与该目标的
sysroot。宿主与目标都只提供 64 位——32 位宿主上，各包加载器直接
报该宿主不受支持，不回退到其它平台的文件。

宿主：

| 宿主 triple | 形态 |
|---|---|
| `x86_64-linux-gnu` | 原生包（glibc） |
| `aarch64-linux-gnu` | 原生包（glibc） |
| `x86_64-w64-mingw32` | 原生包（Windows x86-64） |
| `arm64-apple-darwin` | 原生包（macOS） |
| `x86_64-apple-darwin` | 原生包（macOS） |
| `aarch64-linux-android`、`x86_64-linux-android` | 安卓 AAR；宿主与目标同为设备本身 |

目标共 9 个：`x86_64-linux-gnu`、`aarch64-linux-gnu`、
`riscv64-linux-gnu`、`x86_64-w64-mingw32`、`aarch64-linux-android`、
`x86_64-linux-android`、`arm64-apple-darwin`、`x86_64-apple-darwin`、
`wasm32-wasi`。每个目标能产出的输出格式（ELF / PE / Mach-O /
WebAssembly 及文件扩展名）记录在该目标 `manifest.json` 的
`outputs` 字段里，在所有包里一致。

- `riscv64-linux-gnu` 只是编译目标，没有 riscv64 宿主包。
- iOS 没有原生包：代码签名与禁止执行运行时生成代码这两条，使
  运行时下发的原生二进制在 iOS 上没有成立路径。iOS 上用浏览器包。

**run 规则。** 编译成文件对所有（宿主，目标）组合都成立。把编译
出的程序在宿主进程内运行（`tcc_run`、`-run`）只在目标就是宿主
自身平台时成立；交叉组合在调用点直接报错并指明该组合。浏览器是
同一条规则、宿主换成浏览器本身：只有 `wasm32-wasi` 的产物能在
浏览器里跑。注意进程内运行是把编译出的程序放进你自己的进程里
执行——它的 `exit()` 与崩溃都落到宿主头上。

## 动态添加自己的头文件与库

包内集合之外的头与库，在使用时以本地真实文件系统路径给出、原样
使用——不拷贝、不打包：

- 目录走命令行 `-I` / `-L`，或 API 的 `tcc_add_include_path` /
  `tcc_add_library_path`；
- 单个头文件注册其父目录；
- 单个库文件走 `tcc_add_file`。

用户路径注册在包内 sysroot 之前，同名时以用户版本为准。每次编译
都组装一个新的 state，路径清单改动后下一次编译即生效，不用重启
进程。（浏览器包没有可指向的真实文件系统，文件以内容进入它的
虚拟文件系统，见其小节。）

## 各生态包

npm、Python、Java 三个包内部用同一套布局：

```
<包根>/
├─ packaging-manifest.json     包内实际包含的 (宿主, 目标) 组合清单
├─ natives/<宿主-triple>/toolchain/<目标-triple>/tinyc[.exe]
└─ sysroot/<目标-triple>/      每目标一份，所有宿主共享
```

各包加载器的解析顺序一致：探测宿主 triple（环境变量
`TINYC_HOST_TRIPLE` 可覆盖探测），目标缺省等于宿主
（`TINYC_TARGET_TRIPLE` 可指定交叉目标），再在
`packaging-manifest.json` 里查该组合。包内没有该组合时报错，并
指明当前宿主、目标与包内可用组合。

### npm —— `tinyc`

```
npm install tinyc
```

原生文件放在按宿主分的子包 `@xnetx/tinyc-native-<宿主-triple>`
里，npm 作为可选依赖自动装上（主包自带 `natives/` 时也可独立
工作）。绑定用 koffi。

```js
const tcc = require('tinyc');

const code = tcc.runCli(['hello.c', '-o', 'hello']);   // 退出码
const [code2, output] = tcc.runCliCapture(['--version']);

const s = new tcc.State();
s.addIncludePath('/path/to/headers');     // 本地路径，原样使用
s.addFile('/path/to/libfoo.a');
s.compileString('int add(int a, int b){ return a + b; }');
const add = s.function('add', 'int', ['int', 'int']);
console.log(add(19, 23)); // 42
s.close();
```

IO 选项：`tcc.runCli(args, { io })` 接受 `'capture'`、
`tcc.IO.file(path)`、`tcc.IO.callback({ out: fn })`（fn 收到
Buffer），或任何 Node `Writable`。编译错误抛 `tcc.TinyCError`，
同时收集在 `s.errors` 里。

### Python —— `tinyc`

```
pip install tinyc
```

wheel 按宿主平台标签发布（`manylinux` x86_64 / aarch64、
`win_amd64`、`macosx` arm64 / x86_64），每个 wheel 带该宿主的
全部目标变体。绑定只用 `ctypes`，没有额外原生件。

```python
import ctypes, tinyc

code = tinyc.run_cli(["hello.c", "-o", "hello"])      # 退出码
code, output = tinyc.run_cli_capture(["--version"])   # 捕获的输出

with tinyc.State() as s:
    s.add_include_path("/path/to/headers")     # 本地路径，原样使用
    s.add_file("/path/to/libfoo.a")
    s.compile_string("int add(int a, int b){ return a + b; }")
    add = s.function("add", ctypes.c_int, ctypes.c_int, ctypes.c_int)
    assert add(19, 23) == 42
```

IO 选项：`tinyc.run_cli(args, io=...)` 接受 `"capture"`、
`tinyc.IO.file(path)`、`tinyc.IO.callback(out=my_fn)`（my_fn 收到
`bytes`），或任何二进制文件对象。编译错误抛 `tinyc.TinyCError`，
同时收集在 `s.errors` 里。

### Java —— Jar

服务器端 Java 包是一个 Jar，用 Panama FFM API（JDK 22 或更新，
运行时加 `--enable-native-access=ALL-UNNAMED`）。不用 JNI、不带
额外原生库——绑定层是纯 Java。首次使用时 Jar 把原生文件与
sysroot 解压到 `~/.cache/tinyc-java/<version>/`，之后复用该
目录。类在 `tinyc` 包下。

```java
try (TccSession s = TinyC.newSession()) {
    s.setOutputType(TinyC.TCC_OUTPUT_MEMORY);
    if (s.compileString("int add(int a, int b) { return a + b; }") != 0) {
        throw new IllegalStateException("compile failed");
    }
    s.relocate();
    MemorySegment fn = s.getSymbol("add");
    // 经 TinyC.functionHandle(...) 得到的 MethodHandle 调用它
}

int rc = TinyC.runCli("-o", "hello", "hello.c");   // 退出码
```

IO 是 `TccIO` 接口（`writeOut` / `writeErr` / `readIn`，按字节
传递），有现成的 stdio 实现、流适配实现与捕获实现。

### 安卓 —— AAR

安卓包是一个 AAR，按 ABI 带 `libtinyc.so`（带 JNI 入口的双形态
内核）——`arm64-v8a` ↔ `aarch64-linux-android`、`x86_64` ↔
`x86_64-linux-android`——toolchain 与 bionic sysroot 以 assets
形式携带。在设备上，宿主与目标同为安卓平台。

```gradle
dependencies {
    implementation files('libs/tinyc-android-release.aar')
}
```

```java
TinycAssets.Paths paths = TinycAssets.init(context);  // 调一次：把 assets 解压到 filesDir

try (Tinyc t = Tinyc.create(paths)) {
    t.setOutputType(Tinyc.OUTPUT_OBJ);
    t.addIncludePath("/data/user/0/com.example/files/mylib/include");
    t.compileString("int answer(void) { return 42; }");
    t.outputFile(new File(dir, "answer.o").getAbsolutePath());
}

int rc = Tinyc.runCli("-o", outPath, "-I", includeDir, sourcePath);
```

IO 是 `TinycIO`（流或字节块回调）。动态添加与其它包规则相同，
路径指应用沙盒内可读的真实文件。

### 浏览器 —— `tinyc-web`

把编译器编成 WebAssembly，每个目标一份模块，配 JavaScript 胶水
层与虚拟文件系统。宿主就是浏览器本身。

```
npm install tinyc-web
```

```js
import { loadTinyc } from 'tinyc-web';

const tinyc = await loadTinyc({ target: 'x86_64-linux-gnu', sysroot: true });
await tinyc.addFile('/src/main.c', 'int main(void) { return 0; }');
tinyc.callMain(['-c', '/src/main.c', '-o', '/out/main.o']);
const bytes = tinyc.exportFile('/out/main.o');   // Uint8Array
```

它还发布为单个经典脚本文件 `tinyc-web.js`，定义全局 `TinycWeb`，
API 与模块版相同——可直接经 CDN 引用：

```html
<script src="https://cdn.jsdelivr.net/npm/tinyc-web@0.1.0/tinyc-web.js"></script>
```

只有请求的目标模块会被按需加载。文件按内容管理：写用 `addFile` /
`addFileSync` / `addFileFromUrl`，读用 `readFile` / `readTextFile`
（编译产物用 `exportFile` / `exportBlob`），列举用
`listFiles(prefix)`，删用 `removeFile` / `clearFiles()`；
`await tinyc.whenIdle()` 是排队中的写入全部落盘的确认点。文件
内容持久化在 IndexedDB（每个目标一个库）；内存里只留路径索引与
一小份内容缓存。编译器模块自身的文件系统是 WasmFS：在 Web
Worker 里，其工作集挂在 OPFS（浏览器在磁盘上的源私有文件系统）
上（`tinyc.fsBackend === 'opfs'`）；在页面上下文里留在内存
（`'memory'`）。

运行遵循 run 规则、宿主为浏览器：只有默认目标 `wasm32-wasi` 的
产物能被浏览器执行。`tinyc.runWasmFile(path, options)` 运行已在
虚拟文件系统里的产物；`state.run(...)` 与
`callMain(['-run', ...])` 走同一个运行器。其它目标只能编译成
文件，对其发 run 请求会被以 `TinycRunNotSupportedError` 拒绝；
`tinyc.canRun` 报告一个实例属于哪种情况。程序若导入内置 WASI 与
libc 垫片之外的符号，由注册为 shim 的 JavaScript 函数提供
（`registerShim` / `registerShims`，分单次运行、实例、包级三层）。
另有 `TinycWorker` 接口（`tinyc-web/worker`）把实例放进 Web
Worker 里运行。

## 已知限制

- **macOS 上的动态添加库：** TCC 只能加载它自己产出的目标文件
  与归档。clang / Xcode 产出的 `.o` 会被以 `unrecognized file
  type` 拒绝；clang 产出的 `.a` 里的 BSD 归档索引（`__.SYMDEF`）
  不被识别，其成员不会被拉出。系统库不受影响。
- **浏览器页面上下文：** 模块文件系统后端在那里是内存；OPFS
  磁盘后端只在 Web Worker 里可用。（包自己的虚拟文件系统在两种
  上下文里都持久化到 IndexedDB。）
- **以标准输入为源码：** 把 `-` 作为输入文件名时读的是进程的
  真实 stdin；这条路径不经 TCCIO 的 `read_in`。

## 从源码构建

```
bash build.sh            # 全目标矩阵 -> build/dist/<目标>/
bash build.sh host       # 宿主变体（目标 == 宿主）-> build/dist-host/
bash tests/run_all.sh    # 内核、IO、矩阵与各包测试
```

每个目标落在 `build/dist/<目标>/`，布局为：

```
bin/tinyc          双形态文件（bin/tcc 是指向它的符号链接）
include/           libtcc.h, tinyc_io.h
lib/tcc/           libtcc1.a 与 TCC 自带头文件（lib/tcc/include/）
manifest.json      目标、ISA 基线、outputs、文件位置
```

每个包在 `packages/<名>/` 下有自己的构建入口与 README。构建
浏览器包另需已激活的 Emscripten SDK（`emcc` 在 `PATH` 上）。

## 许可证

LGPL-2.1-or-later，与 TCC 本身的许可证相同。
