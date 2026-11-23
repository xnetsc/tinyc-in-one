# libtinyc.so 占位说明（x86_64）

本目录在发布构建中放 `libtinyc.so`：目标 triple 为
`x86_64-linux-android` 的双形态文件安卓变体（libtcc 内核 + 本包
`src/main/jni/tinyc_jni.c` 的 JNI 入口，见 `src/main/jni/CMakeLists.txt`），
主要供模拟器使用。

内核由双形态内核构建产出后拷入本目录，AAR 打包原样收进
`jni/x86_64/libtinyc.so`。当前仓库里只有占位：没有真实 .so 时
Java 层编译与 AAR 组装可以完成，但运行时 `System.loadLibrary("tinyc")`
会失败，设备侧功能不可用。
