# TinyC Java 服务器包

[English](README.md) | **简体中文**

TinyC 编译器的 Java 绑定：用 Panama FFM 直接调 libtcc 的 C ABI，不经 JNI，
不带额外原生件（绑定层是纯 Java，原生侧只有 TCC 内核文件本身）。

## 要求

- JDK 22 或更新（Panama FFM 正式版；开发与测试在 JDK 25 上进行）。
- 运行时加 `--enable-native-access=ALL-UNNAMED`（JDK 24+ 对原生访问的要求）。

## Jar 布局

Jar 内是与 npm、Python 包相同的统一布局，按构建宿主打包：一个宿主的
双形态文件 × 构建时 dist 里的全部目标（当前构建为 6 个目标），sysroot
按目标一份、为该宿主的全部组合共用：

```
tinyc.jar
├─ tinyc/*.class                 Panama 绑定、双入口封装、IO 跳板
├─ tinyc-native.properties       版本号
├─ packaging-manifest.json       Jar 内的 (host, target) 组合清单
├─ natives/<host>/toolchain/<target>/tinyc[.exe]
└─ sysroot/<target>/             每目标一份：include/ 与 lib/tcc/
```

首次加载时按探测到的宿主/目标 triple 在清单里查组合，把该组合的
二进制与目标 sysroot 解压到 `~/.cache/tinyc-java/<version>/`，
组合标记完整后复用；运行时目录取清单的 `libPath` 字段，新会话自动
以它调用 `tcc_set_lib_path`。宿主与目标可用环境变量
`TINYC_HOST_TRIPLE` / `TINYC_TARGET_TRIPLE` 覆盖（系统属性
`tinyc.host.triple` / `tinyc.target.triple` 亦可）。宿主与目标均为
64 位，32 位宿主直接报不支持。

## 原生文件来源

`build.sh` 用仓库的 `packaging/stage_layout.py`，把主线构建
`../../build/dist` 里能跑在构建宿主上的目标全部 staged 进 Jar。
host == target 的原生组合不在 dist 里时，构建直接报错退出。

## 用法

会话式 API（细粒度，对应 `tcc_*`）：

```java
try (TccSession s = TinyC.newSession()) {
    s.setOutputType(TinyC.TCC_OUTPUT_MEMORY);
    if (s.compileString("int add(int a, int b) { return a + b; }") != 0) {
        throw new IllegalStateException("compile failed");
    }
    s.relocate();
    MemorySegment fn = s.getSymbol("add");
    MethodHandle mh = TinyC.functionHandle(fn,
            FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
    int v = (int) mh.invokeExact(40, 2);   // 42
}
```

命令行式入口（粗粒度，参数与 tcc 命令行一致，返回退出码）：

```java
int rc = TinyC.runCli("-o", "hello", "hello.c");
```

`runCli` 调原生文件导出的真 main（`tinyc_main` 优先，无则 `main`）：
`argv[0]` 由封装层填原生文件的真实路径；已知运行时目录且参数中没有 `-B` 时
自动补 `-B<运行时目录>`；调用全程持锁串行，返回后冲刷 C stdio。
main 返回即返回，不结束宿主进程。

动态添加本地文件（真实路径直读，不拷贝）：

```java
s.addIncludePath("/home/user/mylib/include");      // 目录
s.addIncludePath("/opt/foo");                       // 单个头文件 /opt/foo/bar.h 取其父目录
s.addLibraryPath("/home/user/mylib/lib");
s.addFile("/home/user/mylib/libfoo.a");             // 单个库文件，不走 -l 搜索
```

开发/测试时可用系统属性绕过 Jar 解压，直接指定本地原生文件：

```
-Dtinyc.native.path=/path/to/tinyc -Dtinyc.lib.path=/path/to/runtime-dir
```

`tinyc.lib.path` 是含 `libtcc1.a` 与 `include/` 的目录；不给时若原生文件
同目录下有 `libtcc1.a` 就用该目录，再否则不自动设置 lib 路径。

