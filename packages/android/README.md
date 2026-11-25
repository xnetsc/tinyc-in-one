# TinyC Android package (AAR)

**English** | [简体中文](README.zh-CN.md)

Bring the TCC compiler into an Android app as an AAR: the app calls
`com.tinyc.Tinyc` in-process to compile C code, and can also run it
with command-line semantics in one call (`Tinyc.runCli`). On the
native side is the Android variant of the dual-form file,
`libtinyc.so` (the libtcc kernel + a JNI entry), with target triples
mapped to ABIs: arm64-v8a → `aarch64-linux-android`, x86_64 →
`x86_64-linux-android`. Host and target are both Android; the
sysroot uses bionic's headers and libraries.

## Integration

Gradle (app module):

```gradle
dependencies {
    implementation files('libs/tinyc-android-release.aar')
}
```

The AAR carries only the arm64-v8a and x86_64 ABIs; the app's minSdk
must be at least 24.

Initialization (call once, in Application or at the first point of
use):

```java
TinycAssets.Paths paths = TinycAssets.init(context);
```

`init` does two things: it extracts the toolchain and sysroot for
the current ABI's triple from assets into
`filesDir/tinyc/<version>/<triple>/` (assets are not real files and
TCC cannot read them, so they must land on disk first; an existing
extraction is reused via its marker), and it records the real path
of `libtinyc.so` so `runCli` can fill `argv[0]`.

## Usage

Session-style API (corresponding one-to-one to libtcc's `tcc_*`):

```java
try (Tinyc t = Tinyc.create(paths)) {
    t.setOutputType(Tinyc.OUTPUT_OBJ);
    t.addIncludePath("/data/user/0/com.example/files/mylib/include");
    t.compileString("int answer(void) { return 42; }");
    t.outputFile(new File(dir, "answer.o").getAbsolutePath());
}
```

Command-line semantics (corresponding to the dual-form file's
`tinyc_main` entry; returns the exit code, serialized process-wide):

```java
int rc = Tinyc.runCli("-o", outPath, "-I", includeDir, sourcePath);
```

IO (defaults to the real stdio; each channel can be replaced with a
stream or a callback, and callbacks receive raw byte chunks):

```java
TinycIO io = TinycIO.of(byteArrayOutputStream, byteArrayOutputStream, null);
t.setIO(io);              // session style
Tinyc.runCli(io, "-E", sourcePath);   // command-line style
```

Adding headers and libraries dynamically means handing over local
real paths at runtime — no copying, no packaging: directories go
through `addIncludePath` / `addLibraryPath`, a single library file
through `addFile`, and a single header file by adding its parent
directory to `addIncludePath`. Paths point at real files readable
inside the app sandbox. User paths come before the bundled sysroot,
so a user header wins on a name clash: the sysroot paths bound by
`Tinyc.create(paths)` are registered only just before the first
compile action, so user paths added earlier are naturally in front.

Notes:

- `Tinyc.run(...)` and `-run` inside `runCli` run the compiled code
  inside this process; the target program's exit and crashes belong
  to the host app.
- A single `Tinyc` instance is not thread-safe; do not share one
  across threads. `runCli` is serialized internally.
- IO callbacks execute synchronously on the thread that started the
  compile; an exception thrown by a callback is stored by the JNI
  layer and rethrown as-is on the Java side after the native call
  returns — it never crosses the C stack.

## Package contents

```
src/main/
├─ java/com/tinyc/
│  ├─ Tinyc.java          session-style API + runCli
│  ├─ TinycNative.java    JNI declarations (package-private)
│  ├─ TinycIO.java        IO interface and stream adapters
│  ├─ TinycAssets.java    assets extraction/init (the only class in the package that depends on android.*)
│  ├─ TinycArgs.java      argv assembly (pure logic)
│  └─ TinycException.java
├─ jni/
│  ├─ tinyc_jni.c         JNI glue: forwards tcc_* / tinyc_main, IO callback trampoline
│  ├─ tinyc_kernel_adapter.c  kernel adapter layer, compiled into libtinyc.so together with the JNI glue
│  ├─ tinyc_kernel.h      the kernel's added declarations (TCCIO, tinyc_main)
│  ├─ CMakeLists.txt      used when the kernel build compiles the JNI glue into libtinyc.so
│  └─ Android.mk          the ndk-build version of the same
├─ jniLibs/<abi>/         libtinyc.so (arm64-v8a, x86_64), placed there by the native-libs build
└─ assets/
   ├─ toolchain/<triple>/   TCC's own headers, libtcc1.a and runmain.o
   └─ sysroot/<triple>/     the bionic system headers and libraries (include/, lib/)
```

## SIMD intrinsics

The bundled headers include SIMD intrinsic headers for both
Android targets, in the sysroot's `lib/tcc/include/` — the
compiler's default include path, so there is nothing extra to
install and no `-I` to add:

- `aarch64-linux-android`: `#include <arm_neon.h>` — the ACLE
  NEON surface; the dot-product family needs
  `-march=armv8.2-a+dotprod` or a later baseline.
- `x86_64-linux-android`: `#include <immintrin.h>` — SSE
  through the AVX-512 subsets, gated by `-march` (`x86-64-v3`
  for AVX2 level, `x86-64-v4` for AVX-512).

The root README's "SIMD intrinsics" section lists, per header,
which instructions are covered and which extensions are not
provided.

## Building

The `libtinyc.so` in `jniLibs` and the actual contents of
`assets/toolchain` are build products: the source tree carries only
the directory layout, and the `native-libs` mode generates them
with the NDK (the kernel unity build + the JNI glue, one .so per
ABI; it also stages TCC's own headers, libtcc1.a and runmain.o into
the toolchain assets; when the toolchain assets change from
placeholders to real content, it increments
`TinycAssets.ASSETS_VERSION`, and devices discard the old extraction
directory accordingly). Run it once before assembling the AAR:

```sh
./build.sh native-libs   # requires an NDK (NDK_HOME / ANDROID_NDK_HOME)
./build.sh               # = gradle assembleRelease, product in build/outputs/aar/
```

Gradle build configuration: JDK 17, Android SDK platform 35, AGP
8.6.1.

Auxiliary checks (no device needed):

```sh
./build.sh jvm-test   # compiles all Java sources against android.jar, runs the JVM pure-logic
                      # tests, generates JNI headers with javac -h and checks them against the glue,
                      # and with an NDK does one JNI-glue compile check per ABI (arm64-v8a and x86_64);
                      # output is archived to test-output.txt
./build.sh manual-aar # assembles an .aar from javac + aapt2 + zip in the AAR layout,
                      # at the same output path as the Gradle build
```

Device-side tests live in `src/androidTest/`
(`connectedAndroidTest`) and need a real device or an emulator;
`consumer-test/` is a standalone consumer app module whose device
tests integrate the AAR the way a final user would and call the
public API.
