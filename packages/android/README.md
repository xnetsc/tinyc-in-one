# TinyC 安卓包（AAR）

把 TCC 编译器以 AAR 形式带进 Android 应用：应用在进程内调
`com.tinyc.Tinyc` 就能编译 C 代码，也能以命令行语义一次性运行
（`Tinyc.runCli`）。原生侧是双形态文件的安卓变体 `libtinyc.so`
（libtcc 内核 + JNI 入口），目标 triple 与 ABI 对应：
arm64-v8a → `aarch64-linux-android`，x86_64 → `x86_64-linux-android`，
host 与 target 同为安卓，sysroot 用 bionic 的头与库。

## 集成

Gradle（app 模块）：

```gradle
dependencies {
    implementation files('libs/tinyc-android-release.aar')
}
```

AAR 只带 arm64-v8a 与 x86_64 两个 ABI；应用的 minSdk 需不低于 24。

初始化（Application 或首个使用点调一次）：

```java
TinycAssets.Paths paths = TinycAssets.init(context);
```

`init` 做两件事：把当前 ABI 对应 triple 的 toolchain 与 sysroot 从
assets 解压到 `filesDir/tinyc/<version>/<triple>/`（assets 不是真实
文件，TCC 读不了，必须先落地；已解压过会按标记直接复用），并记录
`libtinyc.so` 的真实路径供 `runCli` 填 `argv[0]`。

## 用法

会话式 API（与 libtcc 的 `tcc_*` 一一对应）：

```java
try (Tinyc t = Tinyc.create(paths)) {
    t.setOutputType(Tinyc.OUTPUT_OBJ);
    t.addIncludePath("/data/user/0/com.example/files/mylib/include");
    t.compileString("int answer(void) { return 42; }");
    t.outputFile(new File(dir, "answer.o").getAbsolutePath());
}
```

命令行语义（对应双形态文件的 `tinyc_main` 入口，返回退出码，
全进程串行）：

```java
int rc = Tinyc.runCli("-o", outPath, "-I", includeDir, sourcePath);
```

IO（默认直达真实 stdio；可按通道换成流或回调，回调收到的是原始
字节块）：

```java
TinycIO io = TinycIO.of(byteArrayOutputStream, byteArrayOutputStream, null);
t.setIO(io);              // 会话式
Tinyc.runCli(io, "-E", sourcePath);   // 命令行式
```

动态添加头与库就是运行时给本地真实路径，不拷贝、不打包：目录走
`addIncludePath` / `addLibraryPath`，单个库文件走 `addFile`，单个头
文件把其父目录加进 `addIncludePath`。路径指应用沙盒内可读的真实
文件。用户路径排在内置 sysroot 之前，同名头以用户版本为准：
`Tinyc.create(paths)` 绑定的 sysroot 路径在第一次编译动作前才注册，
此前加的用户路径天然在前。

注意事项：

- `Tinyc.run(...)` 与 `runCli` 里的 `-run` 是把编译出的代码在本
  进程内运行，目标程序的 exit 与崩溃归宿主应用。
- 单个 `Tinyc` 实例不是线程安全的，不要跨线程共用；`runCli` 内部
  已串行。
- IO 回调在发起编译的线程上同步执行；回调抛出的异常由 JNI 层存下，
  原生调用返回后在 Java 侧原样抛出，不穿过 C 栈。

## 包内结构

```
src/main/
├─ java/com/tinyc/
│  ├─ Tinyc.java          会话式 API + runCli
│  ├─ TinycNative.java    JNI 声明（包内私有）
│  ├─ TinycIO.java        IO 接口与流适配
│  ├─ TinycAssets.java    assets 解压初始化（包内唯一依赖 android.* 的类）
│  ├─ TinycArgs.java      argv 组装（纯逻辑）
│  └─ TinycException.java
├─ jni/
│  ├─ tinyc_jni.c         JNI 胶水：转发 tcc_* / tinyc_main，IO 回调跳板
│  ├─ tinyc_kernel_adapter.c  内核适配层，与 JNI 胶水一同编入 libtinyc.so
│  ├─ tinyc_kernel.h      内核新增声明（TCCIO、tinyc_main）
│  ├─ CMakeLists.txt      内核构建把 JNI 胶水编入 libtinyc.so 时用
│  └─ Android.mk          同上的 ndk-build 版
├─ jniLibs/<abi>/         libtinyc.so（arm64-v8a、x86_64），由 native-libs 构建放入
└─ assets/
   ├─ toolchain/<triple>/   TCC 自带头、libtcc1.a 与 runmain.o
   └─ sysroot/<triple>/     bionic 系统头与库的目录（include/、lib/）
```

## 构建

`jniLibs` 里的 `libtinyc.so` 与 `assets/toolchain` 的实际内容是构建
产物：源码树里只有目录布局，由 `native-libs` 模式用 NDK 生成
（内核 unity 构建 + JNI 胶水，每个 ABI 一个 .so；同时把 TCC 自带头、
libtcc1.a 与 runmain.o staged 进 toolchain assets；toolchain assets
由占位变为实际内容时，它会递增 `TinycAssets.ASSETS_VERSION`，
设备端据此弃用旧解压目录）。组装 AAR 前先跑一次：

```sh
./build.sh native-libs   # 需要 NDK（NDK_HOME / ANDROID_NDK_HOME）
./build.sh               # = gradle assembleRelease，产物在 build/outputs/aar/
```

Gradle 构建配置：JDK 17、Android SDK platform 35、AGP 8.6.1。

辅助校验（不需要设备）：

```sh
./build.sh jvm-test   # 对 android.jar 编译全部 Java 源码、跑 JVM 纯逻辑
                      # 测试、javac -h 生成 JNI 头并核对胶水实现、
                      # 有 NDK 时对 arm64-v8a 与 x86_64 各做一次 JNI 胶水
                      # 编译检查；输出留档到 test-output.txt
./build.sh manual-aar # 用 javac + aapt2 + zip 按 AAR 布局组装 .aar，
                      # 产物路径与 Gradle 构建相同
```

设备侧测试在 `src/androidTest/`（`connectedAndroidTest`），需要真机
或模拟器；`consumer-test/` 是一个独立的消费者应用模块，它的设备测试
以最终用户的方式集成 AAR 并调用公开 API。