## API 对照（libtcc.h）

| Java | C |
|---|---|
| `TinyC.newSession()` / `TccSession.close()` | `tcc_new` / `tcc_delete` |
| `setLibPath` | `tcc_set_lib_path` |
| `setOptions` | `tcc_set_options` |
| `addIncludePath` / `addSysincludePath` | `tcc_add_include_path` / `tcc_add_sysinclude_path` |
| `addLibraryPath` / `addLibrary` / `addFile` | `tcc_add_library_path` / `tcc_add_library` / `tcc_add_file` |
| `defineSymbol` / `undefineSymbol` | `tcc_define_symbol` / `tcc_undefine_symbol` |
| `addSymbol` | `tcc_add_symbol` |
| `setErrorHandler` / `setIO` | `tcc_set_error_func` |
| `TinyC.runCli(io, ...)` | `tinyc_main_io`（io 经 `tcc_io_callback` 组成 TCCIO） |
| `TinyC.runCli(...)` | `tinyc_main` |
| `compileString` | `tcc_compile_string` |
| `compileFile` | `tcc_add_file`（libtcc.h 没有 `tcc_compile_file`） |
| `setOutputType` | `tcc_set_output_type`（常量 `TinyC.TCC_OUTPUT_*`） |
| `outputFile` | `tcc_output_file` |
| `run` | `tcc_run`（目标程序在宿主进程内运行，其 exit 与崩溃归宿主） |
| `relocate` / `getSymbol` | `tcc_relocate` / `tcc_get_symbol` |

## IO

`TccIO` 是三个通道的接口：`writeOut`、`writeErr`、`readIn`，数据以字节传递。
现成实现：`TccIO.stdio()`（System 流）、`TccIO.of(out, err, in)`（流适配，
null 回落 System 流）、`TccIO.capturing()`（内存攒积，out/err 分开取）。

当前接入状态：

- 错误通道：`TccSession.setIO(io)` 或 `setErrorHandler(handler)` 后，
  libtcc 的错误文本经 Panama upcall 跳板以字节交给 `writeErr`。
- 全部三通道：`TinyC.runCli(io, args...)` 把 `writeOut` / `writeErr` /
  `readIn` 的跳板经内核 `tcc_io_callback` 组成回调型 TCCIO，传给
  `tinyc_main_io`，在该次调用期间作为编译器的当前 IO：编译器自身的
  输出（版本、预处理结果等）走 `writeOut`、诊断走 `writeErr`、
  `readIn` 随 TCCIO 一并传给内核。不带 io 的 `runCli(args...)` 走
  `tinyc_main`，输出走进程真实 stdio，与命令行一致。
- 跳板遵守四条规矩：异常不穿 C 边界（抓住存下、原生调用返回后以
  `TccException` 重抛）；字节传递不假设 `\0` 结尾；stub 所在 Arena
  钉到调用结束（会话错误回调钉到 `close()`）；回调在调用线程同步执行。
- 默认不设任何回调：libtcc 的报错走进程真实 stderr，与命令行一致。
- 带 IO 跑 `-run` 时，目标程序在运行期间的 stdin 由 `readIn` 喂入、stdout 与 stderr 分别经 `writeOut` 与 `writeErr` 交付；不带 IO 时目标程序用进程真实 stdio。

## 构建与测试

```sh
bash build.sh        # 产出 dist/tinyc.jar，并编好测试类
bash run-tests.sh    # 三遍：A 系统属性直指本地文件 / B Jar 解压分发模式 / C 宿主模拟
```

宿主模拟（C）由本包清单驱动：模拟的组合在包内则解析并跑全套，不在则
断言错误指名该宿主并列出包内全部组合；32 位宿主单独断言拒绝。

`build.sh` 与 `run-tests.sh` 用同一套规则定位 JDK：按候选逐个核验
主版本号，取第一个 ≥ 22 的，保证编译与运行出自同一个 JDK。
