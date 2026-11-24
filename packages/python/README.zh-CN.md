# tinyc for Python

[English](README.md) | **简体中文**

在 Python 里把 TinyC（TCC）当作可嵌入的编译器使用。本包按目标打包
一份双形态原生文件——同一个文件既是 `tcc` 命令、也是 libtcc 库——
连同它需要的头文件与库文件。绑定只用 `ctypes`，没有额外原生件。

## 安装

```
pip install tinyc
```

wheel 按宿主平台标签发布（`manylinux` x86_64 / aarch64、`win_amd64`、
`macosx` arm64 / x86_64）；每个 wheel 带该宿主的目标变体。wheel 内
`tinyc` 包用的是所有原生包共用的统一布局：

```
tinyc/
├─ packaging-manifest.json     包内实际包含的 (host, target) 组合清单
├─ natives/<host-triple>/toolchain/<target-triple>/tinyc[.exe]
└─ sysroot/<target-triple>/    每目标一份，所有宿主共享
```

宿主 triple 由平台探测（`$TINYC_HOST_TRIPLE` 可覆盖探测，
`$TINYC_TRIPLE` 是它的别名）；目标缺省等于宿主
（`$TINYC_TARGET_TRIPLE` 可指定交叉目标）。`$TINYC_DIST` 覆盖查找
所用的包根，`$TINYC_NATIVE` 直接指向双形态文件本身，绕过清单查找。
wheel 内没有的组合会报错，并指明宿主、目标与包内可用的组合。宿主
与目标都只提供 64 位。

宿主/目标矩阵与 run 规则（编译出的程序只在目标就是宿主自身平台时
才能在进程内运行；交叉组合只编译成文件）见
[仓库 README](../../README.zh-CN.md)。

## 命令行式

```python
import tinyc

code = tinyc.run_cli(["hello.c", "-o", "hello"])   # 退出码
code, output = tinyc.run_cli_capture(["--version"])  # 捕获的输出
```

`run_cli` 把 argv[0] 填成原生文件自身的路径，使编译器能找到它自带
的头与库。宿主进程从不被结束；返回值是编译器的退出码。

## 会话式 API

```python
import ctypes, tinyc

with tinyc.State() as s:
    s.add_include_path("/path/to/headers")     # 本地路径，原样使用
    s.add_file("/path/to/libfoo.a")
    s.compile_string("int add(int a, int b){ return a + b; }")
    add = s.function("add", ctypes.c_int, ctypes.c_int, ctypes.c_int)
    assert add(19, 23) == 42
```

编译错误抛 `tinyc.TinyCError`，同时收集在 `s.errors` 里，不打印。

## 输出（IO）

编译器输出走 TCCIO 接口。缺省是进程的真实 stdout/stderr。其它
选择：

```python
tinyc.run_cli(args, io="capture")                      # 经 run_cli_capture
tinyc.run_cli(args, io=tinyc.IO.file("build.log"))     # 写到文件
tinyc.run_cli(args, io=tinyc.IO.callback(out=my_fn))   # my_fn(bytes)
tinyc.run_cli(args, io=open("log", "wb"))              # 任何二进制文件对象
```

## 测试

```
python3 bundle_native.py   # 先跑 ../../build.sh
python3 -m unittest discover -s tests
```
