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
    // 或发布到 Maven 后：implementation 'com.tinyc:tinyc-android:<version>'
}
```

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
│  ├─ tinyc_kernel.h      内核新增声明（TCCIO、tinyc_main），与规格第 3、6 节一致
│  ├─ CMakeLists.txt      内核构建把 JNI 胶水编入 libtinyc.so 时用
│  └─ Android.mk          同上的 ndk-build 版
├─ jniLibs/<abi>/         libtinyc.so 的位置（arm64-v8a、x86_64）
└─ assets/
   ├─ toolchain/<triple>/   TCC 自带头与 libtcc1.a
   └─ sysroot/<triple>/     bionic 系统头与库
```

## 构建

标准路径是 Gradle（JDK 17、Android SDK platform 35、Gradle 8.7）：

```sh
./build.sh            # = gradle assembleRelease，产物在 build/outputs/aar/
```

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
或模拟器，且 jniLibs 里要有真实的 `libtinyc.so`。

## 当前状态

- `jniLibs/` 下是占位说明，没有真实 `libtinyc.so`：内核由双形态
  内核构建产出后拷入（见各 ABI 目录的 PLACEHOLDER.md）。占位状态下
  Java 编译、AAR 组装、JVM 测试都能过，运行时 `loadLibrary` 会失败。
- `assets/` 下只有布局占位（`.gitkeep`），toolchain 与 sysroot 的
  实际内容由构建流水线按 triple 填入；填入时递增
  `TinycAssets.ASSETS_VERSION`。
- 本地已验证：全部 Java 源码对 android.jar（API 35）编译通过；
  JVM 纯逻辑测试 21 项全过；`javac -h` 生成的 21 个 native 方法在
  `tinyc_jni.c` 中全部有实现；JNI 胶水对 aarch64 与 x86_64 两个
  Android 目标各做了一次 NDK clang 编译检查（对上游 libtcc.h 与
  `tinyc_kernel.h` 声明），无警告；手工组装的 .aar 结构完整
  （二进制 AndroidManifest.xml、classes.jar、R.txt、
  aar-metadata.properties、assets 与 jni 目录齐全）。
- Gradle 标准构建在本仓库的 CI 沙箱里跑不起来：Gradle daemon 的
  注册表自检在该文件系统上误判 daemon 已消失，daemon 起来即自停
  （`--no-daemon` 与更换 GRADLE_USER_HOME 均复现）。这是该沙箱的
  环境问题，`build.gradle` 本身按标准 AGP 8.6.1 配置，在正常开发
  机上直接 `./build.sh` 即可。
