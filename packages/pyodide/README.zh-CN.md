# tinyc 的 Pyodide 版

[English](README.md) | **简体中文**

TinyC（TCC）在 Pyodide 里的嵌入式编译器包。本包自带 tinyc-web
的 JavaScript 构建、每个目标一份的编译器信封与各目标的
sysroot，并自行完成全部加载：`import tinyc_web` 即完成接入
——不访问网络、不要求宿主页面预加载、不需要用户写任何
JavaScript 胶水。

API 与原生 `tinyc` 包对齐（`State`、`run_cli`、
`run_cli_capture`、`IO`、`TinyCError`、`TCC_OUTPUT_*`
常量）。底座是 tinyc-web 的 JavaScript `TinycState`，经
Pyodide 代理握住。

## 安装

在本目录构建 wheel，再装入 Pyodide 环境（例如用
`micropip`）：

```
python -m pip wheel . -w dist
```

## 快速上手

```python
import tinyc_web

with tinyc_web.State() as s:
    s.compile_string("int add(int a, int b) { return a + b; }")
    add = s.function("add")
    assert add(19, 23) == 42

code, out = tinyc_web.run_cli_capture(["--version"])
```

在页面主线程上本包走异步档：同样的调用返回可等待对象，
使用时加 `await`（`await s.compile_string(...)`、
`add = await s.function("add")`、`await add(19, 23)`）。
在 worker 里走同步档，上面的写法直接可用。档位在 import
时判定一次。

同步档用 Pyodide 的 `run_sync` 等待初始化完成，因此第一个
触碰本包的 Python 入口必须是 promising 入口——用
JavaScript 的说法，经 `callPromising` 调用它（或使用启用
JSPI 的 Pyodide 构建）。异步档没有这个要求。

## 目标

默认目标是 `wasm32-wasi`：代码编译成 WebAssembly 并就地
运行。其它自带目标用于交叉编译（例如产出 `x86_64-linux-gnu`
的 ELF 目标文件）。import 前可用 `$TINYC_TARGET_TRIPLE`
选择目标；`tinyc_web.bundled_targets()` 列出包内携带的
目标，`tinyc_web.target_triple()` 返回当前生效的目标。

## 文件与 IO

编译器在 tinyc-web 的虚拟文件系统里工作。`compile_file`
点名的文件、`run_cli` 的输入与 `-o` 产物会在 Python 文件
系统与虚拟文件系统之间自动搬运：输入在虚拟侧缺失时
上传，产物在调用后写回。

`IO` 与原生包同一表面——`stdio`、`null`、`buffer`、
`file`、`callback`——在 Python 表面上传输字节。某个
state 的 IO 接收该 state 的诊断与它运行的程序的输出；
`run_cli` 的 IO 还接收编译器自身的输出（`--version`、
`-E` 等）。与原生包的差异：

- `IO.fd(...)` 在这里没有对应物，调用即抛 `TinyCError`。
- `State.function(...)` 只承载数值参数与返回值；更丰富的
  签名走 `State.run` 或 `run_cli`。
- 这里没有宿主二进制，因此没有 `host_triple`、
  `native_path`、`dist_dir`、`resolve_native`；
  `assets_dir()` 指向包内自带文件。

## 包布局

```
tinyc_web/
├─ __init__.py       API 表面
├─ _bootstrap.py     档位判定与自举初始化
└─ assets/           tinyc-web 的 JS 构建、编译器信封与
                     各目标 sysroot
bundle_assets.py     从 ../browser 重建 assets/
tests/               共用用例表与两档运行器
```

`bundle_assets.py` 从同级的 tinyc-web 包当前构建重新生成
`assets/`（其 JavaScript 源码现编打包；编译器与 sysroot
信封取自它的资产文件）。`../browser` 里尚未构建出编译器
模块的目标会被跳过。

## 测试

`tests/cases.py` 是一份两档共用的用例表：

```
cd tests && npm install        # 提供 pyodide
node run_node.mjs               # 同步档，Node worker
node run_page.mjs               # 异步档，Chromium 页测
```
