# tinyc for Node.js

[English](README.md) | **简体中文**

在 Node.js 里把 TinyC（TCC）当作可嵌入的编译器使用。本包按目标打包
一份双形态原生文件——同一个文件既是 `tcc` 命令、也是 libtcc 库——
连同它需要的头文件与库文件，并用 koffi 驱动它。

## 安装

```
npm install @xnetx/tinyc
```

你所在宿主的原生文件放在按宿主分的子包
`@xnetx/tinyc-native-<host-triple>` 里，npm 会把它作为可选依赖自动
装上。主包自带 staged 文件时也可独立工作。两种情况下，包根之下的
布局都是所有原生包共用的统一布局：

```
<package root>/
├─ packaging-manifest.json     包内实际包含的 (host, target) 组合清单
├─ natives/<host-triple>/toolchain/<target-triple>/tinyc[.exe]
└─ sysroot/<target-triple>/    每目标一份，所有宿主共享
```

查找根的顺序是：`$TINYC_DIST`、已安装的子包、然后本包自身。宿主
triple 由平台探测（`$TINYC_HOST_TRIPLE` 可覆盖探测，`$TINYC_TRIPLE`
是它的别名）；目标缺省等于宿主（`$TINYC_TARGET_TRIPLE` 可指定交叉
目标）。`$TINYC_NATIVE` 直接指向双形态文件本身，绕过清单查找。包内
没有的组合会报错，并指明宿主、目标与包内可用的组合。宿主与目标都
只提供 64 位。

宿主/目标矩阵与 run 规则（编译出的程序只在目标就是宿主自身平台时
才能在进程内运行；交叉组合只编译成文件）见
[仓库 README](../../README.zh-CN.md)。

## 命令行式

```js
const tcc = require('@xnetx/tinyc');

const code = tcc.runCli(['hello.c', '-o', 'hello']);  // 退出码
const [code2, output] = tcc.runCliCapture(['--version']);
```

`runCli` 把 argv[0] 填成原生文件自身的路径，使编译器能找到它自带的
头与库。宿主进程从不被结束；返回值是编译器的退出码。

## 会话式 API

```js
const s = new tcc.State();
s.addIncludePath('/path/to/headers');     // 本地路径，原样使用
s.addFile('/path/to/libfoo.a');
s.compileString('int add(int a, int b){ return a + b; }');
const add = s.function('add', 'int', ['int', 'int']);
console.log(add(19, 23)); // 42
s.close();
```

编译错误抛 `tcc.TinyCError`，同时收集在 `s.errors` 里，不打印。

## 宿主函数

`addSymbol` 把一个 JS 函数注册成编译产物可按名字直接调用的宿主
函数（`tcc_add_symbol`）；签名写法与 `State#function` 一致：

```js
s.addSymbol('host_mul', (a, b) => a * b, 'int', ['int', 'int']);
s.compileString('extern int host_mul(int, int);\n' +
  'int use_host(void){ return host_mul(6, 7); }');
```

作用域：注册的是回调进本进程的指针，只在进程内运行成立——MEMORY
输出 relocate 之后，经 `function()` 触达。文件输出不支持：注册
过宿主符号后，`outputFile()` 直接报错，不会产出把本进程地址写死
的文件。注册由 State 持有到 `close()`。

## 输出（IO）

编译器输出走 TCCIO 接口。缺省是进程的真实 stdout/stderr。其它
选择：

```js
tcc.runCli(args, { io: 'capture' });                    // 经 runCliCapture
tcc.runCli(args, { io: tcc.IO.file('build.log') });     // 写到文件
tcc.runCli(args, { io: tcc.IO.callback({ out: fn }) }); // fn(Buffer)
tcc.runCli(args, { io: process.stdout });               // 任何 Writable
```

IO 回调在调用线程上同步执行，与编译器的同步 IO 调用相匹配。

## SIMD 内建函数

自带头文件集包含每个目标的 SIMD 内建头，位于 sysroot 的
`lib/tcc/include/`——编译器的默认头文件搜索路径，不需要另外
安装，也不需要加 `-I`：

- AArch64 目标：`#include <arm_neon.h>`——ACLE NEON 表面；
  点积家族需 `-march=armv8.2-a+dotprod` 或更高基线。
- x86-64 目标：`#include <immintrin.h>`——SSE、SSE2、SSSE3、
  SSE4、AVX、AVX2、FMA、F16C 与 AVX-512 子集，由 `-march`
  门控（`x86-64-v3` 为 AVX2 级别，`x86-64-v4` 为 AVX-512）。
- riscv64 目标：`#include <riscv_vector.h>`——RVV 1.0 API 的
  定长形式；要求 `-march` 带 V 扩展，如 `rv64gcv_zvl256b`。

每个头具体有哪些指令、哪些扩展不提供，见根 README 的
「SIMD 内建函数」一节。

## 测试

```
node bundle_native.js   # 先跑 ../../build.sh
npm test
```
