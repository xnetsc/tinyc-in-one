# TinyC macOS 宿主测试包

在 Mac 上验证 TinyC 双形态内核的 macOS 宿主形态：宿主与目标同为
arm64-apple-darwin，TCC 本体由本机 clang 构建。

## 执行

把 `tinyc-mac-test.tar.gz` 拷到 Mac 上，解压后在终端执行一行：

```
tar xzf tinyc-mac-test.tar.gz && cd mac-test-bundle && bash run.sh
```

全部输出汇总到同目录的 `result.log`，结尾有一行 `OVERALL:` 总体结论。
把 `result.log` 带回即可分析，脚本中途某一步失败不会中断后面的独立步骤。

## 包内结构

- `src/`：上游 TCC 0.9.28rc 的 pristine 源码
- `patches/apply_patches.py`：本项目的核心补丁（IO 接口、exit 审计、
  `tinyc_main` 双入口），构建时应用到源码副本上
- `core/`：TCCIO 实现（`tcc_io.c` / `tinyc_io.h`）
- `build-mac.sh`：构建脚本
- `run-tests.sh`：测试脚本
- `glue/`：解段器与 dlopen / ctypes 测试程序
- `testdata/`：测试用源码（hello.c、动态添加示例的头与库源码）

## 构建做什么（build-mac.sh）

1. 把 `src/` 拷成工作副本，应用补丁，再用上游方式
   `./configure && make && make install` 编出原生 osx TCC，
   得到 `libtcc1.a` 与 TCC 自带头文件。
2. 用 clang 把补丁后的 `tcc.c`（unity build，含 libtcc 全部逻辑）
   直编成库形态 `libtinyc.dylib`，导出 `tinyc_main` 与全部 `tcc_*`。
3. 再直编成可执行文件，并把上面那份 dylib 作为 Mach-O 的
   `__TINYC,__dylib` 段预置进可执行文件本体，得到发行形态的
   双形态文件 `dist/bin/tinyc`。macOS 上 `MH_PIE` 与 `MH_DYLIB`
   互斥，可执行文件本身 dlopen 不了，库形态靠这段预置 dylib 实现。
4. 编出胶水解段器 `extract_dylib`：它读双形态文件、取出
   `__TINYC,__dylib` 段的字节写成缓存目录里的 `libtinyc.dylib`，
   宿主再 dlopen 那份文件。
5. 编出动态添加测试用的示例本地库 `libmylib.a`。

## 每项测试验什么（run-tests.sh）

- **双形态可执行**：`dist/bin/tinyc --version` 与 `-dumpmachine`
  有正确输出，证明可执行形态可直接拉起。
- **解段**：解段器从双形态文件取出的 dylib 是 Mach-O dylib，
  且与构建出的 dylib 逐字节一致，证明预置段完整无损。
- **编译成 Mach-O 并运行**：用双形态文件把 `hello.c` 编成
  Mach-O 可执行，`file` 核对格式，运行核对输出
  `hello from tinyc: 42`。若产物需要 ad-hoc 签名才能运行，
  脚本自动 `codesign -s -` 后重试，并把这一事实以 `FACT:` 行
  记入日志。
- **`-run`**：双形态文件以 `-run` 在进程内直接运行 `hello.c`
  （JIT 路径），核对输出一致。
- **dlopen 路径**：一个 C 宿主程序 dlopen 解出的 dylib，依次调
  `tinyc_main` 编译、`tinyc_main -dumpversion`（补丁前这条路径
  以 `exit(0)` 结束进程）、`tcc_new`/`tcc_compile_string`/
  `tcc_output_file` 的细粒度 API 编译，最后打印 `HOST-ALIVE`
  ——到达这一行证明任何一次调用都没有把宿主进程带走；
  两个产物可执行文件也都会实际运行并核对输出。
- **Python ctypes 路径**：系统 `python3` 用 ctypes 加载同一份
  dylib 调 `tinyc_main` 完成一次编译，Python 进程存活
  （打印 `CTYPES-ALIVE`），产物可运行。
- **动态添加**：编译时以 `-I`/`-L` 指向包内的示例本地目录
  （自定义头 `mylib.h` 与 `libmylib.a`，不拷贝进 sysroot），
  链接 `-lmylib` 编出程序并运行，核对输出 `dynadd mylib: 42`。

## 结果怎么看

`result.log` 中每个检查一行 `PASS:` / `FAIL:`，构建步骤失败记为
`BUILD-FAIL:`，每行附近附有原始输出。末尾 `OVERALL:` 行汇总
通过数、失败数与构建失败步数。
