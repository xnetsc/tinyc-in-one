var TinycWeb = (() => {
  var __defProp = Object.defineProperty;
  var __getOwnPropDesc = Object.getOwnPropertyDescriptor;
  var __getOwnPropNames = Object.getOwnPropertyNames;
  var __hasOwnProp = Object.prototype.hasOwnProperty;
  var __export = (target, all) => {
    for (var name in all)
      __defProp(target, name, { get: all[name], enumerable: true });
  };
  var __copyProps = (to, from, except, desc) => {
    if (from && typeof from === "object" || typeof from === "function") {
      for (let key of __getOwnPropNames(from))
        if (!__hasOwnProp.call(to, key) && key !== except)
          __defProp(to, key, { get: () => from[key], enumerable: !(desc = __getOwnPropDesc(from, key)) || desc.enumerable });
    }
    return to;
  };
  var __toCommonJS = (mod) => __copyProps(__defProp({}, "__esModule", { value: true }), mod);

  // web-entry.mjs
  var web_entry_exports = {};
  __export(web_entry_exports, {
    DEFAULT_RUN_OUTPUT: () => DEFAULT_RUN_OUTPUT,
    DEFAULT_TARGET: () => DEFAULT_TARGET,
    IndexedDbFsStore: () => IndexedDbFsStore,
    MemoryFsStore: () => MemoryFsStore,
    RTLD_GLOBAL: () => RTLD_GLOBAL,
    RTLD_LAZY: () => RTLD_LAZY,
    RTLD_LOCAL: () => RTLD_LOCAL,
    RTLD_NOW: () => RTLD_NOW,
    SUPPORTED_TARGETS: () => SUPPORTED_TARGETS,
    TARGETS: () => TARGETS,
    TCC_OUTPUT: () => TCC_OUTPUT,
    Tinyc: () => Tinyc,
    TinycDylinkError: () => TinycDylinkError,
    TinycError: () => TinycError,
    TinycMissingExportError: () => TinycMissingExportError,
    TinycMissingImportError: () => TinycMissingImportError,
    TinycProcess: () => TinycProcess,
    TinycRunNotSupportedError: () => TinycRunNotSupportedError,
    TinycState: () => TinycState,
    VFS_CACHE_MAX_BYTES: () => VFS_CACHE_MAX_BYTES,
    VFS_CACHE_MAX_ENTRIES: () => VFS_CACHE_MAX_ENTRIES,
    VIRTUAL_BIN_PATH: () => VIRTUAL_BIN_PATH,
    VirtualFS: () => VirtualFS,
    WasiExit: () => WasiExit,
    canonicalModulePath: () => canonicalModulePath,
    clearTargetModuleCache: () => clearTargetModuleCache,
    createDefaultIO: () => createDefaultIO,
    createFsStore: () => createFsStore,
    createProcess: () => createProcess,
    createShimContext: () => createShimContext,
    createWasiShim: () => createWasiShim,
    default: () => index_default,
    flushIO: () => flushIO,
    getLoadedTargets: () => getLoadedTargets,
    isRunnableTarget: () => isRunnableTarget,
    libraryFromBytes: () => libraryFromBytes,
    loadSysrootManifest: () => loadSysrootManifest,
    loadTinyc: () => loadTinyc,
    loadTinycWithNative: () => loadTinycWithNative,
    makeWrapperFunction: () => makeWrapperFunction,
    manifestToLibrary: () => manifestToLibrary,
    normalizeIO: () => normalizeIO,
    normalizeVfsPath: () => normalizeVfsPath,
    parseImportTypes: () => parseImportTypes,
    readModuleCString: () => readModuleCString,
    readModuleData: () => readModuleData,
    registerGlobalShim: () => registerGlobalShim,
    registerGlobalShims: () => registerGlobalShims,
    runWasmModule: () => runWasmModule,
    runWasmModuleSync: () => runWasmModuleSync,
    sysrootManifest: () => sysrootManifest,
    targetModuleFiles: () => targetModuleFiles,
    unregisterGlobalShim: () => unregisterGlobalShim,
    writeModuleData: () => writeModuleData
  });

  // sysroot-manifests/wasm32-wasi.json
  var wasm32_wasi_default = {
    name: "tinyc-sysroot",
    version: 1,
    target: "wasm32-wasi",
    binPath: "/sysroot/bin/tinyc",
    includeDir: "/sysroot/include",
    includeFiles: [
      {
        path: "/sysroot/include/arm_neon.h",
        source: "include/arm_neon.h"
      },
      {
        path: "/sysroot/include/ctype.h",
        source: "include/ctype.h"
      },
      {
        path: "/sysroot/include/dlfcn.h",
        source: "include/dlfcn.h"
      },
      {
        path: "/sysroot/include/emmintrin.h",
        source: "include/emmintrin.h"
      },
      {
        path: "/sysroot/include/float.h",
        source: "include/float.h"
      },
      {
        path: "/sysroot/include/iso646.h",
        source: "include/iso646.h"
      },
      {
        path: "/sysroot/include/limits.h",
        source: "include/limits.h"
      },
      {
        path: "/sysroot/include/math.h",
        source: "include/math.h"
      },
      {
        path: "/sysroot/include/pthread.h",
        source: "include/pthread.h"
      },
      {
        path: "/sysroot/include/stdalign.h",
        source: "include/stdalign.h"
      },
      {
        path: "/sysroot/include/stdarg.h",
        source: "include/stdarg.h"
      },
      {
        path: "/sysroot/include/stdatomic.h",
        source: "include/stdatomic.h"
      },
      {
        path: "/sysroot/include/stdbool.h",
        source: "include/stdbool.h"
      },
      {
        path: "/sysroot/include/stddef.h",
        source: "include/stddef.h"
      },
      {
        path: "/sysroot/include/stdint.h",
        source: "include/stdint.h"
      },
      {
        path: "/sysroot/include/stdio.h",
        source: "include/stdio.h"
      },
      {
        path: "/sysroot/include/stdlib.h",
        source: "include/stdlib.h"
      },
      {
        path: "/sysroot/include/stdnoreturn.h",
        source: "include/stdnoreturn.h"
      },
      {
        path: "/sysroot/include/string.h",
        source: "include/string.h"
      },
      {
        path: "/sysroot/include/tccdefs.h",
        source: "include/tccdefs.h"
      },
      {
        path: "/sysroot/include/tgmath.h",
        source: "include/tgmath.h"
      },
      {
        path: "/sysroot/include/varargs.h",
        source: "include/varargs.h"
      },
      {
        path: "/sysroot/include/wasm_simd128.h",
        source: "include/wasm_simd128.h"
      },
      {
        path: "/sysroot/include/xmmintrin.h",
        source: "include/xmmintrin.h"
      },
      {
        path: "/sysroot/include/tcclib.h",
        source: "tcclib.h"
      }
    ],
    libDir: "/sysroot/lib",
    libFiles: [
      {
        binary: true,
        path: "/sysroot/lib/libtcc1.a",
        source: "lib/wasm32-wasi/libtcc1.a"
      }
    ],
    outputs: {
      module: {
        format: "WebAssembly",
        ext: ".wasm"
      }
    }
  };

  // sysroot-manifests/x86_64-linux-gnu.json
  var x86_64_linux_gnu_default = {
    name: "tinyc-sysroot",
    version: 1,
    target: "x86_64-linux-gnu",
    binPath: "/sysroot/bin/tinyc",
    includeDir: "/sysroot/include",
    includeFiles: [
      {
        path: "/sysroot/include/float.h",
        source: "include/float.h"
      },
      {
        path: "/sysroot/include/iso646.h",
        source: "include/iso646.h"
      },
      {
        path: "/sysroot/include/limits.h",
        source: "include/limits.h"
      },
      {
        path: "/sysroot/include/math.h",
        source: "include/math.h"
      },
      {
        path: "/sysroot/include/stdalign.h",
        source: "include/stdalign.h"
      },
      {
        path: "/sysroot/include/stdarg.h",
        source: "include/stdarg.h"
      },
      {
        path: "/sysroot/include/stdatomic.h",
        source: "include/stdatomic.h"
      },
      {
        path: "/sysroot/include/stdbool.h",
        source: "include/stdbool.h"
      },
      {
        path: "/sysroot/include/stddef.h",
        source: "include/stddef.h"
      },
      {
        path: "/sysroot/include/stdint.h",
        source: "include/stdint.h"
      },
      {
        path: "/sysroot/include/stdnoreturn.h",
        source: "include/stdnoreturn.h"
      },
      {
        path: "/sysroot/include/tccdefs.h",
        source: "include/tccdefs.h"
      },
      {
        path: "/sysroot/include/tgmath.h",
        source: "include/tgmath.h"
      },
      {
        path: "/sysroot/include/varargs.h",
        source: "include/varargs.h"
      },
      {
        path: "/sysroot/include/tcclib.h",
        source: "tcclib.h"
      }
    ],
    libDir: "/sysroot/lib",
    libFiles: [
      {
        binary: true,
        path: "/sysroot/lib/libtcc1.a",
        source: "lib/x86_64-linux-gnu/libtcc1.a"
      },
      {
        binary: true,
        path: "/sysroot/lib/crt1.o",
        source: "lib/x86_64-linux-gnu/crt1.o"
      },
      {
        binary: true,
        path: "/sysroot/lib/crti.o",
        source: "lib/x86_64-linux-gnu/crti.o"
      },
      {
        binary: true,
        path: "/sysroot/lib/crtn.o",
        source: "lib/x86_64-linux-gnu/crtn.o"
      },
      {
        binary: true,
        path: "/sysroot/lib/libc.so",
        source: "lib/x86_64-linux-gnu/libc.so"
      },
      {
        binary: true,
        path: "/sysroot/lib/libm.so",
        source: "lib/x86_64-linux-gnu/libm.so"
      }
    ],
    outputs: {
      exe: {
        format: "ELF executable",
        ext: ""
      },
      obj: {
        format: "ELF relocatable",
        ext: ".o"
      },
      shared: {
        format: "ELF shared object",
        ext: ".so"
      },
      ar: {
        format: "ar archive",
        ext: ".a"
      }
    }
  };

  // sysroot-manifests/aarch64-linux-gnu.json
  var aarch64_linux_gnu_default = {
    name: "tinyc-sysroot",
    version: 1,
    target: "aarch64-linux-gnu",
    binPath: "/sysroot/bin/tinyc",
    includeDir: "/sysroot/include",
    includeFiles: [
      {
        path: "/sysroot/include/float.h",
        source: "include/float.h"
      },
      {
        path: "/sysroot/include/iso646.h",
        source: "include/iso646.h"
      },
      {
        path: "/sysroot/include/limits.h",
        source: "include/limits.h"
      },
      {
        path: "/sysroot/include/math.h",
        source: "include/math.h"
      },
      {
        path: "/sysroot/include/stdalign.h",
        source: "include/stdalign.h"
      },
      {
        path: "/sysroot/include/stdarg.h",
        source: "include/stdarg.h"
      },
      {
        path: "/sysroot/include/stdatomic.h",
        source: "include/stdatomic.h"
      },
      {
        path: "/sysroot/include/stdbool.h",
        source: "include/stdbool.h"
      },
      {
        path: "/sysroot/include/stddef.h",
        source: "include/stddef.h"
      },
      {
        path: "/sysroot/include/stdint.h",
        source: "include/stdint.h"
      },
      {
        path: "/sysroot/include/stdnoreturn.h",
        source: "include/stdnoreturn.h"
      },
      {
        path: "/sysroot/include/tccdefs.h",
        source: "include/tccdefs.h"
      },
      {
        path: "/sysroot/include/tgmath.h",
        source: "include/tgmath.h"
      },
      {
        path: "/sysroot/include/varargs.h",
        source: "include/varargs.h"
      },
      {
        path: "/sysroot/include/tcclib.h",
        source: "tcclib.h"
      }
    ],
    libDir: "/sysroot/lib",
    libFiles: [
      {
        binary: true,
        path: "/sysroot/lib/libtcc1.a",
        source: "lib/aarch64-linux-gnu/libtcc1.a"
      },
      {
        binary: true,
        path: "/sysroot/lib/crt1.o",
        source: "lib/aarch64-linux-gnu/crt1.o"
      },
      {
        binary: true,
        path: "/sysroot/lib/crti.o",
        source: "lib/aarch64-linux-gnu/crti.o"
      },
      {
        binary: true,
        path: "/sysroot/lib/crtn.o",
        source: "lib/aarch64-linux-gnu/crtn.o"
      },
      {
        binary: true,
        path: "/sysroot/lib/libc.so",
        source: "lib/aarch64-linux-gnu/libc.so"
      },
      {
        binary: true,
        path: "/sysroot/lib/libm.so",
        source: "lib/aarch64-linux-gnu/libm.so"
      }
    ],
    outputs: {
      exe: {
        format: "ELF executable",
        ext: ""
      },
      obj: {
        format: "ELF relocatable",
        ext: ".o"
      },
      shared: {
        format: "ELF shared object",
        ext: ".so"
      },
      ar: {
        format: "ar archive",
        ext: ".a"
      }
    }
  };

  // sysroot-manifests/riscv64-linux-gnu.json
  var riscv64_linux_gnu_default = {
    name: "tinyc-sysroot",
    version: 1,
    target: "riscv64-linux-gnu",
    binPath: "/sysroot/bin/tinyc",
    includeDir: "/sysroot/include",
    includeFiles: [
      {
        path: "/sysroot/include/float.h",
        source: "include/float.h"
      },
      {
        path: "/sysroot/include/iso646.h",
        source: "include/iso646.h"
      },
      {
        path: "/sysroot/include/limits.h",
        source: "include/limits.h"
      },
      {
        path: "/sysroot/include/math.h",
        source: "include/math.h"
      },
      {
        path: "/sysroot/include/stdalign.h",
        source: "include/stdalign.h"
      },
      {
        path: "/sysroot/include/stdarg.h",
        source: "include/stdarg.h"
      },
      {
        path: "/sysroot/include/stdatomic.h",
        source: "include/stdatomic.h"
      },
      {
        path: "/sysroot/include/stdbool.h",
        source: "include/stdbool.h"
      },
      {
        path: "/sysroot/include/stddef.h",
        source: "include/stddef.h"
      },
      {
        path: "/sysroot/include/stdint.h",
        source: "include/stdint.h"
      },
      {
        path: "/sysroot/include/stdnoreturn.h",
        source: "include/stdnoreturn.h"
      },
      {
        path: "/sysroot/include/tccdefs.h",
        source: "include/tccdefs.h"
      },
      {
        path: "/sysroot/include/tgmath.h",
        source: "include/tgmath.h"
      },
      {
        path: "/sysroot/include/varargs.h",
        source: "include/varargs.h"
      },
      {
        path: "/sysroot/include/tcclib.h",
        source: "tcclib.h"
      }
    ],
    libDir: "/sysroot/lib",
    libFiles: [
      {
        binary: true,
        path: "/sysroot/lib/libtcc1.a",
        source: "lib/riscv64-linux-gnu/libtcc1.a"
      },
      {
        binary: true,
        path: "/sysroot/lib/crt1.o",
        source: "lib/riscv64-linux-gnu/crt1.o"
      },
      {
        binary: true,
        path: "/sysroot/lib/crti.o",
        source: "lib/riscv64-linux-gnu/crti.o"
      },
      {
        binary: true,
        path: "/sysroot/lib/crtn.o",
        source: "lib/riscv64-linux-gnu/crtn.o"
      },
      {
        binary: true,
        path: "/sysroot/lib/libc.so",
        source: "lib/riscv64-linux-gnu/libc.so"
      },
      {
        binary: true,
        path: "/sysroot/lib/libm.so",
        source: "lib/riscv64-linux-gnu/libm.so"
      }
    ],
    outputs: {
      exe: {
        format: "ELF executable",
        ext: ""
      },
      obj: {
        format: "ELF relocatable",
        ext: ".o"
      },
      shared: {
        format: "ELF shared object",
        ext: ".so"
      },
      ar: {
        format: "ar archive",
        ext: ".a"
      }
    }
  };

  // sysroot-manifests/x86_64-windows.json
  var x86_64_windows_default = {
    name: "tinyc-sysroot",
    version: 1,
    target: "x86_64-windows",
    binPath: "/sysroot/bin/tinyc",
    includeDir: "/sysroot/include",
    includeFiles: [
      {
        path: "/sysroot/include/float.h",
        source: "include/float.h"
      },
      {
        path: "/sysroot/include/iso646.h",
        source: "include/iso646.h"
      },
      {
        path: "/sysroot/include/limits.h",
        source: "include/limits.h"
      },
      {
        path: "/sysroot/include/stdalign.h",
        source: "include/stdalign.h"
      },
      {
        path: "/sysroot/include/stdarg.h",
        source: "include/stdarg.h"
      },
      {
        path: "/sysroot/include/stdatomic.h",
        source: "include/stdatomic.h"
      },
      {
        path: "/sysroot/include/stdbool.h",
        source: "include/stdbool.h"
      },
      {
        path: "/sysroot/include/stddef.h",
        source: "include/stddef.h"
      },
      {
        path: "/sysroot/include/stdint.h",
        source: "include/stdint.h"
      },
      {
        path: "/sysroot/include/stdnoreturn.h",
        source: "include/stdnoreturn.h"
      },
      {
        path: "/sysroot/include/tccdefs.h",
        source: "include/tccdefs.h"
      },
      {
        path: "/sysroot/include/varargs.h",
        source: "include/varargs.h"
      }
    ],
    libDir: "/sysroot/lib",
    libFiles: [
      {
        binary: true,
        path: "/sysroot/lib/libtcc1.a",
        source: "lib/x86_64-windows/libtcc1.a"
      },
      {
        binary: true,
        path: "/sysroot/lib/msvcrt.def",
        source: "lib/x86_64-windows/msvcrt.def"
      },
      {
        binary: true,
        path: "/sysroot/lib/kernel32.def",
        source: "lib/x86_64-windows/kernel32.def"
      }
    ],
    outputs: {
      exe: {
        format: "PE executable",
        ext: ".exe"
      },
      obj: {
        format: "COFF object",
        ext: ".o"
      },
      shared: {
        format: "PE DLL",
        ext: ".dll"
      },
      ar: {
        format: "ar archive",
        ext: ".a"
      }
    }
  };

  // sysroot-manifests/aarch64-linux-android.json
  var aarch64_linux_android_default = {
    name: "tinyc-sysroot",
    version: 1,
    target: "aarch64-linux-android",
    binPath: "/sysroot/bin/tinyc",
    includeDir: "/sysroot/include",
    includeFiles: [
      {
        path: "/sysroot/include/float.h",
        source: "include/float.h"
      },
      {
        path: "/sysroot/include/iso646.h",
        source: "include/iso646.h"
      },
      {
        path: "/sysroot/include/limits.h",
        source: "include/limits.h"
      },
      {
        path: "/sysroot/include/math.h",
        source: "include/math.h"
      },
      {
        path: "/sysroot/include/stdalign.h",
        source: "include/stdalign.h"
      },
      {
        path: "/sysroot/include/stdarg.h",
        source: "include/stdarg.h"
      },
      {
        path: "/sysroot/include/stdatomic.h",
        source: "include/stdatomic.h"
      },
      {
        path: "/sysroot/include/stdbool.h",
        source: "include/stdbool.h"
      },
      {
        path: "/sysroot/include/stddef.h",
        source: "include/stddef.h"
      },
      {
        path: "/sysroot/include/stdint.h",
        source: "include/stdint.h"
      },
      {
        path: "/sysroot/include/stdnoreturn.h",
        source: "include/stdnoreturn.h"
      },
      {
        path: "/sysroot/include/tccdefs.h",
        source: "include/tccdefs.h"
      },
      {
        path: "/sysroot/include/tgmath.h",
        source: "include/tgmath.h"
      },
      {
        path: "/sysroot/include/varargs.h",
        source: "include/varargs.h"
      },
      {
        path: "/sysroot/include/tcclib.h",
        source: "tcclib.h"
      }
    ],
    libDir: "/sysroot/lib",
    libFiles: [
      {
        binary: true,
        path: "/sysroot/lib/libtcc1.a",
        source: "lib/aarch64-linux-android/libtcc1.a"
      },
      {
        binary: true,
        path: "/sysroot/lib/crt1.o",
        source: "lib/aarch64-linux-android/crt1.o"
      },
      {
        binary: true,
        path: "/sysroot/lib/crti.o",
        source: "lib/aarch64-linux-android/crti.o"
      },
      {
        binary: true,
        path: "/sysroot/lib/crtn.o",
        source: "lib/aarch64-linux-android/crtn.o"
      },
      {
        binary: true,
        path: "/sysroot/lib/libc.so",
        source: "lib/aarch64-linux-android/libc.so"
      },
      {
        binary: true,
        path: "/sysroot/lib/libm.so",
        source: "lib/aarch64-linux-android/libm.so"
      },
      {
        binary: true,
        path: "/sysroot/lib/libdl.so",
        source: "lib/aarch64-linux-android/libdl.so"
      }
    ],
    outputs: {
      exe: {
        format: "ELF executable",
        ext: ""
      },
      obj: {
        format: "ELF relocatable",
        ext: ".o"
      },
      shared: {
        format: "ELF shared object",
        ext: ".so"
      },
      ar: {
        format: "ar archive",
        ext: ".a"
      }
    },
    libc: "bionic",
    androidApi: 30,
    dynamicLinker: "/system/bin/linker64"
  };

  // sysroot-manifests/x86_64-linux-android.json
  var x86_64_linux_android_default = {
    name: "tinyc-sysroot",
    version: 1,
    target: "x86_64-linux-android",
    binPath: "/sysroot/bin/tinyc",
    includeDir: "/sysroot/include",
    includeFiles: [
      {
        path: "/sysroot/include/float.h",
        source: "include/float.h"
      },
      {
        path: "/sysroot/include/iso646.h",
        source: "include/iso646.h"
      },
      {
        path: "/sysroot/include/limits.h",
        source: "include/limits.h"
      },
      {
        path: "/sysroot/include/math.h",
        source: "include/math.h"
      },
      {
        path: "/sysroot/include/stdalign.h",
        source: "include/stdalign.h"
      },
      {
        path: "/sysroot/include/stdarg.h",
        source: "include/stdarg.h"
      },
      {
        path: "/sysroot/include/stdatomic.h",
        source: "include/stdatomic.h"
      },
      {
        path: "/sysroot/include/stdbool.h",
        source: "include/stdbool.h"
      },
      {
        path: "/sysroot/include/stddef.h",
        source: "include/stddef.h"
      },
      {
        path: "/sysroot/include/stdint.h",
        source: "include/stdint.h"
      },
      {
        path: "/sysroot/include/stdnoreturn.h",
        source: "include/stdnoreturn.h"
      },
      {
        path: "/sysroot/include/tccdefs.h",
        source: "include/tccdefs.h"
      },
      {
        path: "/sysroot/include/tgmath.h",
        source: "include/tgmath.h"
      },
      {
        path: "/sysroot/include/varargs.h",
        source: "include/varargs.h"
      },
      {
        path: "/sysroot/include/tcclib.h",
        source: "tcclib.h"
      }
    ],
    libDir: "/sysroot/lib",
    libFiles: [
      {
        binary: true,
        path: "/sysroot/lib/libtcc1.a",
        source: "lib/x86_64-linux-android/libtcc1.a"
      },
      {
        binary: true,
        path: "/sysroot/lib/crt1.o",
        source: "lib/x86_64-linux-android/crt1.o"
      },
      {
        binary: true,
        path: "/sysroot/lib/crti.o",
        source: "lib/x86_64-linux-android/crti.o"
      },
      {
        binary: true,
        path: "/sysroot/lib/crtn.o",
        source: "lib/x86_64-linux-android/crtn.o"
      },
      {
        binary: true,
        path: "/sysroot/lib/libc.so",
        source: "lib/x86_64-linux-android/libc.so"
      },
      {
        binary: true,
        path: "/sysroot/lib/libm.so",
        source: "lib/x86_64-linux-android/libm.so"
      },
      {
        binary: true,
        path: "/sysroot/lib/libdl.so",
        source: "lib/x86_64-linux-android/libdl.so"
      }
    ],
    outputs: {
      exe: {
        format: "ELF executable",
        ext: ""
      },
      obj: {
        format: "ELF relocatable",
        ext: ".o"
      },
      shared: {
        format: "ELF shared object",
        ext: ".so"
      },
      ar: {
        format: "ar archive",
        ext: ".a"
      }
    },
    libc: "bionic",
    androidApi: 30,
    dynamicLinker: "/system/bin/linker64"
  };

  // sysroot-manifests/arm64-apple-darwin.json
  var arm64_apple_darwin_default = {
    name: "tinyc-sysroot",
    version: 1,
    target: "arm64-apple-darwin",
    binPath: "/sysroot/bin/tinyc",
    includeDir: "/sysroot/include",
    includeFiles: [
      {
        path: "/sysroot/include/float.h",
        source: "include/float.h"
      },
      {
        path: "/sysroot/include/iso646.h",
        source: "include/iso646.h"
      },
      {
        path: "/sysroot/include/limits.h",
        source: "include/limits.h"
      },
      {
        path: "/sysroot/include/math.h",
        source: "include/math.h"
      },
      {
        path: "/sysroot/include/stdalign.h",
        source: "include/stdalign.h"
      },
      {
        path: "/sysroot/include/stdarg.h",
        source: "include/stdarg.h"
      },
      {
        path: "/sysroot/include/stdatomic.h",
        source: "include/stdatomic.h"
      },
      {
        path: "/sysroot/include/stdbool.h",
        source: "include/stdbool.h"
      },
      {
        path: "/sysroot/include/stddef.h",
        source: "include/stddef.h"
      },
      {
        path: "/sysroot/include/stdint.h",
        source: "include/stdint.h"
      },
      {
        path: "/sysroot/include/stdnoreturn.h",
        source: "include/stdnoreturn.h"
      },
      {
        path: "/sysroot/include/tccdefs.h",
        source: "include/tccdefs.h"
      },
      {
        path: "/sysroot/include/tgmath.h",
        source: "include/tgmath.h"
      },
      {
        path: "/sysroot/include/varargs.h",
        source: "include/varargs.h"
      },
      {
        path: "/sysroot/include/tcclib.h",
        source: "tcclib.h"
      }
    ],
    libDir: "/sysroot/lib",
    libFiles: [
      {
        binary: true,
        path: "/sysroot/lib/libtcc1.a",
        source: "lib/arm64-apple-darwin/libtcc1.a"
      },
      {
        binary: true,
        path: "/sysroot/lib/libc.tbd",
        source: "lib/arm64-apple-darwin/libc.tbd"
      }
    ],
    outputs: {
      exe: {
        format: "Mach-O executable",
        ext: ""
      },
      obj: {
        format: "Mach-O object",
        ext: ".o"
      },
      shared: {
        format: "Mach-O dylib",
        ext: ".dylib"
      },
      ar: {
        format: "ar archive",
        ext: ".a"
      }
    },
    systemHeaders: "user-sdk"
  };

  // sysroot-manifests/x86_64-apple-darwin.json
  var x86_64_apple_darwin_default = {
    name: "tinyc-sysroot",
    version: 1,
    target: "x86_64-apple-darwin",
    binPath: "/sysroot/bin/tinyc",
    includeDir: "/sysroot/include",
    includeFiles: [
      {
        path: "/sysroot/include/float.h",
        source: "include/float.h"
      },
      {
        path: "/sysroot/include/iso646.h",
        source: "include/iso646.h"
      },
      {
        path: "/sysroot/include/limits.h",
        source: "include/limits.h"
      },
      {
        path: "/sysroot/include/math.h",
        source: "include/math.h"
      },
      {
        path: "/sysroot/include/stdalign.h",
        source: "include/stdalign.h"
      },
      {
        path: "/sysroot/include/stdarg.h",
        source: "include/stdarg.h"
      },
      {
        path: "/sysroot/include/stdatomic.h",
        source: "include/stdatomic.h"
      },
      {
        path: "/sysroot/include/stdbool.h",
        source: "include/stdbool.h"
      },
      {
        path: "/sysroot/include/stddef.h",
        source: "include/stddef.h"
      },
      {
        path: "/sysroot/include/stdint.h",
        source: "include/stdint.h"
      },
      {
        path: "/sysroot/include/stdnoreturn.h",
        source: "include/stdnoreturn.h"
      },
      {
        path: "/sysroot/include/tccdefs.h",
        source: "include/tccdefs.h"
      },
      {
        path: "/sysroot/include/tgmath.h",
        source: "include/tgmath.h"
      },
      {
        path: "/sysroot/include/varargs.h",
        source: "include/varargs.h"
      },
      {
        path: "/sysroot/include/tcclib.h",
        source: "tcclib.h"
      }
    ],
    libDir: "/sysroot/lib",
    libFiles: [
      {
        binary: true,
        path: "/sysroot/lib/libtcc1.a",
        source: "lib/x86_64-apple-darwin/libtcc1.a"
      },
      {
        binary: true,
        path: "/sysroot/lib/libc.tbd",
        source: "lib/x86_64-apple-darwin/libc.tbd"
      }
    ],
    outputs: {
      exe: {
        format: "Mach-O executable",
        ext: ""
      },
      obj: {
        format: "Mach-O object",
        ext: ".o"
      },
      shared: {
        format: "Mach-O dylib",
        ext: ".dylib"
      },
      ar: {
        format: "ar archive",
        ext: ".a"
      }
    },
    systemHeaders: "user-sdk"
  };

  // runtime-shims.mjs
  var textEncoder = new TextEncoder();
  var textDecoder = new TextDecoder();
  var ShimExit = class extends Error {
    constructor(code) {
      super(`tinyc: libc exit(${code})`);
      this.name = "ShimExit";
      this.exitCode = code;
    }
  };
  var ALLOC_USED = 2702233105;
  function createDefaultShims(options = {}) {
    const { holder } = options;
    const io = options.io ?? {};
    const emitOut = typeof io.out === "function" ? io.out : () => {
    };
    const emitErr = typeof io.err === "function" ? io.err : () => {
    };
    const heapBase = options.heapBase ?? 65536;
    const memory = () => holder?.memory ?? null;
    const u8 = () => new Uint8Array(memory().buffer);
    const dv = () => new DataView(memory().buffer);
    const readU32 = (ptr) => dv().getUint32(ptr, true);
    const writeU32 = (ptr, v) => dv().setUint32(ptr, v >>> 0, true);
    const readBytes = (ptr, len) => u8().slice(ptr, ptr + len);
    const writeBytes = (ptr, data) => {
      u8().set(data, ptr);
    };
    const readCString = (ptr, max = 1 << 20) => {
      if (!ptr) return "";
      const m = u8();
      let end = ptr;
      while (end < m.length && end - ptr < max && m[end] !== 0) end++;
      return textDecoder.decode(m.slice(ptr, end));
    };
    const writeCStringBytes = (ptr, bytes) => {
      writeBytes(ptr, bytes);
      u8()[ptr + bytes.length] = 0;
      return bytes.length;
    };
    const ERRNO = {
      ENOENT: 2,
      EBADF: 9,
      ENOMEM: 12,
      EEXIST: 17,
      EINVAL: 22
    };
    let errnoValue = 0;
    const setErrno = (code) => {
      errnoValue = code;
    };
    const memoryAllocator = options.memoryAllocator === true;
    const HDR_HEAP_TOP = 512, HDR_FREE_HEAD = 516, HDR_ALLOC_LOCK = 520;
    const allocState = memoryAllocator ? {
      get heapTop() {
        return dv().getInt32(HDR_HEAP_TOP, true);
      },
      set heapTop(v) {
        dv().setInt32(HDR_HEAP_TOP, v | 0, true);
      },
      get freeHead() {
        return dv().getInt32(HDR_FREE_HEAD, true);
      },
      set freeHead(v) {
        dv().setInt32(HDR_FREE_HEAD, v | 0, true);
      }
    } : options.allocatorState ?? { heapTop: 0, freeHead: 0 };
    function allocLock() {
      if (!memoryAllocator) return;
      for (; ; ) {
        const words = new Int32Array(memory().buffer);
        if (Atomics.compareExchange(words, HDR_ALLOC_LOCK / 4, 0, 1) === 0)
          return;
      }
    }
    function allocUnlock() {
      if (!memoryAllocator) return;
      const words = new Int32Array(memory().buffer);
      Atomics.store(words, HDR_ALLOC_LOCK / 4, 0);
      Atomics.notify(words, HDR_ALLOC_LOCK / 4, 1);
    }
    function withAllocLock(fn) {
      if (!memoryAllocator) return fn();
      allocLock();
      try {
        return fn();
      } finally {
        allocUnlock();
      }
    }
    const moduleAllocator = () => {
      const ex = holder?.instance?.exports;
      return ex && typeof ex.malloc === "function" ? ex : null;
    };
    function ensureCapacity(end) {
      const mem = memory();
      if (!mem) return false;
      if (mem.buffer.byteLength >= end) return true;
      const pages = Math.ceil((end - mem.buffer.byteLength) / 65536);
      try {
        mem.grow(pages);
        return true;
      } catch {
        return false;
      }
    }
    function rawMallocImpl(size) {
      size = Math.max(8, size + 7 & ~7);
      let prev = 0;
      let cur = allocState.freeHead;
      while (cur) {
        const csize = readU32(cur);
        const cnext = readU32(cur + 4);
        if (csize >= size) {
          if (csize - size >= 16) {
            const rem = cur + 8 + size;
            writeU32(rem, csize - size - 8);
            writeU32(rem + 4, cnext);
            writeU32(cur, size);
            if (prev) writeU32(prev + 4, rem);
            else allocState.freeHead = rem;
          } else {
            if (prev) writeU32(prev + 4, cnext);
            else allocState.freeHead = cnext;
          }
          writeU32(cur + 4, ALLOC_USED);
          return cur + 8;
        }
        prev = cur;
        cur = cnext;
      }
      if (!allocState.heapTop) allocState.heapTop = heapBase;
      const header = allocState.heapTop;
      const end = header + 8 + size;
      if (!ensureCapacity(end)) return 0;
      writeU32(header, size);
      writeU32(header + 4, ALLOC_USED);
      allocState.heapTop = end;
      return header + 8;
    }
    function rawFreeImpl(ptr) {
      if (!ptr) return;
      const header = ptr - 8;
      if (readU32(header + 4) !== ALLOC_USED) return;
      writeU32(header + 4, allocState.freeHead);
      allocState.freeHead = header;
    }
    function reserveStaticImpl(size, align = 16) {
      if (!allocState.heapTop) allocState.heapTop = heapBase;
      const base = allocState.heapTop + (align - 1) & ~(align - 1);
      const end = base + size;
      if (!ensureCapacity(end))
        throw new Error(`tinyc: cannot grow linear memory to reserve ${size} bytes at ${base}`);
      allocState.heapTop = end;
      return base;
    }
    function rawMalloc(size) {
      return withAllocLock(() => rawMallocImpl(size));
    }
    function rawFree(ptr) {
      return withAllocLock(() => rawFreeImpl(ptr));
    }
    function reserveStatic(size, align) {
      return withAllocLock(() => reserveStaticImpl(size, align));
    }
    function malloc(size) {
      const own = moduleAllocator();
      const ptr = own ? own.malloc(size) : rawMalloc(size);
      if (!ptr && size) setErrno(ERRNO.ENOMEM);
      return ptr;
    }
    function free(ptr) {
      const own = moduleAllocator();
      if (own && typeof own.free === "function") {
        own.free(ptr);
        return;
      }
      rawFree(ptr);
    }
    function calloc(n, size) {
      const own = moduleAllocator();
      if (own && typeof own.calloc === "function") return own.calloc(n, size);
      const total = n * size;
      const ptr = malloc(total);
      if (ptr) writeBytes(ptr, new Uint8Array(total));
      return ptr;
    }
    function realloc(ptr, size) {
      if (!ptr) return malloc(size);
      if (size === 0) {
        free(ptr);
        return 0;
      }
      const own = moduleAllocator();
      if (own) {
        if (typeof own.realloc === "function") return own.realloc(ptr, size);
        const np2 = own.malloc(size);
        if (!np2) {
          setErrno(ERRNO.ENOMEM);
          return 0;
        }
        writeBytes(np2, readBytes(ptr, size));
        if (typeof own.free === "function") own.free(ptr);
        return np2;
      }
      const oldSize = readU32(ptr - 8);
      if (size <= oldSize) return ptr;
      const np = rawMalloc(size);
      if (!np) {
        setErrno(ERRNO.ENOMEM);
        return 0;
      }
      writeBytes(np, readBytes(ptr, oldSize));
      rawFree(ptr);
      return np;
    }
    function memcpy(dest, src, n) {
      writeBytes(dest, readBytes(src, n));
      return dest;
    }
    function memmove(dest, src, n) {
      writeBytes(dest, readBytes(src, n));
      return dest;
    }
    function memset(ptr, value, n) {
      u8().fill(value & 255, ptr, ptr + n);
      return ptr;
    }
    function memcmp(a, b, n) {
      const ma = u8();
      for (let i = 0; i < n; i++) {
        const d = ma[a + i] - ma[b + i];
        if (d !== 0) return d;
      }
      return 0;
    }
    function strlen(ptr) {
      const m = u8();
      let n = 0;
      while (m[ptr + n] !== 0) n++;
      return n;
    }
    function strcpy(dest, src) {
      writeBytes(dest, readBytes(src, strlen(src) + 1));
      return dest;
    }
    function strncpy(dest, src, n) {
      const m = u8();
      let i = 0;
      while (i < n && m[src + i] !== 0) {
        m[dest + i] = m[src + i];
        i++;
      }
      while (i < n) m[dest + i++] = 0;
      return dest;
    }
    function strcat(dest, src) {
      writeBytes(dest + strlen(dest), readBytes(src, strlen(src) + 1));
      return dest;
    }
    function strncat(dest, src, n) {
      const m = u8();
      const at = dest + strlen(dest);
      let i = 0;
      while (i < n && m[src + i] !== 0) {
        m[at + i] = m[src + i];
        i++;
      }
      m[at + i] = 0;
      return dest;
    }
    function strcmp(a, b) {
      const m = u8();
      let i = 0;
      for (; ; ) {
        const d = m[a + i] - m[b + i];
        if (d !== 0 || m[a + i] === 0) return d;
        i++;
      }
    }
    function strncmp(a, b, n) {
      const m = u8();
      for (let i = 0; i < n; i++) {
        const d = m[a + i] - m[b + i];
        if (d !== 0 || m[a + i] === 0) return d;
      }
      return 0;
    }
    function strchr(ptr, c) {
      const m = u8();
      const want = c & 255;
      let i = 0;
      for (; ; ) {
        if (m[ptr + i] === want) return ptr + i;
        if (m[ptr + i] === 0) return 0;
        i++;
      }
    }
    function strrchr(ptr, c) {
      const m = u8();
      const want = c & 255;
      let found = 0;
      let i = 0;
      for (; ; ) {
        if (m[ptr + i] === want) found = ptr + i;
        if (m[ptr + i] === 0) return found;
        i++;
      }
    }
    function strstr(haystack, needle) {
      const h = readCString(haystack);
      const ndl = readCString(needle);
      if (ndl === "") return haystack;
      const idx = h.indexOf(ndl);
      return idx < 0 ? 0 : haystack + textEncoder.encode(h.slice(0, idx)).length;
    }
    function strdup(ptr) {
      const copy = readBytes(ptr, strlen(ptr) + 1);
      const dest = malloc(copy.length);
      if (dest) writeBytes(dest, copy);
      return dest;
    }
    function memchr(ptr, c, n) {
      const m = u8();
      const want = c & 255;
      for (let i = 0; i < n; i++) {
        if (m[ptr + i] === want) return ptr + i;
      }
      return 0;
    }
    function strspn(ptr, acceptPtr) {
      const s = readCString(ptr);
      const accept = readCString(acceptPtr);
      let n = 0;
      while (n < s.length && accept.includes(s[n])) n++;
      return n;
    }
    function strcspn(ptr, rejectPtr) {
      const s = readCString(ptr);
      const reject = readCString(rejectPtr);
      let n = 0;
      while (n < s.length && !reject.includes(s[n])) n++;
      return n;
    }
    function strpbrk(ptr, acceptPtr) {
      const n = strcspn(ptr, acceptPtr);
      return n < strlen(ptr) ? ptr + n : 0;
    }
    let strtokSave = 0;
    function strtok(ptr, delimPtr) {
      const delim = readCString(delimPtr);
      let start;
      if (ptr) {
        start = ptr;
      } else {
        if (!strtokSave) return 0;
        start = strtokSave;
      }
      const m = u8();
      let i = start;
      while (m[i] !== 0 && delim.includes(String.fromCharCode(m[i]))) i++;
      if (m[i] === 0) {
        strtokSave = 0;
        return 0;
      }
      const token = i;
      while (m[i] !== 0 && !delim.includes(String.fromCharCode(m[i]))) i++;
      if (m[i] !== 0) {
        m[i] = 0;
        strtokSave = i + 1;
      } else {
        strtokSave = 0;
      }
      return token;
    }
    function strerrorText(errnum) {
      const table = {
        0: "Success",
        1: "Operation not permitted",
        2: "No such file or directory",
        9: "Bad file descriptor",
        12: "Cannot allocate memory",
        17: "File exists",
        22: "Invalid argument"
      };
      return table[errnum] ?? `Unknown error ${errnum | 0}`;
    }
    function strerror(errnum) {
      const bytes = textEncoder.encode(strerrorText(errnum));
      const buf = malloc(bytes.length + 1);
      if (buf) writeCStringBytes(buf, bytes);
      return buf;
    }
    function strcoll(a, b) {
      return strcmp(a, b);
    }
    function strxfrm(dest, src, n) {
      const len = strlen(src);
      if (dest && n > 0) {
        const take = Math.min(len, n - 1);
        writeBytes(dest, readBytes(src, take));
        u8()[dest + take] = 0;
      }
      return len;
    }
    const CT = (() => {
      const t = new Uint8Array(256);
      const U = 1, L = 2, D = 4, S = 8, P = 16, C = 32, X = 64, B = 128;
      for (let c = 0; c < 32; c++) t[c] |= C;
      t[127] |= C;
      for (let c = 48; c <= 57; c++) t[c] |= D | X;
      for (let c = 65; c <= 70; c++) t[c] |= X;
      for (let c = 97; c <= 102; c++) t[c] |= X;
      for (let c = 65; c <= 90; c++) t[c] |= U;
      for (let c = 97; c <= 122; c++) t[c] |= L;
      for (const c of [32, 9, 10, 11, 12, 13]) t[c] |= S;
      t[32] |= B;
      t[9] |= B;
      for (let c = 33; c <= 126; c++) {
        if (!(t[c] & (U | L | D))) t[c] |= P;
      }
      return { t, U, L, D, S, P, C, X, B };
    })();
    const ctFlag = (bit) => (c) => c >= 0 && c <= 255 ? CT.t[c] & bit ? 1 : 0 : 0;
    const isupper = ctFlag(CT.U);
    const islower = ctFlag(CT.L);
    const isdigit = ctFlag(CT.D);
    const isspace = ctFlag(CT.S);
    const ispunct = ctFlag(CT.P);
    const iscntrl = ctFlag(CT.C);
    const isxdigit = ctFlag(CT.X);
    const isblank = ctFlag(CT.B);
    const isalpha = (c) => isupper(c) || islower(c) ? 1 : 0;
    const isalnum = (c) => isalpha(c) || isdigit(c) ? 1 : 0;
    const isprint = (c) => c >= 32 && c <= 126 ? 1 : 0;
    const isgraph = (c) => c >= 33 && c <= 126 ? 1 : 0;
    const tolower = (c) => isupper(c) ? c + 32 : c;
    const toupper = (c) => islower(c) ? c - 32 : c;
    function scanInt(ptr, base) {
      const s = readCString(ptr);
      let i = 0;
      while (i < s.length && " 	\n\v\f\r".includes(s[i])) i++;
      let sign = 1n;
      if (s[i] === "+" || s[i] === "-") {
        if (s[i] === "-") sign = -1n;
        i++;
      }
      let b = base;
      if (b === 0) {
        if (s[i] === "0" && (s[i + 1] === "x" || s[i + 1] === "X")) b = 16;
        else if (s[i] === "0") b = 8;
        else b = 10;
      }
      if (b === 16 && s[i] === "0" && (s[i + 1] === "x" || s[i + 1] === "X")) i += 2;
      const digits = "0123456789abcdefghijklmnopqrstuvwxyz";
      let value = 0n;
      let any = false;
      while (i < s.length) {
        const d = digits.indexOf(s[i].toLowerCase());
        if (d < 0 || d >= b) break;
        value = value * BigInt(b) + BigInt(d);
        i++;
        any = true;
      }
      return { value: sign * value, consumed: any ? i : 0, any };
    }
    function finishEndptr(endptr, ptr, consumed) {
      if (endptr) writeU32(endptr, ptr + consumed);
    }
    const I32_MIN = -2147483648n;
    const I32_MAX = 2147483647n;
    const U32_MAX = 4294967295n;
    function clampBig(v, lo, hi) {
      return v < lo ? lo : v > hi ? hi : v;
    }
    function strtol(ptr, endptr, base) {
      const r = scanInt(ptr, base | 0);
      finishEndptr(endptr, ptr, r.consumed);
      return Number(clampBig(r.value, I32_MIN, I32_MAX));
    }
    function strtoul(ptr, endptr, base) {
      const r = scanInt(ptr, base | 0);
      finishEndptr(endptr, ptr, r.consumed);
      return Number(clampBig(r.value, 0n, U32_MAX)) >>> 0;
    }
    function strtoll(ptr, endptr, base) {
      const r = scanInt(ptr, base | 0);
      finishEndptr(endptr, ptr, r.consumed);
      return r.value;
    }
    function strtoull(ptr, endptr, base) {
      const r = scanInt(ptr, base | 0);
      finishEndptr(endptr, ptr, r.consumed);
      return r.value < 0n ? 0n : r.value;
    }
    function scanFloat(ptr) {
      const s = readCString(ptr);
      const m = /^[\s]*[+-]?(\d+\.?\d*|\.\d+)([eE][+-]?\d+)?/.exec(s) ?? /^[\s]*[+-]?(inf(inity)?|nan)/i.exec(s);
      if (!m) return { value: 0, consumed: 0 };
      return { value: parseFloat(m[0]), consumed: m[0].length };
    }
    function strtod(ptr, endptr) {
      const r = scanFloat(ptr);
      finishEndptr(endptr, ptr, r.consumed);
      return r.value;
    }
    function strtof(ptr, endptr) {
      return Math.fround(strtod(ptr, endptr));
    }
    function atoi(ptr) {
      return Number(clampBig(scanInt(ptr, 10).value, I32_MIN, I32_MAX));
    }
    function atol(ptr) {
      return atoi(ptr);
    }
    function atoll(ptr) {
      return scanInt(ptr, 10).value;
    }
    function atof(ptr) {
      return scanFloat(ptr).value;
    }
    const abs = (v) => Math.abs(v | 0);
    const labs = (v) => Math.abs(v | 0);
    const llabs = (v) => v < 0n ? -v : v;
    function comparatorFn(cmpPtr) {
      const table = options.processTable ?? holder?.instance?.exports?.__indirect_function_table;
      if (!table || typeof table.get !== "function") return null;
      const fn = table.get(cmpPtr);
      return typeof fn === "function" ? fn : null;
    }
    function qsort(base, nmemb, size, cmpPtr) {
      const cmp = comparatorFn(cmpPtr);
      if (!cmp || nmemb < 2 || size <= 0) return;
      const order = Array.from({ length: nmemb }, (_, i) => i);
      order.sort((ia, ib) => cmp(base + ia * size, base + ib * size) | 0);
      const items = order.map((i) => readBytes(base + i * size, size));
      for (let i = 0; i < nmemb; i++) writeBytes(base + i * size, items[i]);
    }
    function bsearch(key, base, nmemb, size, cmpPtr) {
      const cmp = comparatorFn(cmpPtr);
      if (!cmp) return 0;
      let lo = 0;
      let hi = nmemb - 1;
      while (lo <= hi) {
        const mid = lo + hi >> 1;
        const r = cmp(key, base + mid * size) | 0;
        if (r === 0) return base + mid * size;
        if (r < 0) hi = mid - 1;
        else lo = mid + 1;
      }
      return 0;
    }
    let randState = 1;
    function srand(seed) {
      randState = seed >>> 0 || 1;
    }
    function rand() {
      randState = Math.imul(randState, 1103515245) + 12345 & 2147483647;
      return randState >> 16 & 32767;
    }
    const atexitHandlers = [];
    function atexit(fnPtr) {
      atexitHandlers.push(fnPtr);
      return 0;
    }
    function exit(code) {
      const table = options.processTable ?? holder?.instance?.exports?.__indirect_function_table;
      if (table && typeof table.get === "function") {
        for (let i = atexitHandlers.length - 1; i >= 0; i--) {
          try {
            const fn = table.get(atexitHandlers[i]);
            if (typeof fn === "function") fn();
          } catch {
          }
        }
      }
      throw new ShimExit(code | 0);
    }
    function abort() {
      throw new ShimExit(134);
    }
    let stdinBuf = null;
    let stdinPos = 0;
    const pushback = [];
    if (options.stdin != null) {
      stdinBuf = typeof options.stdin === "string" ? textEncoder.encode(options.stdin) : options.stdin;
    }
    function stdinPull() {
      if (stdinBuf && stdinPos < stdinBuf.length) return true;
      if (typeof io.in === "function") {
        const chunk = io.in();
        if (chunk == null) return false;
        const b = typeof chunk === "string" ? textEncoder.encode(chunk) : chunk;
        if (!b || b.length === 0) return false;
        stdinBuf = b;
        stdinPos = 0;
        return true;
      }
      return false;
    }
    function stdinRead(n) {
      const parts = [];
      let got = 0;
      while (got < n) {
        if (pushback.length) {
          parts.push(Uint8Array.of(pushback.pop()));
          got++;
          continue;
        }
        if (!stdinPull()) break;
        const take = Math.min(n - got, stdinBuf.length - stdinPos);
        parts.push(stdinBuf.slice(stdinPos, stdinPos + take));
        stdinPos += take;
        got += take;
      }
      const result2 = new Uint8Array(got);
      let at = 0;
      for (const p of parts) {
        result2.set(p, at);
        at += p.length;
      }
      return result2;
    }
    function makeVa(vaPtr) {
      let cur = vaPtr;
      const alignTo = (n) => {
        cur = cur + n - 1 & ~(n - 1);
      };
      return {
        i32() {
          alignTo(4);
          const v = dv().getInt32(cur, true);
          cur += 4;
          return v;
        },
        u32() {
          alignTo(4);
          const v = dv().getUint32(cur, true);
          cur += 4;
          return v;
        },
        i64() {
          alignTo(8);
          const v = dv().getBigInt64(cur, true);
          cur += 8;
          return v;
        },
        u64() {
          alignTo(8);
          const v = dv().getBigUint64(cur, true);
          cur += 8;
          return v;
        },
        f64() {
          alignTo(8);
          const v = dv().getFloat64(cur, true);
          cur += 8;
          return v;
        }
      };
    }
    function padSpaces(s, width, left) {
      if (s.length >= width) return s;
      const p = " ".repeat(width - s.length);
      return left ? s + p : p + s;
    }
    function fmtInteger(digits, sign, prefix, flags, width, precision) {
      let body = digits;
      if (precision >= 0) body = body.padStart(precision, "0");
      const head = sign + prefix;
      if (flags.includes("0") && !flags.includes("-") && precision < 0) {
        const total = head.length + body.length;
        if (total < width) body = "0".repeat(width - total) + body;
        return head + body;
      }
      return padSpaces(head + body, width, flags.includes("-"));
    }
    function fmtFloatNumber(body, flags, width) {
      let sign = "";
      let rest = body;
      if (rest.startsWith("-")) {
        sign = "-";
        rest = rest.slice(1);
      } else if (flags.includes("+")) {
        sign = "+";
      } else if (flags.includes(" ")) {
        sign = " ";
      }
      const special = /^(inf|nan)/i.test(rest);
      if (flags.includes("0") && !flags.includes("-") && !special) {
        const total = sign.length + rest.length;
        if (total < width) rest = "0".repeat(width - total) + rest;
        return sign + rest;
      }
      return padSpaces(sign + rest, width, flags.includes("-"));
    }
    function expParts(s) {
      const [mant, exp] = s.split("e");
      const e = parseInt(exp, 10);
      return [mant, (e < 0 ? "-" : "+") + String(Math.abs(e)).padStart(2, "0")];
    }
    function fmtE(v, precision, upper) {
      if (Number.isNaN(v)) return upper ? "NAN" : "nan";
      if (!Number.isFinite(v)) return (v > 0 ? "" : "-") + (upper ? "INF" : "inf");
      const [mant, e] = expParts(v.toExponential(precision));
      return mant + (upper ? "E" : "e") + e;
    }
    function fmtF(v, precision, upper) {
      if (Number.isNaN(v)) return upper ? "NAN" : "nan";
      if (!Number.isFinite(v)) return (v > 0 ? "" : "-") + (upper ? "INF" : "inf");
      return v.toFixed(precision);
    }
    function fmtG(v, precision, upper, alt) {
      if (Number.isNaN(v) || !Number.isFinite(v)) return fmtF(v, 0, upper);
      const p = precision <= 0 ? 1 : precision;
      let s = v.toPrecision(p);
      if (!alt) {
        const strip = (m) => m.includes(".") ? m.replace(/0+$/, "").replace(/\.$/, "") : m;
        if (s.includes("e")) {
          const [m, e] = s.split("e");
          s = strip(m) + "e" + e;
        } else {
          s = strip(s);
        }
        s = s.replace(/e([+-])(\d)$/, "e$10$2");
      }
      return upper ? s.toUpperCase() : s;
    }
    function convert(conv, { flags, width, precision, length, va }, outLen) {
      switch (conv) {
        case "d":
        case "i": {
          let sign = "";
          let digits;
          if (length === "ll") {
            const v = va.i64();
            if (v < 0n) {
              sign = "-";
              digits = (-v).toString(10);
            } else digits = v.toString(10);
            if (v === 0n && precision === 0) digits = "";
          } else {
            let v = va.i32();
            if (length === "hh") v = v << 24 >> 24;
            else if (length === "h") v = v << 16 >> 16;
            if (v < 0) {
              sign = "-";
              digits = String(-v);
            } else digits = String(v);
            if (v === 0 && precision === 0) digits = "";
          }
          if (!sign && flags.includes("+")) sign = "+";
          else if (!sign && flags.includes(" ")) sign = " ";
          return fmtInteger(digits, sign, "", flags, width, precision);
        }
        case "u":
        case "x":
        case "X":
        case "o": {
          const radix = conv === "u" ? 10 : conv === "o" ? 8 : 16;
          let digits;
          let isZero;
          if (length === "ll") {
            const v = va.u64();
            isZero = v === 0n;
            digits = v.toString(radix);
            if (isZero && precision === 0) digits = "";
          } else {
            let v = va.u32();
            if (length === "hh") v &= 255;
            else if (length === "h") v &= 65535;
            isZero = v === 0;
            digits = v.toString(radix);
            if (isZero && precision === 0) digits = "";
          }
          if (conv === "X") digits = digits.toUpperCase();
          let prefix = "";
          if (flags.includes("#") && conv !== "u") {
            if (conv === "o") {
              if (!digits.startsWith("0")) prefix = "0";
            } else if (!isZero) {
              prefix = conv === "X" ? "0X" : "0x";
            }
          }
          return fmtInteger(digits, "", prefix, flags, width, precision);
        }
        case "c": {
          const ch = String.fromCharCode(va.i32() & 255);
          return padSpaces(ch, width, flags.includes("-"));
        }
        case "s": {
          const ptr = va.u32();
          let s = ptr ? readCString(ptr) : "(null)";
          if (precision >= 0) s = s.slice(0, precision);
          return padSpaces(s, width, flags.includes("-"));
        }
        case "p": {
          const ptr = va.u32();
          return padSpaces("0x" + ptr.toString(16), width, flags.includes("-"));
        }
        case "f":
        case "F":
          return fmtFloatNumber(
            fmtF(va.f64(), precision < 0 ? 6 : precision, conv === "F"),
            flags,
            width
          );
        case "e":
        case "E":
          return fmtFloatNumber(
            fmtE(va.f64(), precision < 0 ? 6 : precision, conv === "E"),
            flags,
            width
          );
        case "g":
        case "G":
          return fmtFloatNumber(
            fmtG(va.f64(), precision < 0 ? 6 : precision, conv === "G", flags.includes("#")),
            flags,
            width
          );
        case "a":
        case "A": {
          const v = va.f64();
          return fmtFloatNumber(String(v), flags, width);
        }
        case "n": {
          const ptr = va.u32();
          if (ptr) {
            if (length === "ll") dv().setBigInt64(ptr, BigInt(outLen), true);
            else dv().setInt32(ptr, outLen, true);
          }
          return "";
        }
        default:
          return "%" + conv;
      }
    }
    function formatString(fmtPtr, vaPtr) {
      const fmt = readCString(fmtPtr);
      const va = makeVa(vaPtr);
      let out = "";
      let i = 0;
      while (i < fmt.length) {
        if (fmt[i] !== "%") {
          out += fmt[i++];
          continue;
        }
        i++;
        if (fmt[i] === "%") {
          out += "%";
          i++;
          continue;
        }
        let flags = "";
        while (i < fmt.length && "-+ #0".includes(fmt[i])) flags += fmt[i++];
        let width = 0;
        if (fmt[i] === "*") {
          width = va.i32();
          i++;
          if (width < 0) {
            flags += "-";
            width = -width;
          }
        } else {
          while (i < fmt.length && fmt[i] >= "0" && fmt[i] <= "9") {
            width = width * 10 + (fmt.charCodeAt(i++) - 48);
          }
        }
        let precision = -1;
        if (fmt[i] === ".") {
          i++;
          if (fmt[i] === "*") {
            precision = va.i32();
            i++;
          } else {
            precision = 0;
            while (i < fmt.length && fmt[i] >= "0" && fmt[i] <= "9") {
              precision = precision * 10 + (fmt.charCodeAt(i++) - 48);
            }
          }
          if (precision < 0) precision = -1;
        }
        let length = "";
        if (fmt.startsWith("hh", i)) {
          length = "hh";
          i += 2;
        } else if (fmt.startsWith("ll", i)) {
          length = "ll";
          i += 2;
        } else if (i < fmt.length && "hlzjtL".includes(fmt[i])) {
          length = fmt[i++];
        }
        const conv = fmt[i++] ?? "";
        out += convert(conv, { flags, width, precision, length, va }, out.length);
      }
      return out;
    }
    function emitText(text, toErr) {
      const bytes = textEncoder.encode(text);
      (toErr ? emitErr : emitOut)(bytes);
      return bytes.length;
    }
    function printf(fmt, va) {
      return emitText(formatString(fmt, va), false);
    }
    function fprintf(stream, fmt, va) {
      return streamWriteText(stream, formatString(fmt, va));
    }
    function sprintf(dest, fmt, va) {
      const bytes = textEncoder.encode(formatString(fmt, va));
      return writeCStringBytes(dest, bytes);
    }
    function snprintf(dest, size, fmt, va) {
      const bytes = textEncoder.encode(formatString(fmt, va));
      if (size > 0) {
        const n = Math.min(bytes.length, size - 1);
        writeBytes(dest, bytes.subarray(0, n));
        u8()[dest + n] = 0;
      }
      return bytes.length;
    }
    const backing = options.fileSystem ?? null;
    const memFiles = /* @__PURE__ */ new Map();
    const backingRead = backing ? (p) => backing.read(p) : (p) => memFiles.has(p) ? memFiles.get(p) : null;
    const backingWrite = backing ? (p, bytes) => backing.write(p, bytes) : (p, bytes) => {
      memFiles.set(p, bytes.slice());
    };
    const backingRemove = backing ? (p) => typeof backing.remove === "function" ? backing.remove(p) : false : (p) => memFiles.delete(p);
    function guestPath(ptr) {
      const raw = readCString(ptr);
      const parts = [];
      for (const seg of raw.split("/")) {
        if (seg === "" || seg === ".") continue;
        if (seg === "..") parts.pop();
        else parts.push(seg);
      }
      return "/" + parts.join("/");
    }
    const openFiles = /* @__PURE__ */ new Map();
    let nextHandle = 3;
    let stdinEof = false;
    function flushStream(st) {
      backingWrite(st.path, st.data);
    }
    function streamGetByte(handle) {
      if (handle === 0) {
        const data = stdinRead(1);
        if (!data.length) stdinEof = true;
        return data.length ? data[0] : -1;
      }
      const st = openFiles.get(handle);
      if (!st || !st.readable) return -1;
      if (st.unget.length) return st.unget.pop();
      if (st.pos >= st.data.length) {
        st.eof = true;
        return -1;
      }
      return st.data[st.pos++];
    }
    function streamUnget(handle, c) {
      if (handle === 0) {
        pushback.push(c & 255);
        stdinEof = false;
        return;
      }
      const st = openFiles.get(handle);
      if (st) {
        st.unget.push(c & 255);
        st.eof = false;
      }
    }
    function streamRead(handle, n) {
      if (handle === 0) {
        const data = stdinRead(n);
        if (data.length < n) stdinEof = true;
        return data;
      }
      const st = openFiles.get(handle);
      if (!st || !st.readable) return new Uint8Array(0);
      const parts = [];
      let got = 0;
      while (got < n && st.unget.length) {
        parts.push(Uint8Array.of(st.unget.pop()));
        got++;
      }
      if (got < n) {
        const take = Math.min(n - got, st.data.length - st.pos);
        if (take > 0) {
          parts.push(st.data.slice(st.pos, st.pos + take));
          st.pos += take;
          got += take;
        }
      }
      if (got < n) st.eof = true;
      const out = new Uint8Array(got);
      let at = 0;
      for (const p of parts) {
        out.set(p, at);
        at += p.length;
      }
      return out;
    }
    function streamWrite(handle, bytes) {
      if (handle === 1) {
        emitOut(bytes);
        return bytes.length;
      }
      if (handle === 2) {
        emitErr(bytes);
        return bytes.length;
      }
      const st = openFiles.get(handle);
      if (!st || !st.writable) return 0;
      if (st.append) st.pos = st.data.length;
      const end = st.pos + bytes.length;
      if (end > st.data.length) {
        const grown = new Uint8Array(end);
        grown.set(st.data);
        st.data = grown;
      }
      st.data.set(bytes, st.pos);
      st.pos = end;
      flushStream(st);
      return bytes.length;
    }
    function streamWriteText(handle, text) {
      return streamWrite(handle, textEncoder.encode(text));
    }
    function fopen(pathPtr, modePtr) {
      const path = guestPath(pathPtr);
      const mode = readCString(modePtr);
      const base = mode[0];
      if (base !== "r" && base !== "w" && base !== "a") {
        setErrno(ERRNO.EINVAL);
        return 0;
      }
      const plus = mode.includes("+");
      if (base === "r") {
        const data = backingRead(path);
        if (data == null) {
          setErrno(ERRNO.ENOENT);
          return 0;
        }
        const handle2 = nextHandle++;
        openFiles.set(handle2, {
          path,
          data: data.slice(),
          pos: 0,
          readable: true,
          writable: plus,
          append: false,
          eof: false,
          err: false,
          unget: []
        });
        return handle2;
      }
      if (base === "w") {
        if (mode.includes("x") && backingRead(path) != null) {
          setErrno(ERRNO.EEXIST);
          return 0;
        }
        const handle2 = nextHandle++;
        const st = {
          path,
          data: new Uint8Array(0),
          pos: 0,
          readable: plus,
          writable: true,
          append: false,
          eof: false,
          err: false,
          unget: []
        };
        openFiles.set(handle2, st);
        flushStream(st);
        return handle2;
      }
      const existing = backingRead(path);
      const handle = nextHandle++;
      openFiles.set(handle, {
        path,
        data: existing ? existing.slice() : new Uint8Array(0),
        pos: existing ? existing.length : 0,
        readable: plus,
        writable: true,
        append: true,
        eof: false,
        err: false,
        unget: []
      });
      return handle;
    }
    function fclose(handle) {
      const st = openFiles.get(handle);
      if (!st) return handle >= 0 && handle <= 2 ? 0 : -1;
      if (st.temp) backingRemove(st.path);
      else flushStream(st);
      openFiles.delete(handle);
      return 0;
    }
    function fflush(handle) {
      if (handle === 0) {
        for (const st2 of openFiles.values()) flushStream(st2);
        return 0;
      }
      const st = openFiles.get(handle);
      if (st) flushStream(st);
      return 0;
    }
    function fseek(handle, offset, whence) {
      const st = openFiles.get(handle);
      if (!st) return -1;
      const base = whence === 0 ? 0 : whence === 1 ? st.pos : st.data.length;
      const np = base + offset;
      if (np < 0) {
        st.err = true;
        setErrno(ERRNO.EINVAL);
        return -1;
      }
      st.pos = np;
      st.eof = false;
      st.unget.length = 0;
      return 0;
    }
    function ftell(handle) {
      const st = openFiles.get(handle);
      return st ? st.pos : -1;
    }
    function rewind(handle) {
      const st = openFiles.get(handle);
      if (!st) return;
      st.pos = 0;
      st.eof = false;
      st.err = false;
      st.unget.length = 0;
    }
    function fgetpos(handle, posPtr) {
      const st = openFiles.get(handle);
      if (!st) return -1;
      dv().setBigInt64(posPtr, BigInt(st.pos), true);
      return 0;
    }
    function fsetpos(handle, posPtr) {
      return fseek(handle, Number(dv().getBigInt64(posPtr, true)), 0);
    }
    function feof(handle) {
      if (handle === 0) return stdinEof ? 1 : 0;
      const st = openFiles.get(handle);
      return st && st.eof ? 1 : 0;
    }
    function ferror(handle) {
      const st = openFiles.get(handle);
      return st && st.err ? 1 : 0;
    }
    function clearerr(handle) {
      if (handle === 0) {
        stdinEof = false;
        return;
      }
      const st = openFiles.get(handle);
      if (st) {
        st.eof = false;
        st.err = false;
      }
    }
    function freopen(pathPtr, modePtr, handle) {
      if (!pathPtr) return handle;
      const st = openFiles.get(handle);
      if (st) {
        flushStream(st);
        openFiles.delete(handle);
      }
      const fresh = fopen(pathPtr, modePtr);
      if (!fresh) return 0;
      const nst = openFiles.get(fresh);
      openFiles.delete(fresh);
      openFiles.set(handle, nst);
      return handle;
    }
    function removeFile(pathPtr) {
      if (backingRemove(guestPath(pathPtr))) return 0;
      setErrno(ERRNO.ENOENT);
      return -1;
    }
    function renameFile(oldPtr, newPtr) {
      const data = backingRead(guestPath(oldPtr));
      if (data == null) {
        setErrno(ERRNO.ENOENT);
        return -1;
      }
      backingWrite(guestPath(newPtr), data);
      backingRemove(guestPath(oldPtr));
      return 0;
    }
    function puts(ptr) {
      return streamWriteText(1, readCString(ptr) + "\n") >= 0 ? 0 : -1;
    }
    function putchar(c) {
      streamWrite(1, Uint8Array.of(c & 255));
      return c;
    }
    function fputs(ptr, stream) {
      const bytes = textEncoder.encode(readCString(ptr));
      return streamWrite(stream, bytes) === bytes.length ? 0 : -1;
    }
    function putc(c, stream) {
      return streamWrite(stream, Uint8Array.of(c & 255)) === 1 ? c : -1;
    }
    function fwrite(ptr, size, nmemb, stream) {
      const total = size * nmemb;
      if (total === 0) return 0;
      const written = streamWrite(stream, readBytes(ptr, total));
      return Math.floor(written / size);
    }
    function fread(ptr, size, nmemb, stream) {
      if (size === 0 || nmemb === 0) return 0;
      const data = streamRead(stream, size * nmemb);
      if (data.length) writeBytes(ptr, data);
      return Math.floor(data.length / size);
    }
    function fgetc(stream) {
      return streamGetByte(stream);
    }
    function ungetc(c, stream) {
      if (c < 0) return -1;
      streamUnget(stream, c);
      return c;
    }
    function fgets(s, size, stream) {
      if (size <= 0) return 0;
      if (size === 1) {
        u8()[s] = 0;
        return s;
      }
      const bytes = [];
      while (bytes.length < size - 1) {
        const c = streamGetByte(stream);
        if (c < 0) break;
        bytes.push(c);
        if (c === 10) break;
      }
      if (bytes.length === 0) return 0;
      writeBytes(s, Uint8Array.from(bytes));
      u8()[s + bytes.length] = 0;
      return s;
    }
    function fdopen(fd, modePtr) {
      const base = readCString(modePtr)[0];
      if (base !== "r" && base !== "w" && base !== "a") {
        setErrno(ERRNO.EINVAL);
        return 0;
      }
      if (fd >= 0 && fd <= 2) return fd;
      if (!openFiles.has(fd)) {
        setErrno(ERRNO.EBADF);
        return 0;
      }
      return fd;
    }
    let tmpfileCounter = 0;
    function tmpfile() {
      let path;
      do {
        tmpfileCounter++;
        const rand2 = Math.floor(Math.random() * 4294967296).toString(36);
        path = `/tmpfile-${tmpfileCounter.toString(36)}-${rand2}`;
      } while (backingRead(path) != null);
      const handle = nextHandle++;
      const st = {
        path,
        data: new Uint8Array(0),
        pos: 0,
        readable: true,
        writable: true,
        append: false,
        eof: false,
        err: false,
        unget: [],
        temp: true
      };
      openFiles.set(handle, st);
      flushStream(st);
      return handle;
    }
    function setvbuf(stream, buf, mode, size) {
      if (stream >= 0 && stream <= 2) return 0;
      const st = openFiles.get(stream);
      if (!st) {
        setErrno(ERRNO.EBADF);
        return -1;
      }
      st.bufMode = mode;
      st.bufPtr = buf;
      st.bufSize = size;
      return 0;
    }
    function setbuf(stream, buf) {
      return setvbuf(stream, buf, buf ? 0 : 2, buf ? 1024 : 0);
    }
    function perror(sPtr) {
      const prefix = sPtr ? readCString(sPtr) : "";
      const text = (prefix ? `${prefix}: ` : "") + strerrorText(errnoValue) + "\n";
      streamWriteText(2, text);
    }
    function scanFrom(getByte, putBack, fmtPtr, vaPtr) {
      const fmt = readCString(fmtPtr);
      const va = makeVa(vaPtr);
      let assigned = 0;
      let consumed = 0;
      let inputEof = false;
      const held = [];
      const rd = () => {
        if (held.length) {
          consumed++;
          return held.pop();
        }
        const c = getByte();
        if (c < 0) inputEof = true;
        else consumed++;
        return c;
      };
      const un = (c) => {
        if (c >= 0) {
          held.push(c);
          consumed--;
        }
      };
      const pk = () => {
        const c = rd();
        if (c >= 0) un(c);
        return c;
      };
      const isWs = (c) => c === 32 || c >= 9 && c <= 13;
      const skipWs = () => {
        for (; ; ) {
          const c = rd();
          if (c < 0) return false;
          if (!isWs(c)) {
            un(c);
            return true;
          }
        }
      };
      const DIGITS = "0123456789abcdefghijklmnopqrstuvwxyz";
      function scanIntToken(conv, width) {
        let text = "";
        const room = () => width < 0 || text.length < width;
        if (room()) {
          const c = pk();
          if (c === 43 || c === 45) text += String.fromCharCode(rd());
        }
        let radix = conv === "x" || conv === "X" ? 16 : conv === "o" ? 8 : conv === "i" ? 0 : 10;
        if ((conv === "x" || conv === "X" || conv === "i") && room()) {
          const a = pk();
          if (a === 48) {
            rd();
            text += "0";
            const b = room() ? pk() : -1;
            if (b === 120 || b === 88) {
              rd();
              const d = pk();
              if (d >= 0 && DIGITS.indexOf(String.fromCharCode(d).toLowerCase()) < 16 && DIGITS.indexOf(String.fromCharCode(d).toLowerCase()) >= 0) {
                text += "x";
                radix = 16;
              } else {
                un(b);
              }
            }
          }
        }
        if (radix === 0) {
          radix = text.includes("x") ? 16 : text.replace(/^[+-]/, "").startsWith("0") ? 8 : 10;
        }
        let digits = 0;
        while (room()) {
          const c = pk();
          if (c < 0) break;
          const d = DIGITS.indexOf(String.fromCharCode(c).toLowerCase());
          if (d < 0 || d >= radix) break;
          text += String.fromCharCode(rd());
          digits++;
        }
        const digitText = text.replace(/^[+-]/, "").replace(/^0[xX]/, "");
        if (digitText.length === 0) {
          for (let i2 = text.length - 1; i2 >= 0; i2--) un(text.charCodeAt(i2));
          return { fail: inputEof && text.length === 0 ? "input" : "match" };
        }
        let v = 0n;
        const br = BigInt(radix);
        for (const ch of digitText) {
          v = v * br + BigInt(DIGITS.indexOf(ch.toLowerCase()));
        }
        if (text.startsWith("-")) v = -v;
        return { value: v };
      }
      function scanFloatToken(width) {
        let text = "";
        const room = () => width < 0 || text.length < width;
        if (room()) {
          const c = pk();
          if (c === 43 || c === 45) text += String.fromCharCode(rd());
        }
        const word = (w) => {
          const base = text;
          let t = text;
          for (const ch of w) {
            if (!room()) return false;
            const c = pk();
            if (c < 0 || String.fromCharCode(c).toLowerCase() !== ch) {
              for (let i2 = t.length - 1; i2 >= base.length; i2--) un(t.charCodeAt(i2));
              return false;
            }
            t += String.fromCharCode(rd());
          }
          text = t;
          return true;
        };
        if (word("inf")) {
          if (!word("inity")) {
          }
          return { value: text.startsWith("-") ? -Infinity : Infinity };
        }
        if (word("nan")) return { value: NaN };
        let sawDigit = false;
        let sawDot = false;
        while (room()) {
          const c = pk();
          if (c < 0) break;
          if (c >= 48 && c <= 57) {
            text += String.fromCharCode(rd());
            sawDigit = true;
          } else if (c === 46 && !sawDot) {
            text += String.fromCharCode(rd());
            sawDot = true;
          } else break;
        }
        if (!sawDigit) {
          for (let i2 = text.length - 1; i2 >= 0; i2--) un(text.charCodeAt(i2));
          return { fail: inputEof && text.length === 0 ? "input" : "match" };
        }
        if (room()) {
          const c = pk();
          if (c === 101 || c === 69) {
            rd();
            let exp = "e";
            if (room()) {
              const s = pk();
              if (s === 43 || s === 45) exp += String.fromCharCode(rd());
            }
            let expDigits = "";
            while (room()) {
              const d = pk();
              if (d < 48 || d > 57) break;
              expDigits += String.fromCharCode(rd());
            }
            if (expDigits) text += exp + expDigits;
            else {
              const tail = exp.slice(1);
              for (let i2 = tail.length - 1; i2 >= 0; i2--) un(tail.charCodeAt(i2));
              un(c);
            }
          }
        }
        return { value: parseFloat(text) };
      }
      function storeInt(ptr, value, length) {
        if (length === "ll" || length === "j") {
          dv().setBigInt64(ptr, value, true);
        } else if (length === "hh") {
          u8()[ptr] = Number(value & 0xffn);
        } else if (length === "h") {
          dv().setInt16(ptr, Number(BigInt.asIntN(16, value)), true);
        } else {
          dv().setInt32(ptr, Number(BigInt.asIntN(32, value)), true);
        }
      }
      let result2 = null;
      let i = 0;
      while (i < fmt.length && result2 === null) {
        const fc = fmt[i];
        if (isWs(fc.charCodeAt(0))) {
          while (i < fmt.length && isWs(fmt.charCodeAt(i))) i++;
          if (!skipWs()) {
            result2 = assigned > 0 ? assigned : -1;
          }
          continue;
        }
        if (fc !== "%") {
          const c = rd();
          if (c < 0) result2 = assigned > 0 ? assigned : -1;
          else if (c !== fc.charCodeAt(0)) {
            un(c);
            result2 = assigned;
          }
          i++;
          continue;
        }
        i++;
        if (fmt[i] === "%") {
          const c = rd();
          if (c < 0) result2 = assigned > 0 ? assigned : -1;
          else if (c !== 37) {
            un(c);
            result2 = assigned;
          }
          i++;
          continue;
        }
        let suppress = false;
        if (fmt[i] === "*") {
          suppress = true;
          i++;
        }
        let width = -1;
        if (i < fmt.length && fmt[i] >= "0" && fmt[i] <= "9") {
          width = 0;
          while (i < fmt.length && fmt[i] >= "0" && fmt[i] <= "9") {
            width = width * 10 + (fmt.charCodeAt(i++) - 48);
          }
        }
        let length = "";
        if (fmt.startsWith("hh", i)) {
          length = "hh";
          i += 2;
        } else if (fmt.startsWith("ll", i)) {
          length = "ll";
          i += 2;
        } else if (i < fmt.length && "hlzjtL".includes(fmt[i])) length = fmt[i++];
        const conv = fmt[i++] ?? "";
        const argPtr = suppress || conv === "%" ? 0 : va.i32();
        const fail = (kind) => {
          result2 = kind === "input" && assigned === 0 ? -1 : assigned;
        };
        switch (conv) {
          case "d":
          case "i":
          case "u":
          case "x":
          case "X":
          case "o": {
            if (!skipWs()) {
              fail("input");
              break;
            }
            const r = scanIntToken(conv === "d" || conv === "u" ? "d" : conv, width);
            if (r.fail) {
              fail(r.fail);
              break;
            }
            if (!suppress) {
              storeInt(argPtr, r.value, length);
              assigned++;
            }
            break;
          }
          case "f":
          case "e":
          case "E":
          case "g":
          case "G":
          case "a":
          case "A": {
            if (!skipWs()) {
              fail("input");
              break;
            }
            const r = scanFloatToken(width);
            if (r.fail) {
              fail(r.fail);
              break;
            }
            if (!suppress) {
              if (length === "l" || length === "L") {
                dv().setFloat64(argPtr, r.value, true);
              } else {
                dv().setFloat32(argPtr, Math.fround(r.value), true);
              }
              assigned++;
            }
            break;
          }
          case "c": {
            const want = width < 0 ? 1 : width;
            const bytes = [];
            while (bytes.length < want) {
              const c = rd();
              if (c < 0) break;
              bytes.push(c);
            }
            if (bytes.length === 0) {
              fail("input");
              break;
            }
            if (!suppress) {
              writeBytes(argPtr, Uint8Array.from(bytes));
              assigned++;
            }
            break;
          }
          case "s": {
            if (!skipWs()) {
              fail("input");
              break;
            }
            const bytes = [];
            while (width < 0 || bytes.length < width) {
              const c = pk();
              if (c < 0 || isWs(c)) break;
              bytes.push(rd());
            }
            if (bytes.length === 0) {
              fail(inputEof ? "input" : "match");
              break;
            }
            if (!suppress) {
              writeBytes(argPtr, Uint8Array.from(bytes));
              u8()[argPtr + bytes.length] = 0;
              assigned++;
            }
            break;
          }
          case "[": {
            let negate = false;
            if (fmt[i] === "^") {
              negate = true;
              i++;
            }
            const set = /* @__PURE__ */ new Set();
            let first = true;
            while (i < fmt.length && (fmt[i] !== "]" || first)) {
              first = false;
              if (fmt[i + 1] === "-" && fmt[i + 2] !== void 0 && fmt[i + 2] !== "]") {
                const lo = fmt.charCodeAt(i);
                const hi = fmt.charCodeAt(i + 2);
                for (let c = lo; c <= hi; c++) set.add(c);
                i += 3;
              } else {
                set.add(fmt.charCodeAt(i));
                i++;
              }
            }
            i++;
            const bytes = [];
            while (width < 0 || bytes.length < width) {
              const c = pk();
              if (c < 0) break;
              if (set.has(c) === negate) break;
              bytes.push(rd());
            }
            if (bytes.length === 0) {
              fail(inputEof ? "input" : "match");
              break;
            }
            if (!suppress) {
              writeBytes(argPtr, Uint8Array.from(bytes));
              u8()[argPtr + bytes.length] = 0;
              assigned++;
            }
            break;
          }
          case "p": {
            if (!skipWs()) {
              fail("input");
              break;
            }
            const r = scanIntToken("x", width);
            if (r.fail) {
              fail(r.fail);
              break;
            }
            if (!suppress) {
              dv().setInt32(argPtr, Number(BigInt.asIntN(32, r.value)), true);
              assigned++;
            }
            break;
          }
          case "n": {
            if (!suppress) storeInt(argPtr, BigInt(consumed), length);
            break;
          }
          default:
            result2 = assigned;
            break;
        }
      }
      for (let k = 0; k < held.length; k++) putBack(held[k]);
      return result2 === null ? assigned : result2;
    }
    function scanf(fmt, va) {
      return scanFrom(() => streamGetByte(0), (c) => streamUnget(0, c), fmt, va);
    }
    function fscanf(stream, fmt, va) {
      return scanFrom(
        () => streamGetByte(stream),
        (c) => streamUnget(stream, c),
        fmt,
        va
      );
    }
    function sscanf(str, fmt, va) {
      let idx = 0;
      const get = () => {
        const c = u8()[str + idx];
        if (c === 0) return -1;
        idx++;
        return c;
      };
      return scanFrom(get, () => {
      }, fmt, va);
    }
    const f64view = new DataView(new ArrayBuffer(8));
    const f64Bits = (x) => {
      f64view.setFloat64(0, x, true);
      return f64view.getBigUint64(0, true);
    };
    const bitsToF64 = (b) => {
      f64view.setBigUint64(0, b, true);
      return f64view.getFloat64(0, true);
    };
    const f32Bits = (x) => {
      f64view.setFloat32(0, x, true);
      return f64view.getUint32(0, true);
    };
    const bitsToF32 = (b) => {
      f64view.setUint32(0, b >>> 0, true);
      return f64view.getFloat32(0, true);
    };
    const F64_SIGN = 1n << 63n;
    const F64_FRAC = (1n << 52n) - 1n;
    function decomposeF64(x) {
      const bits = f64Bits(x);
      const neg = (bits & F64_SIGN) !== 0n;
      const be = Number(bits >> 52n & 0x7ffn);
      const frac = bits & F64_FRAC;
      if (be !== 0) return { neg, sig: frac | 1n << 52n, exp: be - 1075 };
      if (frac === 0n) return { neg, sig: 0n, exp: 0 };
      let sig = frac;
      let shift = 0;
      while ((sig & 1n << 52n) === 0n) {
        sig <<= 1n;
        shift++;
      }
      return { neg, sig, exp: -1074 - shift };
    }
    function composeF64(neg, sig, exp) {
      if (sig === 0n) return neg ? -0 : 0;
      let bits = sig.toString(2).length;
      if (bits > 53) {
        const drop = BigInt(bits - 53);
        const dropped = sig & (1n << drop) - 1n;
        const half = 1n << drop - 1n;
        sig >>= drop;
        exp += bits - 53;
        if (dropped > half || dropped === half && (sig & 1n) === 1n) {
          sig += 1n;
          if (sig === 1n << 53n) {
            sig >>= 1n;
            exp += 1;
          }
        }
        bits = 53;
      }
      const topExp = exp + bits - 1;
      if (topExp > 1023) return neg ? -Infinity : Infinity;
      const sign = neg ? F64_SIGN : 0n;
      if (topExp >= -1022) {
        const s = sig << BigInt(53 - bits);
        return bitsToF64(sign | BigInt(topExp + 1023) << 52n | s & F64_FRAC);
      }
      const sh = exp + 1074;
      let units;
      if (sh >= 0) {
        units = sig << BigInt(sh);
      } else if (-sh > bits) {
        units = 0n;
      } else {
        const drop = BigInt(-sh);
        const dropped = sig & (1n << drop) - 1n;
        const half = 1n << drop - 1n;
        units = sig >> drop;
        if (dropped > half || dropped === half && (units & 1n) === 1n) units += 1n;
      }
      return bitsToF64(sign | units);
    }
    function composeF32(neg, sig, exp) {
      if (sig === 0n) return neg ? -0 : 0;
      let bits = sig.toString(2).length;
      if (bits > 24) {
        const drop = BigInt(bits - 24);
        const dropped = sig & (1n << drop) - 1n;
        const half = 1n << drop - 1n;
        sig >>= drop;
        exp += bits - 24;
        if (dropped > half || dropped === half && (sig & 1n) === 1n) {
          sig += 1n;
          if (sig === 1n << 24n) {
            sig >>= 1n;
            exp += 1;
          }
        }
        bits = 24;
      }
      const topExp = exp + bits - 1;
      if (topExp > 127) return neg ? -Infinity : Infinity;
      const sign = neg ? 2147483648 : 0;
      if (topExp >= -126) {
        const s = Number(sig << BigInt(24 - bits));
        return bitsToF32((sign | topExp + 127 << 23 | s & 8388607) >>> 0);
      }
      const sh = exp + 149;
      let units;
      if (sh >= 0) {
        units = sig << BigInt(sh);
      } else if (-sh > bits) {
        units = 0n;
      } else {
        const drop = BigInt(-sh);
        const dropped = sig & (1n << drop) - 1n;
        const half = 1n << drop - 1n;
        units = sig >> drop;
        if (dropped > half || dropped === half && (units & 1n) === 1n) units += 1n;
      }
      return bitsToF32((sign | Number(units)) >>> 0);
    }
    function frexpCore(x) {
      if (x === 0 || !Number.isFinite(x)) return [x, 0];
      const d = decomposeF64(x);
      const m = (d.neg ? -1 : 1) * Number(d.sig) * 2 ** -53;
      return [m, d.exp + 53];
    }
    function frexp(x, eptr) {
      const [m, e] = frexpCore(x);
      dv().setInt32(eptr, e, true);
      return m;
    }
    function frexpf(x, eptr) {
      const [m, e] = frexpCore(x);
      dv().setInt32(eptr, e, true);
      return Math.fround(m);
    }
    function ilogbImpl(x) {
      if (Number.isNaN(x)) return 2147483647;
      if (x === 0) return -2147483647 - 1;
      if (!Number.isFinite(x)) return 2147483647;
      return decomposeF64(x).exp + 52;
    }
    function logbImpl(x) {
      if (Number.isNaN(x)) return NaN;
      if (x === 0) return -Infinity;
      if (!Number.isFinite(x)) return Infinity;
      return ilogbImpl(x);
    }
    function scalbnImpl(x, n) {
      if (x === 0 || !Number.isFinite(x) || n === 0) return x;
      const d = decomposeF64(x);
      return composeF64(d.neg, d.sig, d.exp + n);
    }
    function exp2Impl(x) {
      if (Number.isInteger(x) && x >= -1074 && x <= 1023) {
        if (x >= -1022) return bitsToF64(BigInt(x + 1023) << 52n);
        return bitsToF64(1n << BigInt(x + 1074));
      }
      return Math.pow(2, x);
    }
    function powImpl(x, y) {
      if (y === 0) return 1;
      if (x === 1) return 1;
      if (x === -1 && !Number.isFinite(y) && !Number.isNaN(y)) return 1;
      return Math.pow(x, y);
    }
    const TWO_POW_52 = 4503599627370496;
    function rintImpl(x) {
      if (!Number.isFinite(x) || x === 0) return x;
      const ax = Math.abs(x);
      if (ax >= TWO_POW_52) return x;
      const r = ax + TWO_POW_52 - TWO_POW_52;
      return x < 0 ? -r : r;
    }
    function roundImpl(x) {
      if (!Number.isFinite(x) || x === 0) return x;
      const r = Math.round(Math.abs(x));
      return x < 0 ? -r : r;
    }
    function toLongSat(x) {
      if (Number.isNaN(x)) return 0;
      if (x >= 2147483648) return 2147483647;
      if (x < -2147483648) return -2147483647 - 1;
      return x | 0;
    }
    function toLLongSat(x) {
      if (Number.isNaN(x)) return 0n;
      if (x >= 9223372036854776e3) return 9223372036854775807n;
      if (x < -9223372036854776e3) return -9223372036854775808n;
      return BigInt(x);
    }
    function modfCore(x) {
      if (Number.isNaN(x)) return [NaN, NaN];
      if (!Number.isFinite(x)) return [x < 0 ? -0 : 0, x];
      if (x === 0) return [x, x];
      const ip = Math.trunc(x);
      return [x - ip, ip];
    }
    function modf(x, iptr) {
      const [frac, ip] = modfCore(x);
      dv().setFloat64(iptr, ip, true);
      return frac;
    }
    function modff(x, iptr) {
      const [frac, ip] = modfCore(x);
      dv().setFloat32(iptr, Math.fround(ip), true);
      return Math.fround(frac);
    }
    function remainderCore(x, y) {
      const dx = decomposeF64(x);
      const dy = decomposeF64(y);
      const e = Math.min(dx.exp, dy.exp);
      const X = dx.sig << BigInt(dx.exp - e);
      const Y = dy.sig << BigInt(dy.exp - e);
      let q = X / Y;
      const rem = X % Y;
      const twice = rem * 2n;
      if (twice > Y || twice === Y && (q & 1n) === 1n) q += 1n;
      const rInt = X - q * Y;
      const r = composeF64(dx.neg !== rInt < 0n, rInt < 0n ? -rInt : rInt, e);
      return [r, dx.neg !== dy.neg ? -q : q];
    }
    function remainderImpl(x, y) {
      if (Number.isNaN(x) || Number.isNaN(y)) return NaN;
      if (!Number.isFinite(x) || y === 0) return NaN;
      if (!Number.isFinite(y) || x === 0) return x;
      return remainderCore(x, y)[0];
    }
    function remquoImpl(x, y, qptr) {
      if (Number.isNaN(x) || Number.isNaN(y) || !Number.isFinite(x) || y === 0) return NaN;
      if (!Number.isFinite(y) || x === 0) {
        dv().setInt32(qptr, 0, true);
        return x;
      }
      const [r, n] = remainderCore(x, y);
      dv().setInt32(qptr, Number(BigInt.asIntN(32, n)), true);
      return r;
    }
    function remquofImpl(x, y, qptr) {
      return Math.fround(remquoImpl(x, y, qptr));
    }
    function nextafterImpl(x, y) {
      if (Number.isNaN(x) || Number.isNaN(y)) return NaN;
      if (x === y) return y;
      if (x === 0) return bitsToF64(f64Bits(y) & F64_SIGN | 1n);
      let b = f64Bits(x);
      const up = y > x;
      if ((b & F64_SIGN) === 0n) b += up ? 1n : -1n;
      else b += up ? -1n : 1n;
      return bitsToF64(b);
    }
    function nextafterfImpl(x, y) {
      if (Number.isNaN(x) || Number.isNaN(y)) return NaN;
      if (x === y) return Math.fround(y);
      if (x === 0) return bitsToF32((f32Bits(y) & 2147483648 | 1) >>> 0);
      let b = f32Bits(x);
      const up = y > x;
      if ((b & 2147483648) === 0) b += up ? 1 : -1;
      else b += up ? -1 : 1;
      return bitsToF32(b >>> 0);
    }
    function copysignImpl(x, y) {
      return bitsToF64(f64Bits(x) & ~F64_SIGN | f64Bits(y) & F64_SIGN);
    }
    function copysignfImpl(x, y) {
      return bitsToF32((f32Bits(x) & 2147483647 | f32Bits(y) & 2147483648) >>> 0);
    }
    function fmaxImpl(a, b) {
      if (Number.isNaN(a)) return b;
      if (Number.isNaN(b)) return a;
      return Math.max(a, b);
    }
    function fminImpl(a, b) {
      if (Number.isNaN(a)) return b;
      if (Number.isNaN(b)) return a;
      return Math.min(a, b);
    }
    function fdimImpl(a, b) {
      if (Number.isNaN(a) || Number.isNaN(b)) return NaN;
      return a > b ? a - b : 0;
    }
    function fmaExact(x, y, z, decompose, compose) {
      const dx = decompose(x);
      const dy = decompose(y);
      const dz = decompose(z);
      const pSig = dx.sig * dy.sig;
      const pExp = dx.exp + dy.exp;
      const e = Math.min(pExp, dz.exp);
      let total = pSig << BigInt(pExp - e);
      if (dx.neg !== dy.neg) total = -total;
      let zTerm = dz.sig << BigInt(dz.exp - e);
      if (dz.neg) zTerm = -zTerm;
      total += zTerm;
      const neg = total < 0n;
      return compose(neg, neg ? -total : total, e);
    }
    function fmaImpl(x, y, z) {
      if (Number.isNaN(x) || Number.isNaN(y) || Number.isNaN(z)) return NaN;
      if (!Number.isFinite(x) || !Number.isFinite(y)) {
        const p = x * y;
        return Number.isNaN(p) ? NaN : p + z;
      }
      if (!Number.isFinite(z)) return z;
      if (x === 0 || y === 0) return x * y + z;
      if (z === 0) return x * y;
      return fmaExact(x, y, z, decomposeF64, composeF64);
    }
    function decomposeF32(x) {
      const bits = f32Bits(x);
      const neg = (bits & 2147483648) !== 0;
      const be = bits >> 23 & 255;
      const frac = BigInt(bits & 8388607);
      if (be !== 0) return { neg, sig: frac | 1n << 23n, exp: be - 150 };
      if (frac === 0n) return { neg, sig: 0n, exp: 0 };
      let sig = frac;
      let shift = 0;
      while ((sig & 1n << 23n) === 0n) {
        sig <<= 1n;
        shift++;
      }
      return { neg, sig, exp: -149 - shift };
    }
    function fmafImpl(x, y, z) {
      if (Number.isNaN(x) || Number.isNaN(y) || Number.isNaN(z)) return NaN;
      if (!Number.isFinite(x) || !Number.isFinite(y)) {
        const p = x * y;
        return Number.isNaN(p) ? NaN : Math.fround(p + z);
      }
      if (!Number.isFinite(z)) return Math.fround(z);
      if (x === 0 || y === 0) return Math.fround(x * y + z);
      if (z === 0) return Math.fround(x * y);
      return fmaExact(x, y, z, decomposeF32, composeF32);
    }
    const SQRT_PI = 1.772453850905516;
    function erfSeries(ax) {
      const z = ax * ax;
      let term = 1;
      let sum = 1;
      for (let k = 0; k < 300; k++) {
        term *= 2 * z / (2 * k + 3);
        sum += term;
        if (term < sum * 1e-18) break;
      }
      return 2 * ax / SQRT_PI * Math.exp(-z) * sum;
    }
    function erfcFraction(x) {
      const TINY = 1e-300;
      let f = TINY;
      let c = TINY;
      let d = 0;
      for (let n = 1; n <= 1e3; n++) {
        const a = n === 1 ? 1 : (n - 1) / 2;
        d = x + a * d;
        if (d === 0) d = TINY;
        d = 1 / d;
        c = x + a / c;
        if (c === 0) c = TINY;
        const delta = c * d;
        f *= delta;
        if (Math.abs(delta - 1) < 1e-17) break;
      }
      return Math.exp(-x * x) / SQRT_PI * f;
    }
    function erfImpl(x) {
      if (Number.isNaN(x)) return NaN;
      if (!Number.isFinite(x)) return x > 0 ? 1 : -1;
      const ax = Math.abs(x);
      if (ax >= 6) return x < 0 ? -1 : 1;
      const r = ax <= 2.5 ? erfSeries(ax) : 1 - erfcFraction(ax);
      return x < 0 ? -r : r;
    }
    function erfcImpl(x) {
      if (Number.isNaN(x)) return NaN;
      if (!Number.isFinite(x)) return x > 0 ? 0 : 2;
      if (x < 0) return 2 - erfcImpl(-x);
      if (x >= 27.5) return 0;
      return x < 0.75 ? 1 - erfSeries(x) : erfcFraction(x);
    }
    const LANCZOS = [
      0.9999999999998099,
      676.5203681218851,
      -1259.1392167224028,
      771.3234287776531,
      -176.6150291621406,
      12.507343278686905,
      -0.13857109526572012,
      9984369578019572e-21,
      15056327351493116e-23
    ];
    const HALF_LOG_2PI = 0.9189385332046727;
    function logGammaAbs(x) {
      if (x !== 0 && Math.abs(x) < 1e-8) {
        return -Math.log(Math.abs(x)) - 0.5772156649015329 * x;
      }
      if (x < 0.5) {
        return Math.log(Math.PI) - Math.log(Math.abs(Math.sin(Math.PI * x))) - logGammaAbs(1 - x);
      }
      const z = x - 1;
      let a = LANCZOS[0];
      for (let k = 1; k < LANCZOS.length; k++) a += LANCZOS[k] / (z + k);
      const t = z + 7.5;
      return HALF_LOG_2PI + (z + 0.5) * Math.log(t) - t + Math.log(a);
    }
    function lgammaImpl(x) {
      if (Number.isNaN(x)) return NaN;
      if (x === 0 || Number.isInteger(x) && x < 0) return Infinity;
      if (!Number.isFinite(x)) return Infinity;
      return logGammaAbs(x);
    }
    function tgammaImpl(x) {
      if (Number.isNaN(x)) return NaN;
      if (x === 0) return 1 / x;
      if (Number.isInteger(x) && x < 0) return NaN;
      if (!Number.isFinite(x)) return x > 0 ? Infinity : NaN;
      if (x < 0) {
        return Math.PI / (Math.sin(Math.PI * x) * Math.exp(logGammaAbs(1 - x)));
      }
      return Math.exp(logGammaAbs(x));
    }
    function nanPayload(tagPtr) {
      if (!tagPtr) return 0n;
      const t = readCString(tagPtr).replace(/^\s+/, "");
      try {
        if (/^0[xX][0-9a-fA-F]+$/.test(t)) return BigInt(t);
        if (/^0[0-7]+$/.test(t)) return BigInt(`0o${t.slice(1)}`);
        if (/^[0-9]+$/.test(t)) return BigInt(t);
      } catch {
        return 0n;
      }
      return 0n;
    }
    function nanImpl(tagPtr) {
      return bitsToF64(0x7ff8000000000000n | nanPayload(tagPtr) & (1n << 51n) - 1n);
    }
    function nanfImpl(tagPtr) {
      return bitsToF32(2143289344 | Number(nanPayload(tagPtr) & 0x3fffffn));
    }
    const asF32 = (fn) => (...args) => Math.fround(fn(...args));
    const env = {
      // memory / string
      memcpy,
      memmove,
      memset,
      memcmp,
      strlen,
      strcpy,
      strncpy,
      strcat,
      strncat,
      strcmp,
      strncmp,
      strchr,
      strrchr,
      strstr,
      strdup,
      memchr,
      strspn,
      strcspn,
      strpbrk,
      strtok,
      strerror,
      strcoll,
      strxfrm,
      // allocation
      malloc,
      calloc,
      realloc,
      free,
      // stdio
      printf,
      fprintf,
      vprintf: printf,
      vfprintf: fprintf,
      sprintf,
      vsprintf: sprintf,
      snprintf,
      vsnprintf: snprintf,
      puts,
      putchar,
      fputs,
      putc,
      fputc: putc,
      fwrite,
      fread,
      fgetc,
      getchar: () => fgetc(0),
      ungetc,
      fopen,
      fclose,
      fflush,
      fseek,
      ftell,
      rewind,
      fgetpos,
      fsetpos,
      feof,
      ferror,
      clearerr,
      freopen,
      remove: removeFile,
      rename: renameFile,
      fdopen,
      tmpfile,
      setbuf,
      setvbuf,
      perror,
      fgets,
      scanf,
      fscanf,
      sscanf,
      vscanf: scanf,
      vfscanf: fscanf,
      vsscanf: sscanf,
      // stdlib
      exit,
      abort,
      atexit,
      atoi,
      atol,
      atoll,
      atof,
      strtol,
      strtoul,
      strtoll,
      strtoull,
      strtod,
      strtof,
      abs,
      labs,
      llabs,
      bsearch,
      qsort,
      rand,
      srand,
      getenv: (_name) => 0,
      system: (cmd) => cmd === 0 ? 0 : -1,
      // ctype
      isalpha,
      isdigit,
      isspace,
      isalnum,
      isupper,
      islower,
      ispunct,
      iscntrl,
      isxdigit,
      isblank,
      isprint,
      isgraph,
      tolower,
      toupper,
      // math — the full <math.h> surface (see the implementation
      // block above). The l forms are the double implementations:
      // long double is double in this target's ABI.
      sin: Math.sin,
      sinf: asF32(Math.sin),
      sinl: Math.sin,
      cos: Math.cos,
      cosf: asF32(Math.cos),
      cosl: Math.cos,
      tan: Math.tan,
      tanf: asF32(Math.tan),
      tanl: Math.tan,
      asin: Math.asin,
      asinf: asF32(Math.asin),
      asinl: Math.asin,
      acos: Math.acos,
      acosf: asF32(Math.acos),
      acosl: Math.acos,
      atan: Math.atan,
      atanf: asF32(Math.atan),
      atanl: Math.atan,
      atan2: Math.atan2,
      atan2f: asF32(Math.atan2),
      atan2l: Math.atan2,
      sinh: Math.sinh,
      sinhf: asF32(Math.sinh),
      sinhl: Math.sinh,
      cosh: Math.cosh,
      coshf: asF32(Math.cosh),
      coshl: Math.cosh,
      tanh: Math.tanh,
      tanhf: asF32(Math.tanh),
      tanhl: Math.tanh,
      asinh: Math.asinh,
      asinhf: asF32(Math.asinh),
      asinhl: Math.asinh,
      acosh: Math.acosh,
      acoshf: asF32(Math.acosh),
      acoshl: Math.acosh,
      atanh: Math.atanh,
      atanhf: asF32(Math.atanh),
      atanhl: Math.atanh,
      exp: Math.exp,
      expf: asF32(Math.exp),
      expl: Math.exp,
      exp2: exp2Impl,
      exp2f: asF32(exp2Impl),
      exp2l: exp2Impl,
      expm1: Math.expm1,
      expm1f: asF32(Math.expm1),
      expm1l: Math.expm1,
      frexp,
      frexpf,
      frexpl: frexp,
      ilogb: ilogbImpl,
      ilogbf: ilogbImpl,
      ilogbl: ilogbImpl,
      ldexp: scalbnImpl,
      ldexpf: asF32(scalbnImpl),
      ldexpl: scalbnImpl,
      log: Math.log,
      logf: asF32(Math.log),
      logl: Math.log,
      log10: Math.log10,
      log10f: asF32(Math.log10),
      log10l: Math.log10,
      log1p: Math.log1p,
      log1pf: asF32(Math.log1p),
      log1pl: Math.log1p,
      log2: Math.log2,
      log2f: asF32(Math.log2),
      log2l: Math.log2,
      logb: logbImpl,
      logbf: logbImpl,
      logbl: logbImpl,
      modf,
      modff,
      modfl: modf,
      scalbn: scalbnImpl,
      scalbnf: asF32(scalbnImpl),
      scalbnl: scalbnImpl,
      scalbln: scalbnImpl,
      scalblnf: asF32(scalbnImpl),
      scalblnl: scalbnImpl,
      cbrt: Math.cbrt,
      cbrtf: asF32(Math.cbrt),
      cbrtl: Math.cbrt,
      fabs: Math.abs,
      fabsf: asF32(Math.abs),
      fabsl: Math.abs,
      hypot: Math.hypot,
      hypotf: asF32(Math.hypot),
      hypotl: Math.hypot,
      pow: powImpl,
      powf: asF32(powImpl),
      powl: powImpl,
      sqrt: Math.sqrt,
      sqrtf: asF32(Math.sqrt),
      sqrtl: Math.sqrt,
      erf: erfImpl,
      erff: asF32(erfImpl),
      erfl: erfImpl,
      erfc: erfcImpl,
      erfcf: asF32(erfcImpl),
      erfcl: erfcImpl,
      lgamma: lgammaImpl,
      lgammaf: asF32(lgammaImpl),
      lgammal: lgammaImpl,
      tgamma: tgammaImpl,
      tgammaf: asF32(tgammaImpl),
      tgammal: tgammaImpl,
      ceil: Math.ceil,
      ceilf: asF32(Math.ceil),
      ceill: Math.ceil,
      floor: Math.floor,
      floorf: asF32(Math.floor),
      floorl: Math.floor,
      nearbyint: rintImpl,
      nearbyintf: asF32(rintImpl),
      nearbyintl: rintImpl,
      rint: rintImpl,
      rintf: asF32(rintImpl),
      rintl: rintImpl,
      lrint: (x) => toLongSat(rintImpl(x)),
      lrintf: (x) => toLongSat(rintImpl(x)),
      lrintl: (x) => toLongSat(rintImpl(x)),
      llrint: (x) => toLLongSat(rintImpl(x)),
      llrintf: (x) => toLLongSat(rintImpl(x)),
      llrintl: (x) => toLLongSat(rintImpl(x)),
      round: roundImpl,
      roundf: asF32(roundImpl),
      roundl: roundImpl,
      lround: (x) => toLongSat(roundImpl(x)),
      lroundf: (x) => toLongSat(roundImpl(x)),
      lroundl: (x) => toLongSat(roundImpl(x)),
      llround: (x) => toLLongSat(roundImpl(x)),
      llroundf: (x) => toLLongSat(roundImpl(x)),
      llroundl: (x) => toLLongSat(roundImpl(x)),
      trunc: Math.trunc,
      truncf: asF32(Math.trunc),
      truncl: Math.trunc,
      fmod: (a, b) => a % b,
      fmodf: asF32((a, b) => a % b),
      fmodl: (a, b) => a % b,
      remainder: remainderImpl,
      remainderf: asF32(remainderImpl),
      remainderl: remainderImpl,
      remquo: remquoImpl,
      remquof: remquofImpl,
      remquol: remquoImpl,
      copysign: copysignImpl,
      copysignf: copysignfImpl,
      copysignl: copysignImpl,
      nan: nanImpl,
      nanf: nanfImpl,
      nanl: nanImpl,
      nextafter: nextafterImpl,
      nextafterf: nextafterfImpl,
      nextafterl: nextafterImpl,
      nexttoward: nextafterImpl,
      nexttowardf: nextafterfImpl,
      nexttowardl: nextafterImpl,
      fdim: fdimImpl,
      fdimf: asF32(fdimImpl),
      fdiml: fdimImpl,
      fmax: fmaxImpl,
      fmaxf: asF32(fmaxImpl),
      fmaxl: fmaxImpl,
      fmin: fminImpl,
      fminf: asF32(fminImpl),
      fminl: fminImpl,
      fma: fmaImpl,
      fmaf: fmafImpl,
      fmal: fmaImpl,
      // time
      time: (tloc) => {
        const sec = Math.floor(Date.now() / 1e3);
        if (tloc) writeU32(tloc, sec);
        return sec;
      },
      clock: () => Math.floor(performance.now() * 1e3),
      difftime: (a, b) => a - b
    };
    const result = { env };
    Object.defineProperty(result, "reserveStatic", {
      value: reserveStatic,
      enumerable: false
    });
    Object.defineProperty(result, "allocatorState", {
      value: allocState,
      enumerable: false
    });
    return result;
  }

  // threads.mjs
  var EPERM = 1;
  var ESRCH = 3;
  var EAGAIN = 11;
  var EBUSY = 16;
  var EINVAL = 22;
  var EDEADLK = 35;
  var ETIMEDOUT = 110;
  var HDR_KEY_COUNTER = 524;
  var HDR_TID_COUNTER = 528;
  var PTHREAD_KEYS_MAX = 128;
  var DESTRUCTOR_ITERATIONS = 4;
  var SLICE_MS = 16;
  var SPIN_BUDGET = 2048;
  var ThreadExitSignal = class extends Error {
    constructor(value) {
      super("tinyc: pthread_exit");
      this.name = "ThreadExitSignal";
      this.value = value | 0;
    }
  };
  function jspiError(what) {
    return new Error(
      `tinyc: ${what}: the simulated thread tier needs to park a direct module's stack, which requires WebAssembly.promising (JSPI); this engine does not provide it`
    );
  }
  var nextLocalThreadKey = 1;
  var ThreadRecord = class {
    constructor(id) {
      this.id = id;
      this.state = "runnable";
      this.detached = false;
      this.joined = false;
      this.exitValue = 0;
      this.joiners = [];
      this.waitInfo = null;
      this.parkResolve = null;
      this.wakeValue = void 0;
      this.timer = null;
      this.stepSettle = null;
      this.savedSp = /* @__PURE__ */ new Map();
      this.keyValuesAddr = 0;
      this.invoke = null;
      this.started = false;
      this.localKey = nextLocalThreadKey++;
    }
  };
  var Scheduler = class {
    constructor(proc) {
      this.proc = proc;
      this.threads = /* @__PURE__ */ new Map();
      this.joinedIds = /* @__PURE__ */ new Set();
      this.runnable = [];
      this.hostOps = [];
      this.current = null;
      this.hostDriving = false;
      this.contextOwner = null;
      this.mainThread = null;
      this.mutexQueues = /* @__PURE__ */ new Map();
      this.condQueues = /* @__PURE__ */ new Map();
      this.onceQueues = /* @__PURE__ */ new Map();
      this.keyDtors = new Array(PTHREAD_KEYS_MAX).fill(0);
      this.keyLive = new Array(PTHREAD_KEYS_MAX).fill(false);
      this.processEnd = null;
      this.drivingRun = false;
      this._completion = null;
      this._pumpRunning = false;
      this._nudgeResolve = null;
      this._sliceStart = 0;
    }
    // -- registry ---------------------------------------------------
    createMainThread() {
      const t = new ThreadRecord(1);
      this.mainThread = t;
      this.threads.set(1, t);
      this.contextOwner = t;
      return t;
    }
    /** The thread whose identity the current context acts with:
     *  the running thread, or the main thread while a host
     *  operation drives its context. Primitives that do not need
     *  to block work from either; park() is the single gate that
     *  fails by name when a host context would have to block
     *  (§4.1: the host has no stack to park). */
    actingThread() {
      if (this.current instanceof ThreadRecord) return this.current;
      return this.mainThread;
    }
    // -- pump ---------------------------------------------------------
    /** Start the pump if it is not running. The pump is a
     *  process-lifetime async loop; it idles on a nudge promise
     *  when there is nothing to do. */
    kick() {
      if (this._pumpRunning) return;
      this._pumpRunning = true;
      this._sliceStart = performance.now();
      (async () => {
        try {
          await this._pumpLoop();
        } catch (err) {
          if (!this.processEnd) {
            this.processEnd = { type: "fault", error: err };
          }
          this._settleRun();
        } finally {
          this._pumpRunning = false;
        }
      })();
    }
    _nudge() {
      const r = this._nudgeResolve;
      this._nudgeResolve = null;
      if (r) r();
    }
    _waitNudge() {
      return new Promise((res) => {
        this._nudgeResolve = res;
      });
    }
    async _pumpLoop() {
      for (; ; ) {
        if (this.processEnd) {
          this._settleRun();
          return;
        }
        const op = this.hostOps.shift();
        if (op) {
          await this._runHostOp(op);
          continue;
        }
        const thread = this.runnable.shift();
        if (thread) {
          if (thread.state === "runnable") {
            if (performance.now() - this._sliceStart > SLICE_MS) {
              await new Promise((r) => setTimeout(r, 0));
              this._sliceStart = performance.now();
            }
            await this._step(thread);
            continue;
          }
          continue;
        }
        if (this.drivingRun) {
          const cls = this._classifyStandstill();
          if (cls === "deadlock") {
            this.processEnd = { type: "deadlock", error: this._deadlockError() };
            this._settleRun();
            return;
          }
          if (cls === "all-exited") {
            this.processEnd = { type: "all-exited" };
            this._settleRun();
            return;
          }
        }
        await this._waitNudge();
      }
    }
    /** One rotation step: install the thread's context, start or
     *  resume it, and wait until it parks, yields or settles. */
    async _step(thread) {
      this.current = thread;
      thread.state = "running";
      this.proc._installThreadContext(thread);
      const stepPromise = new Promise((res) => {
        thread.stepSettle = res;
      });
      if (!thread.started) {
        thread.started = true;
        let p;
        try {
          p = thread.invoke();
        } catch (err) {
          p = Promise.reject(err);
        }
        Promise.resolve(p).then(
          (value) => this._entrySettled(thread, { ok: true, value }),
          (err) => this._entrySettled(thread, { ok: false, err })
        );
      } else {
        const res = thread.parkResolve;
        thread.parkResolve = null;
        const wake = thread.wakeValue;
        thread.wakeValue = void 0;
        if (res) res(wake);
      }
      await stepPromise;
      thread.stepSettle = null;
      this.current = null;
    }
    _settleStep(thread) {
      const s = thread.stepSettle;
      thread.stepSettle = null;
      if (s) s();
    }
    /** The entry invocation of a thread returned or threw. The
     *  outcome is classified here, once: exit() from any thread
     *  ends the process (rule 1), pthread_exit and plain returns
     *  are per-thread exits, the main thread's entry return ends
     *  the process (rule 2), anything else is a process fault. */
    _entrySettled(thread, outcome) {
      if (thread.state === "exited") {
        this._settleStep(thread);
        return;
      }
      if (!outcome.ok) {
        const err = outcome.err;
        if (err instanceof ThreadExitSignal) {
          this._exitThread(thread, err.value);
        } else if (isProcessExit(err)) {
          if (!this.processEnd) {
            this.processEnd = { type: "exit", code: err.exitCode | 0 };
          }
          thread.state = "exited";
          this._settleRun();
        } else {
          if (!this.processEnd) {
            this.processEnd = {
              type: "fault",
              error: err instanceof WebAssembly.RuntimeError ? new Error(`tinyc: thread ${thread.id} trapped: ${err.message}`) : err
            };
          }
          thread.state = "exited";
          this._settleRun();
        }
        this._settleStep(thread);
        return;
      }
      if (thread === this.mainThread && thread.mainEntry) {
        if (!this.processEnd) {
          this.processEnd = { type: "entry", result: outcome.value };
        }
        thread.state = "exited";
        this._settleRun();
        this._settleStep(thread);
        return;
      }
      this._exitThread(thread, outcome.value | 0);
      this._settleStep(thread);
    }
    /** Per-thread exit processing (§6.6), in order: TLS key
     *  destructors on the thread's own context, exit value
     *  stored, joiners woken, detached records reaped. */
    _exitThread(thread, value) {
      this.proc._installThreadContext(thread);
      this.current = thread;
      try {
        this._runKeyDestructors(thread);
      } catch (err) {
        if (!this.processEnd) {
          this.processEnd = { type: "fault", error: err };
        }
        this._settleRun();
      } finally {
        this.current = null;
      }
      thread.exitValue = value | 0;
      thread.state = "exited";
      const joiners = thread.joiners.splice(0);
      for (const j of joiners) this.wake(j, "joined");
      if (thread.detached) this._reapThread(thread);
      this._nudge();
    }
    _reapThread(thread) {
      this.threads.delete(thread.id);
      const proc = this.proc;
      for (const record of proc._loadOrder) {
        const stack = record.stackRegions?.get(thread.id);
        if (stack !== void 0) {
          proc._regionFree(stack, record.meta.stackSize, 16);
          record.stackRegions.delete(thread.id);
        }
        const tls = record.tlsAreas?.get(thread.id);
        if (tls !== void 0) {
          proc._regionFree(
            tls,
            record.meta.tlsSize,
            record.meta.tlsAlign || 4
          );
          record.tlsAreas.delete(thread.id);
        }
      }
      if (thread.keyValuesAddr) {
        proc._regionFree(thread.keyValuesAddr, PTHREAD_KEYS_MAX * 4, 4);
        thread.keyValuesAddr = 0;
      }
    }
    _runKeyDestructors(thread) {
      const proc = this.proc;
      if (!thread.keyValuesAddr) return;
      for (let pass = 0; pass < DESTRUCTOR_ITERATIONS; pass++) {
        let called = false;
        for (let key = 0; key < PTHREAD_KEYS_MAX; key++) {
          const dtor = this.keyDtors[key];
          if (!dtor) continue;
          const value = proc._readI32(thread.keyValuesAddr + key * 4);
          if (value === 0) continue;
          proc._writeI32(thread.keyValuesAddr + key * 4, 0);
          called = true;
          const fn = proc.table.get(dtor);
          if (typeof fn === "function") fn(value);
        }
        if (!called) break;
      }
    }
    // -- parking and waking ------------------------------------------
    /** Park the calling thread on a wait the caller has already
     *  queued it for. Returns the promise the blocking shim hands
     *  to the engine; it resolves with the wake value when the
     *  rotation selects the thread again. */
    park(thread, waitInfo) {
      if (this.hostDriving || !(thread instanceof ThreadRecord)) {
        throw new Error(
          "tinyc: a blocking primitive was reached from the host context, which has no stack to park (threads design \xA74.1)"
        );
      }
      if (!this.proc._jspi) throw jspiError("blocking primitive");
      thread.state = "blocked";
      thread.waitInfo = waitInfo;
      thread.parkToken = (thread.parkToken ?? 0) + 1;
      const p = new Promise((res) => {
        thread.parkResolve = res;
      });
      this._settleStep(thread);
      return p;
    }
    /** Park with a deadline: whichever of wake and timeout comes
     *  first resumes the thread; the wake value says which. The
     *  deadline survives queue transfers (a cond waiter moved to
     *  its mutex's queue keeps the same park token). */
    parkTimed(thread, waitInfo, deadlineMs) {
      const p = this.park(thread, waitInfo);
      const token = thread.parkToken;
      const delay = Math.max(0, deadlineMs - Date.now());
      thread.timer = setTimeout(() => {
        thread.timer = null;
        if (thread.state === "blocked" && thread.parkToken === token) {
          this._withdraw(thread);
          this.wake(thread, "timeout");
        }
      }, delay);
      return p;
    }
    /** Park on a host resource (a retrieval). The resource's
     *  completion wakes the thread; the pump counts the wait as
     *  outstanding, never as deadlock. */
    parkOnHost(thread, startResource) {
      const p = this.park(thread, { kind: "host" });
      let resource;
      try {
        resource = startResource();
      } catch (err) {
        resource = Promise.reject(err);
      }
      Promise.resolve(resource).then(
        () => this.wake(thread, "host"),
        () => this.wake(thread, "host")
      );
      return p;
    }
    /** Record a wake: the thread becomes runnable; the pump
     *  resumes it when the rotation reaches it. Never resumes
     *  inline (§4.1). */
    wake(thread, value) {
      if (thread.state !== "blocked") return;
      if (thread.timer) {
        clearTimeout(thread.timer);
        thread.timer = null;
      }
      thread.state = "runnable";
      thread.waitInfo = null;
      thread.wakeValue = value;
      this.runnable.push(thread);
      this._nudge();
    }
    /** A yield: the step ends and the thread re-queues at the
     *  tail (§4.4). The returned promise resolves when the
     *  rotation selects the thread again. */
    yieldThread(thread) {
      if (this.hostDriving || !(thread instanceof ThreadRecord)) {
        throw new Error(
          "tinyc: a yield was reached from the host context, which has no stack to park (threads design \xA74.1)"
        );
      }
      if (!this.proc._jspi) throw jspiError("sched_yield");
      thread.state = "runnable";
      this.runnable.push(thread);
      const p = new Promise((res) => {
        thread.parkResolve = res;
      });
      this._settleStep(thread);
      this._nudge();
      return p;
    }
    /** Remove a blocked thread from whatever queue holds it
     *  (timeout withdrawal, §6.3). Queue entries are cleaned
     *  lazily elsewhere; here the waitInfo identity is what
     *  matters. */
    _withdraw(thread) {
      const info = thread.waitInfo;
      if (!info) return;
      const queue = info.kind === "mutex" ? this.mutexQueues.get(info.addr) : info.kind === "cond" ? this.condQueues.get(info.addr) : info.kind === "once" ? this.onceQueues.get(info.addr) : null;
      if (queue) {
        const i = queue.indexOf(thread);
        if (i >= 0) queue.splice(i, 1);
      }
      if (info.kind === "join" && info.target) {
        const i = info.target.joiners.indexOf(thread);
        if (i >= 0) info.target.joiners.splice(i, 1);
      }
    }
    enqueue(queueMap, addr, thread) {
      let q = queueMap.get(addr);
      if (!q) {
        q = [];
        queueMap.set(addr, q);
      }
      q.push(thread);
    }
    dequeue(queueMap, addr) {
      const q = queueMap.get(addr);
      if (!q) return null;
      while (q.length > 0) {
        const t = q.shift();
        if (t.state === "blocked" && t.waitInfo) return t;
      }
      return null;
    }
    // -- host operations (§4.1) ---------------------------------------
    /** Run a host operation that executes guest code through the
     *  rotation, on the main thread's context. Returns a promise
     *  for the operation's result. */
    enqueueHostOp(fn) {
      return new Promise((resolve, reject) => {
        if (this.processEnd) {
          reject(new Error("tinyc: the process has ended"));
          return;
        }
        this.hostOps.push({ fn, resolve, reject });
        this.kick();
        this._nudge();
      });
    }
    async _runHostOp(op) {
      const main = this.mainThread;
      this.current = main;
      this.hostDriving = true;
      this.proc._installThreadContext(main);
      try {
        const value = await op.fn();
        op.resolve(value);
      } catch (err) {
        if (isProcessExit(err) && !this.processEnd) {
          this.processEnd = { type: "exit", code: err.exitCode | 0 };
          this._settleRun();
        }
        op.reject(err);
      } finally {
        this.hostDriving = false;
        this.current = null;
      }
    }
    // -- run driving ----------------------------------------------------
    /** The promise a threaded runMainModule awaits: resolves
     *  with the processEnd record. */
    runCompletion() {
      if (!this._completion) {
        let resolve;
        const promise = new Promise((res) => {
          resolve = res;
        });
        this._completion = { promise, resolve };
      }
      return this._completion.promise;
    }
    _settleRun() {
      if (this._completion && this.processEnd) {
        const c = this._completion;
        this._completion = null;
        c.resolve(this.processEnd);
      }
      if (this.processEnd) {
        for (const op of this.hostOps.splice(0)) {
          op.reject(new Error("tinyc: the process has ended"));
        }
      }
      this._nudge();
    }
    // -- standstill classification (§4.6) -------------------------------
    _classifyStandstill() {
      let live = 0;
      let syncBlocked = 0;
      for (const t of this.threads.values()) {
        if (t.state === "exited") continue;
        live++;
        if (t.state === "blocked") {
          const kind = t.waitInfo?.kind;
          if (kind === "host" || t.timer) return "waiting";
          syncBlocked++;
        }
      }
      if (live === 0) return "all-exited";
      if (syncBlocked === live) return "deadlock";
      return "waiting";
    }
    _deadlockError() {
      const parts = [];
      for (const t of this.threads.values()) {
        if (t.state !== "blocked" || !t.waitInfo) continue;
        const info = t.waitInfo;
        if (info.kind === "mutex") {
          const owner = this.proc._readI32(info.addr + 4);
          parts.push(
            `thread ${t.id} waits on the mutex at 0x${info.addr.toString(16)} (owner: thread ${owner})`
          );
        } else if (info.kind === "cond") {
          parts.push(
            `thread ${t.id} waits on the condition variable at 0x${info.addr.toString(16)}`
          );
        } else if (info.kind === "join") {
          parts.push(`thread ${t.id} waits to join thread ${info.target.id}`);
        } else if (info.kind === "once") {
          parts.push(
            `thread ${t.id} waits on the pthread_once control at 0x${info.addr.toString(16)}`
          );
        } else {
          parts.push(`thread ${t.id} waits (${info.kind})`);
        }
      }
      return new Error(
        `tinyc: deadlock: no thread can run and nothing is outstanding: ${parts.join("; ")}`
      );
    }
  };
  function isProcessExit(err) {
    return err && (err.name === "ShimExit" || err.name === "WasiExit") && typeof err.exitCode === "number";
  }
  var MUTEX_STATE = 0;
  var MUTEX_OWNER = 4;
  var MUTEX_COUNT = 8;
  var MUTEX_KIND = 12;
  var COND_SEQ = 0;
  var COND_WAITERS = 4;
  var COND_CLOCK = 8;
  function mutexAttempt(ctx, thread, m) {
    const proc = ctx.proc;
    const kind = proc._readI32(m + MUTEX_KIND);
    if (kind === -1) return { code: EINVAL };
    const state = proc._readI32(m + MUTEX_STATE);
    if (state === 0) {
      proc._writeI32(m + MUTEX_STATE, 1);
      proc._writeI32(m + MUTEX_OWNER, thread.id);
      proc._writeI32(m + MUTEX_COUNT, 1);
      return { code: 0 };
    }
    if (proc._readI32(m + MUTEX_OWNER) === thread.id) {
      if (kind === 2) {
        const count = proc._readI32(m + MUTEX_COUNT);
        if (count >= 2147483647) return { code: EDEADLK };
        proc._writeI32(m + MUTEX_COUNT, count + 1);
        return { code: 0 };
      }
      if (kind === 1) return { code: EDEADLK };
      return { block: true };
    }
    proc._writeI32(m + MUTEX_STATE, 2);
    return { block: true };
  }
  function mutexRelease(ctx, thread, m) {
    const proc = ctx.proc;
    const kind = proc._readI32(m + MUTEX_KIND);
    if (kind === -1) return EINVAL;
    if (proc._readI32(m + MUTEX_STATE) === 0) return EINVAL;
    if (proc._readI32(m + MUTEX_OWNER) !== thread.id) return EPERM;
    if (kind === 2) {
      const count = proc._readI32(m + MUTEX_COUNT) - 1;
      proc._writeI32(m + MUTEX_COUNT, count);
      if (count > 0) return 0;
    }
    proc._writeI32(m + MUTEX_STATE, 0);
    proc._writeI32(m + MUTEX_OWNER, 0);
    proc._writeI32(m + MUTEX_COUNT, 0);
    return 0;
  }
  function mutexWakeOne(ctx, m) {
    const sched = ctx.scheduler;
    const waiter = sched.dequeue(sched.mutexQueues, m);
    if (waiter) sched.wake(waiter, "mutex");
  }
  function abstimeToDateMs(proc, abstimePtr) {
    const sec = proc._readI32(abstimePtr);
    const nsec = proc._readI32(abstimePtr + 4);
    return sec * 1e3 + Math.floor(nsec / 1e6);
  }
  function createSimPthreadShims(proc, scheduler) {
    const ctx = { proc, scheduler };
    const cur = () => scheduler.actingThread();
    const keyArray = (thread) => {
      if (!thread.keyValuesAddr) {
        thread.keyValuesAddr = proc._reserveStatic(PTHREAD_KEYS_MAX * 4, 4);
      }
      return thread.keyValuesAddr;
    };
    function mutexLockLoop(thread, m, deadlineMs) {
      const attempt = mutexAttempt(ctx, thread, m);
      if (!attempt.block) return attempt.code;
      scheduler.enqueue(scheduler.mutexQueues, m, thread);
      const info = { kind: "mutex", addr: m };
      const parked = deadlineMs === null ? scheduler.park(thread, info) : scheduler.parkTimed(thread, info, deadlineMs);
      return parked.then((wake) => {
        if (wake === "timeout") return ETIMEDOUT;
        return mutexLockLoop(thread, m, deadlineMs);
      });
    }
    function condWaitImpl(thread, c, m, deadlineMs) {
      const savedCount = proc._readI32(m + MUTEX_KIND) === 2 ? proc._readI32(m + MUTEX_COUNT) : 1;
      proc._writeI32(c + COND_WAITERS, proc._readI32(c + COND_WAITERS) + 1);
      proc._writeI32(m + MUTEX_STATE, 0);
      proc._writeI32(m + MUTEX_OWNER, 0);
      proc._writeI32(m + MUTEX_COUNT, 0);
      mutexWakeOne(ctx, m);
      scheduler.enqueue(scheduler.condQueues, c, thread);
      const info = { kind: "cond", addr: c, mutex: m };
      const parked = deadlineMs === null ? scheduler.park(thread, info) : scheduler.parkTimed(thread, info, deadlineMs);
      return parked.then((wake) => {
        proc._writeI32(c + COND_WAITERS, proc._readI32(c + COND_WAITERS) - 1);
        const code = mutexLockLoop(thread, m, null);
        const restore = () => {
          if (proc._readI32(m + MUTEX_KIND) === 2) {
            proc._writeI32(m + MUTEX_COUNT, savedCount);
          }
          return wake === "timeout" ? ETIMEDOUT : 0;
        };
        if (code && typeof code.then === "function") return code.then(restore);
        return restore();
      });
    }
    const shims = {
      // -- threads --
      pthread_create(threadPtr, attrPtr, startSlot, arg) {
        if (scheduler.hostDriving) {
          throw new Error(
            "tinyc: pthread_create: the host context cannot create a thread (threads design \xA74.1)"
          );
        }
        if (!proc._jspi) throw jspiError("pthread_create");
        let detached = false;
        if (attrPtr) {
          const ds = proc._readI32(attrPtr);
          if (ds !== 0 && ds !== 1) return EINVAL;
          detached = ds === 1;
        }
        let id;
        try {
          id = proc._createThread(startSlot | 0, arg | 0, detached);
        } catch (err) {
          return EAGAIN;
        }
        proc._writeI32(threadPtr, id);
        return 0;
      },
      pthread_join(targetId, retvalPtr) {
        const thread = cur();
        const target = scheduler.threads.get(targetId | 0);
        if (!target) {
          return scheduler.joinedIds.has(targetId | 0) ? EINVAL : ESRCH;
        }
        if (target === thread) return EDEADLK;
        if (target.detached || target.joined) return EINVAL;
        target.joined = true;
        const finish = () => {
          if (retvalPtr) proc._writeI32(retvalPtr, target.exitValue);
          scheduler.joinedIds.add(target.id);
          scheduler._reapThread(target);
          return 0;
        };
        if (target.state === "exited") return finish();
        target.joiners.push(thread);
        return scheduler.park(thread, { kind: "join", target }).then(() => finish());
      },
      pthread_detach(targetId) {
        const target = scheduler.threads.get(targetId | 0);
        if (!target) return ESRCH;
        if (target.detached) return EINVAL;
        target.detached = true;
        if (target.state === "exited") scheduler._reapThread(target);
        return 0;
      },
      pthread_self() {
        return scheduler.actingThread().id;
      },
      pthread_equal(a, b) {
        return a === b ? 1 : 0;
      },
      pthread_exit(value) {
        if (scheduler.hostDriving) {
          throw new Error(
            "tinyc: pthread_exit: the host context has no thread to exit (threads design \xA74.1)"
          );
        }
        throw new ThreadExitSignal(value | 0);
      },
      // -- thread attributes: detach state is honored; the rest
      // round-trip without effect (§6.2) --
      pthread_attr_init(attr) {
        proc._writeI32(attr, 0);
        proc._writeI32(attr + 4, 0);
        proc._writeI32(attr + 8, 0);
        proc._writeI32(attr + 12, 0);
        return 0;
      },
      pthread_attr_destroy() {
        return 0;
      },
      pthread_attr_setdetachstate(attr, state) {
        if (state !== 0 && state !== 1) return EINVAL;
        proc._writeI32(attr, state);
        return 0;
      },
      pthread_attr_getdetachstate(attr, out) {
        proc._writeI32(out, proc._readI32(attr));
        return 0;
      },
      // -- mutexes --
      pthread_mutex_init(m, attrPtr) {
        let kind = 0;
        if (attrPtr) {
          kind = proc._readI32(attrPtr);
          if (kind < 0 || kind > 2) return EINVAL;
        }
        proc._writeI32(m + MUTEX_STATE, 0);
        proc._writeI32(m + MUTEX_OWNER, 0);
        proc._writeI32(m + MUTEX_COUNT, 0);
        proc._writeI32(m + MUTEX_KIND, kind);
        return 0;
      },
      pthread_mutex_destroy(m) {
        if (proc._readI32(m + MUTEX_KIND) === -1) return EINVAL;
        if (proc._readI32(m + MUTEX_STATE) !== 0) return EBUSY;
        const q = scheduler.mutexQueues.get(m);
        if (q && q.some((t) => t.state === "blocked")) return EBUSY;
        proc._writeI32(m + MUTEX_KIND, -1);
        return 0;
      },
      pthread_mutex_lock(m) {
        return mutexLockLoop(cur(), m, null);
      },
      pthread_mutex_trylock(m) {
        const thread = cur();
        const kind = proc._readI32(m + MUTEX_KIND);
        if (kind === -1) return EINVAL;
        if (proc._readI32(m + MUTEX_STATE) === 0) {
          proc._writeI32(m + MUTEX_STATE, 1);
          proc._writeI32(m + MUTEX_OWNER, thread.id);
          proc._writeI32(m + MUTEX_COUNT, 1);
          return 0;
        }
        if (proc._readI32(m + MUTEX_OWNER) === thread.id && kind === 2) {
          const count = proc._readI32(m + MUTEX_COUNT);
          if (count >= 2147483647) return EDEADLK;
          proc._writeI32(m + MUTEX_COUNT, count + 1);
          return 0;
        }
        return EBUSY;
      },
      pthread_mutex_timedlock(m, abstimePtr) {
        return mutexLockLoop(cur(), m, abstimeToDateMs(proc, abstimePtr));
      },
      pthread_mutex_unlock(m) {
        const thread = cur();
        const rc = mutexRelease(ctx, thread, m);
        if (rc === 0) mutexWakeOne(ctx, m);
        return rc;
      },
      pthread_mutexattr_init(attr) {
        proc._writeI32(attr, 0);
        return 0;
      },
      pthread_mutexattr_destroy() {
        return 0;
      },
      pthread_mutexattr_settype(attr, type) {
        if (type < 0 || type > 2) return EINVAL;
        proc._writeI32(attr, type);
        return 0;
      },
      pthread_mutexattr_gettype(attr, out) {
        proc._writeI32(out, proc._readI32(attr));
        return 0;
      },
      // -- condition variables --
      pthread_cond_init(c, attrPtr) {
        let clock = 0;
        if (attrPtr) {
          clock = proc._readI32(attrPtr);
          if (clock !== 0 && clock !== 1) return EINVAL;
        }
        proc._writeI32(c + COND_SEQ, 0);
        proc._writeI32(c + COND_WAITERS, 0);
        proc._writeI32(c + COND_CLOCK, clock);
        proc._writeI32(c + 12, 0);
        return 0;
      },
      pthread_cond_destroy(c) {
        if (proc._readI32(c + COND_WAITERS) > 0) return EBUSY;
        const q = scheduler.condQueues.get(c);
        if (q && q.some((t) => t.state === "blocked")) return EBUSY;
        return 0;
      },
      pthread_cond_wait(c, m) {
        return condWaitImpl(cur(), c, m, null);
      },
      pthread_cond_timedwait(c, m, abstimePtr) {
        const thread = cur();
        const sec = proc._readI32(abstimePtr);
        const nsec = proc._readI32(abstimePtr + 4);
        const abstimeMs = sec * 1e3 + nsec / 1e6;
        const clock = proc._readI32(c + COND_CLOCK);
        const deadlineMs = clock === 1 ? Date.now() + (abstimeMs - performance.now()) : abstimeMs;
        return condWaitImpl(thread, c, m, deadlineMs);
      },
      pthread_cond_signal(c) {
        proc._writeI32(c + COND_SEQ, proc._readI32(c + COND_SEQ) + 1);
        const waiter = scheduler.dequeue(scheduler.condQueues, c);
        if (waiter) transferCondWaiter(ctx, waiter);
        return 0;
      },
      pthread_cond_broadcast(c) {
        proc._writeI32(c + COND_SEQ, proc._readI32(c + COND_SEQ) + 1);
        let waiter;
        while (waiter = scheduler.dequeue(scheduler.condQueues, c)) {
          transferCondWaiter(ctx, waiter);
        }
        return 0;
      },
      pthread_condattr_init(attr) {
        proc._writeI32(attr, 0);
        return 0;
      },
      pthread_condattr_destroy() {
        return 0;
      },
      pthread_condattr_setclock(attr, clock) {
        if (clock !== 0 && clock !== 1) return EINVAL;
        proc._writeI32(attr, clock);
        return 0;
      },
      pthread_condattr_getclock(attr, out) {
        proc._writeI32(out, proc._readI32(attr));
        return 0;
      },
      // -- once --
      pthread_once(ctrl, initSlot) {
        const thread = cur();
        const state = proc._readI32(ctrl);
        if (state === 2) return 0;
        if (state === 0) {
          proc._writeI32(ctrl, 1);
          const fn = proc.table.get(initSlot);
          if (typeof fn === "function") fn();
          proc._writeI32(ctrl, 2);
          const q = scheduler.onceQueues.get(ctrl);
          if (q) {
            for (const w of q.splice(0)) scheduler.wake(w, "once");
          }
          return 0;
        }
        scheduler.enqueue(scheduler.onceQueues, ctrl, thread);
        return scheduler.park(thread, { kind: "once", addr: ctrl }).then(() => 0);
      },
      // -- spin locks --
      pthread_spin_init(s) {
        proc._writeI32(s, 0);
        return 0;
      },
      pthread_spin_destroy(s) {
        return proc._readI32(s) !== 0 ? EBUSY : 0;
      },
      pthread_spin_lock(s) {
        const thread = scheduler.actingThread();
        const spin = () => {
          const view = new Int32Array(proc.memory.buffer);
          let budget = SPIN_BUDGET;
          while (budget-- > 0) {
            if (Atomics.compareExchange(view, s / 4, 0, 1) === 0) return 0;
          }
          return scheduler.yieldThread(thread).then(() => spin());
        };
        return spin();
      },
      pthread_spin_trylock(s) {
        const view = new Int32Array(proc.memory.buffer);
        return Atomics.compareExchange(view, s / 4, 0, 1) === 0 ? 0 : EBUSY;
      },
      pthread_spin_unlock(s) {
        const view = new Int32Array(proc.memory.buffer);
        Atomics.store(view, s / 4, 0);
        return 0;
      },
      // -- thread-local storage (pthread_key) --
      pthread_key_create(keyPtr, dtorSlot) {
        const word = proc._readI32(HDR_KEY_COUNTER);
        if (word >= PTHREAD_KEYS_MAX) return EAGAIN;
        proc._writeI32(HDR_KEY_COUNTER, word + 1);
        scheduler.keyDtors[word] = dtorSlot | 0;
        scheduler.keyLive[word] = true;
        proc._writeI32(keyPtr, word);
        return 0;
      },
      pthread_key_delete(key) {
        key |= 0;
        if (key < 0 || key >= PTHREAD_KEYS_MAX || !scheduler.keyLive[key]) {
          return EINVAL;
        }
        scheduler.keyLive[key] = false;
        scheduler.keyDtors[key] = 0;
        return 0;
      },
      pthread_setspecific(key, value) {
        key |= 0;
        if (key < 0 || key >= PTHREAD_KEYS_MAX || !scheduler.keyLive[key]) {
          return EINVAL;
        }
        const thread = scheduler.actingThread();
        proc._writeI32(keyArray(thread) + key * 4, value | 0);
        return 0;
      },
      pthread_getspecific(key) {
        key |= 0;
        if (key < 0 || key >= PTHREAD_KEYS_MAX || !scheduler.keyLive[key]) {
          return 0;
        }
        const thread = scheduler.actingThread();
        return proc._readI32(keyArray(thread) + key * 4);
      },
      // -- scheduling --
      sched_yield() {
        const thread = scheduler.current;
        if (thread instanceof ThreadRecord && !scheduler.hostDriving) {
          return scheduler.yieldThread(thread);
        }
        return 0;
      }
    };
    function transferCondWaiter(ctx2, waiter) {
      const m = waiter.waitInfo.mutex;
      waiter.waitInfo = { kind: "mutex", addr: m };
      if (proc._readI32(m + MUTEX_STATE) === 0) {
        scheduler.wake(waiter, "cond");
      } else {
        proc._writeI32(m + MUTEX_STATE, 2);
        scheduler.enqueue(scheduler.mutexQueues, m, waiter);
      }
    }
    return shims;
  }
  var PTHREAD_SURFACE_NAMES = [
    "pthread_create",
    "pthread_join",
    "pthread_detach",
    "pthread_self",
    "pthread_equal",
    "pthread_exit",
    "pthread_attr_init",
    "pthread_attr_destroy",
    "pthread_attr_setdetachstate",
    "pthread_attr_getdetachstate",
    "pthread_mutex_init",
    "pthread_mutex_destroy",
    "pthread_mutex_lock",
    "pthread_mutex_trylock",
    "pthread_mutex_timedlock",
    "pthread_mutex_unlock",
    "pthread_mutexattr_init",
    "pthread_mutexattr_destroy",
    "pthread_mutexattr_settype",
    "pthread_mutexattr_gettype",
    "pthread_cond_init",
    "pthread_cond_destroy",
    "pthread_cond_wait",
    "pthread_cond_timedwait",
    "pthread_cond_signal",
    "pthread_cond_broadcast",
    "pthread_condattr_init",
    "pthread_condattr_destroy",
    "pthread_condattr_setclock",
    "pthread_condattr_getclock",
    "pthread_once",
    "pthread_spin_init",
    "pthread_spin_destroy",
    "pthread_spin_lock",
    "pthread_spin_trylock",
    "pthread_spin_unlock",
    "pthread_key_create",
    "pthread_key_delete",
    "pthread_setspecific",
    "pthread_getspecific",
    "sched_yield"
  ];
  function createCoordinatorPthreadShims() {
    const shims = {};
    for (const name of PTHREAD_SURFACE_NAMES) {
      shims[name] = () => {
        throw new Error(
          `tinyc: ${name}: a thread primitive was reached on the coordinator's replica, which never runs program code (threads design \xA75.1)`
        );
      };
    }
    return shims;
  }
  var REGION_CLASSES = 64;
  var REGION_TABLE_SIZE = 16 + REGION_CLASSES * 16;
  var rtEncoder = new TextEncoder();
  var rtDecoder = new TextDecoder();

  // vfs-store.mjs
  var MemoryFsStore = class {
    constructor() {
      this.kind = "memory";
      this._records = /* @__PURE__ */ new Map();
      this.index = /* @__PURE__ */ new Map();
    }
    async open() {
      this.index = /* @__PURE__ */ new Map();
      for (const [path, rec] of this._records) {
        this.index.set(path, { size: rec.size, mtime: rec.mtime });
      }
      return this.index;
    }
    async get(path) {
      const rec = this._records.get(path);
      return rec ? { bytes: rec.bytes, size: rec.size, mtime: rec.mtime } : null;
    }
    async put(path, record) {
      this._records.set(path, {
        bytes: record.bytes,
        size: record.size,
        mtime: record.mtime
      });
    }
    async delete(path) {
      this._records.delete(path);
    }
    async close() {
    }
  };
  var IndexedDbFsStore = class {
    constructor(dbName) {
      this.kind = "indexeddb";
      this.dbName = dbName;
      this.index = /* @__PURE__ */ new Map();
      this._db = null;
    }
    _openDatabase() {
      return new Promise((resolve, reject) => {
        const req = globalThis.indexedDB.open(this.dbName, 1);
        req.onupgradeneeded = () => {
          const db = req.result;
          if (!db.objectStoreNames.contains("files")) db.createObjectStore("files");
        };
        req.onsuccess = () => resolve(req.result);
        req.onerror = () => reject(req.error);
      });
    }
    async open() {
      this._db = await this._openDatabase();
      this.index = /* @__PURE__ */ new Map();
      await new Promise((resolve, reject) => {
        const tx = this._db.transaction("files", "readonly");
        const req = tx.objectStore("files").openCursor();
        req.onsuccess = () => {
          const cursor = req.result;
          if (!cursor) {
            resolve();
            return;
          }
          const value = cursor.value;
          this.index.set(cursor.key, { size: value.size, mtime: value.mtime });
          cursor.continue();
        };
        req.onerror = () => reject(req.error);
        tx.onerror = () => reject(tx.error);
      });
      return this.index;
    }
    _request(request) {
      return new Promise((resolve, reject) => {
        request.onsuccess = () => resolve(request.result);
        request.onerror = () => reject(request.error);
      });
    }
    _transactionDone(tx) {
      return new Promise((resolve, reject) => {
        tx.oncomplete = () => resolve();
        tx.onerror = () => reject(tx.error);
        tx.onabort = () => reject(tx.error);
      });
    }
    async get(path) {
      const tx = this._db.transaction("files", "readonly");
      const rec = await this._request(tx.objectStore("files").get(path));
      return rec ?? null;
    }
    async put(path, record) {
      const tx = this._db.transaction("files", "readwrite");
      tx.objectStore("files").put(
        { bytes: record.bytes, size: record.size, mtime: record.mtime },
        path
      );
      await this._transactionDone(tx);
    }
    async delete(path) {
      const tx = this._db.transaction("files", "readwrite");
      tx.objectStore("files").delete(path);
      await this._transactionDone(tx);
    }
    async close() {
      if (this._db) {
        this._db.close();
        this._db = null;
      }
    }
  };
  function createFsStore(target, options = {}) {
    if (options.fsStore) return options.fsStore;
    if (typeof options.storeFactory === "function") {
      return options.storeFactory(target);
    }
    if (typeof globalThis.indexedDB !== "undefined" && globalThis.indexedDB) {
      return new IndexedDbFsStore(`tinyc-web-fs-${target}`);
    }
    return new MemoryFsStore();
  }
  async function openFsStore(target, options = {}) {
    const injected = Boolean(options.fsStore) || typeof options.storeFactory === "function";
    let store = await createFsStore(target, options);
    try {
      await store.open();
    } catch (err) {
      if (injected || store.kind !== "indexeddb") throw err;
      store = new MemoryFsStore();
      await store.open();
    }
    return store;
  }

  // index.mjs
  var import_meta = {};
  var sysrootManifest = wasm32_wasi_default;
  var SYSROOT_MANIFESTS = Object.freeze({
    "wasm32-wasi": wasm32_wasi_default,
    "x86_64-linux-gnu": x86_64_linux_gnu_default,
    "aarch64-linux-gnu": aarch64_linux_gnu_default,
    "riscv64-linux-gnu": riscv64_linux_gnu_default,
    "x86_64-windows": x86_64_windows_default,
    "aarch64-linux-android": aarch64_linux_android_default,
    "x86_64-linux-android": x86_64_linux_android_default,
    "arm64-apple-darwin": arm64_apple_darwin_default,
    "x86_64-apple-darwin": x86_64_apple_darwin_default
  });
  var SCRIPT_DIR_HREF = (() => {
    try {
      const script = typeof document !== "undefined" ? document.currentScript : null;
      if (script && script.src) return new URL(".", script.src).href;
    } catch {
    }
    return null;
  })();
  var MODULE_URL = (() => {
    try {
      return import_meta.url || null;
    } catch {
      return null;
    }
  })();
  var PAGE_DIR_HREF = (() => {
    try {
      if (typeof location !== "undefined" && location.href) {
        return new URL(".", location.href).href;
      }
    } catch {
    }
    return null;
  })();
  function dirHref(url) {
    return url.endsWith("/") ? url : url + "/";
  }
  function assetUrl(name, baseUrl) {
    const base = baseUrl ? dirHref(baseUrl) : SCRIPT_DIR_HREF ?? MODULE_URL ?? PAGE_DIR_HREF;
    return base ? new URL(name, base).href : name;
  }
  var VIRTUAL_BIN_PATH = sysrootManifest.binPath;
  var TinycError = class extends Error {
    constructor(message, code) {
      super(message);
      this.name = "TinycError";
      if (code !== void 0) this.code = code;
    }
  };
  var TinycMissingExportError = class extends TinycError {
    constructor(exportName) {
      super(
        `tinyc: native module does not export "${exportName}". The WASM build must list it in EXPORTED_FUNCTIONS (see build.sh).`
      );
      this.name = "TinycMissingExportError";
      this.exportName = exportName;
    }
  };
  var DEFAULT_TARGET = "wasm32-wasi";
  var TARGETS = Object.freeze({
    "wasm32-wasi": { runnable: true, description: "WebAssembly (WASI); output runs in the browser" },
    "x86_64-linux-gnu": { runnable: false, description: "Linux x86-64 ELF" },
    "aarch64-linux-gnu": { runnable: false, description: "Linux AArch64 ELF" },
    "riscv64-linux-gnu": { runnable: false, description: "Linux RISC-V 64 ELF" },
    "x86_64-windows": { runnable: false, description: "Windows x86-64 PE" },
    "aarch64-linux-android": { runnable: false, description: "Android AArch64 ELF (bionic)" },
    "x86_64-linux-android": { runnable: false, description: "Android x86-64 ELF (bionic)" },
    "arm64-apple-darwin": { runnable: false, description: "macOS arm64 Mach-O" },
    "x86_64-apple-darwin": { runnable: false, description: "macOS x86-64 Mach-O" }
  });
  var SUPPORTED_TARGETS = Object.freeze(Object.keys(TARGETS));
  var ELF_TARGETS = /* @__PURE__ */ new Set([
    "x86_64-linux-gnu",
    "aarch64-linux-gnu",
    "riscv64-linux-gnu",
    "aarch64-linux-android",
    "x86_64-linux-android"
  ]);
  var ANDROID_TARGETS = /* @__PURE__ */ new Set([
    "aarch64-linux-android",
    "x86_64-linux-android"
  ]);
  function isRunnableTarget(target) {
    return Boolean(TARGETS[target]?.runnable);
  }
  function targetModuleFiles(target) {
    return { envelopeFile: `tcc-${target}.tinyc.js` };
  }
  function assertKnownTarget(target) {
    if (!Object.hasOwn(TARGETS, target)) {
      throw new TinycError(
        `tinyc: unknown target "${target}". Supported targets: ${SUPPORTED_TARGETS.join(", ")}`
      );
    }
  }
  var TinycRunNotSupportedError = class extends TinycError {
    constructor(target) {
      super(
        `tinyc: target "${target}" cannot be run in the browser: in the browser it can only be compiled to a file. Running compiled output is only possible for the "${DEFAULT_TARGET}" target, whose output is WebAssembly and is instantiated and executed by the browser itself.`
      );
      this.name = "TinycRunNotSupportedError";
      this.target = target;
    }
  };
  function lineBuffered(sink) {
    let pending = "";
    const write = (text) => {
      pending += text;
      let idx;
      while ((idx = pending.indexOf("\n")) >= 0) {
        sink(pending.slice(0, idx));
        pending = pending.slice(idx + 1);
      }
    };
    write.flush = () => {
      if (pending.length > 0) {
        sink(pending);
        pending = "";
      }
    };
    return write;
  }
  function createDefaultIO() {
    return {
      out: lineBuffered((line) => console.log(line)),
      err: lineBuffered((line) => console.error(line)),
      in: () => null
    };
  }
  function normalizeIO(partial, base) {
    const io = { ...base ?? createDefaultIO() };
    if (partial) {
      for (const key of ["out", "err", "in"]) {
        if (partial[key] !== void 0) {
          if (typeof partial[key] !== "function") {
            throw new TinycError(`tinyc: io.${key} must be a function`);
          }
          io[key] = partial[key];
        }
      }
    }
    return io;
  }
  function flushIO(io) {
    for (const key of ["out", "err"]) {
      if (typeof io[key]?.flush === "function") io[key].flush();
    }
  }
  var VFS_CACHE_MAX_ENTRIES = 32;
  var VFS_CACHE_MAX_BYTES = 16 * 1024 * 1024;
  var textEncoder2 = new TextEncoder();
  var textDecoder2 = new TextDecoder();
  function normalizeVfsPath(path) {
    if (typeof path !== "string" || path.length === 0) {
      throw new TinycError("tinyc: vfs path must be a non-empty string");
    }
    const parts = [];
    for (const seg of path.split("/")) {
      if (seg === "" || seg === ".") continue;
      if (seg === "..") parts.pop();
      else parts.push(seg);
    }
    return "/" + parts.join("/");
  }
  function contentToBytesSync(content) {
    if (typeof content === "string") return textEncoder2.encode(content);
    if (content instanceof Uint8Array) return new Uint8Array(content);
    if (content instanceof ArrayBuffer) return new Uint8Array(content);
    if (typeof Blob !== "undefined" && content instanceof Blob) return null;
    throw new TinycError(
      "tinyc: addFile content must be a string, Uint8Array, ArrayBuffer, File or Blob"
    );
  }
  async function contentToBytes(content) {
    const syncBytes = contentToBytesSync(content);
    if (syncBytes) return syncBytes;
    return new Uint8Array(await content.arrayBuffer());
  }
  var VirtualFS = class {
    /** `store` is an opened FsStore (see vfs-store.mjs); its persisted
     *  index becomes this filesystem's initial file set. With no store
     *  argument, a volatile in-memory store is used. loadTinyc opens
     *  the per-target store (IndexedDB by default) before building
     *  the instance's VirtualFS. */
    constructor(store = null) {
      this._store = store ?? new MemoryFsStore();
      this.index = new Map(this._store.index ?? []);
      this.includeDirs = [];
      this._emscriptenFS = null;
      this._moduleFsPrefix = "";
      this._cache = /* @__PURE__ */ new Map();
      this._cacheBytes = 0;
      this._pending = /* @__PURE__ */ new Set();
      this._idleError = null;
    }
    /** The kind of the backing store: 'indexeddb' or 'memory'. */
    get storeKind() {
      return this._store.kind;
    }
    _cacheGet(path) {
      const bytes = this._cache.get(path);
      if (bytes) {
        this._cache.delete(path);
        this._cache.set(path, bytes);
      }
      return bytes;
    }
    _cachePut(path, bytes) {
      const prev = this._cache.get(path);
      if (prev) {
        this._cache.delete(path);
        this._cacheBytes -= prev.byteLength;
      }
      this._cache.set(path, bytes);
      this._cacheBytes += bytes.byteLength;
      while (this._cache.size > 1 && (this._cache.size > VFS_CACHE_MAX_ENTRIES || this._cacheBytes > VFS_CACHE_MAX_BYTES)) {
        const oldest = this._cache.keys().next().value;
        this._cacheBytes -= this._cache.get(oldest).byteLength;
        this._cache.delete(oldest);
      }
    }
    _cacheDelete(path) {
      const prev = this._cache.get(path);
      if (prev) {
        this._cache.delete(path);
        this._cacheBytes -= prev.byteLength;
      }
    }
    /** Queue a store operation kicked off by a synchronous method
     *  (addFileSync / removeFile / clear). Tracked so whenIdle() can
     *  await the queue; the first failure is remembered and rethrown
     *  by whenIdle() instead of surfacing as an unhandled rejection. */
    _queueStoreOp(op) {
      let base;
      try {
        base = Promise.resolve(op());
      } catch (err) {
        base = Promise.reject(err);
      }
      const tracked = base.then(
        () => {
          this._pending.delete(tracked);
        },
        (err) => {
          this._pending.delete(tracked);
          if (this._idleError == null) this._idleError = err;
        }
      );
      this._pending.add(tracked);
    }
    /** Resolve once every queued store write/delete has landed.
     *  Rethrows the first queued operation's failure, if any. */
    async whenIdle() {
      while (this._pending.size > 0) {
        await Promise.all([...this._pending]);
      }
      if (this._idleError != null) {
        const err = this._idleError;
        this._idleError = null;
        throw err;
      }
    }
    /** Synchronous half of a write: index, content cache and the
     *  Emscripten FS mirror. Returns the normalized path and the
     *  record to persist. */
    _stage(path, bytes) {
      const p = normalizeVfsPath(path);
      const buf = bytes instanceof Uint8Array ? bytes : new Uint8Array(bytes);
      const meta = { size: buf.byteLength, mtime: Date.now() };
      this.index.set(p, meta);
      this._cachePut(p, buf);
      this.syncFile(p);
      return { p, record: { bytes: buf, size: meta.size, mtime: meta.mtime } };
    }
    /** Add or replace a file. `content` may be a string, Uint8Array,
     *  ArrayBuffer, File or Blob. For the content kinds whose bytes
     *  are at hand (string, Uint8Array, ArrayBuffer) the file is
     *  staged — index, content cache and the Emscripten FS mirror —
     *  synchronously, before this call returns its promise, so a
     *  compile or read issued immediately afterwards, without
     *  awaiting, already sees the file; Blob and File content stages
     *  once its bytes have been read. The returned promise resolves
     *  once the content is persisted in the store. */
    async addFile(path, content) {
      const bytes = contentToBytesSync(content) ?? await contentToBytes(content);
      const { p, record } = this._stage(path, bytes);
      await this._store.put(p, record);
    }
    /** Synchronous write: index, cache and the Emscripten mirror are
     *  updated immediately (so the file can be compiled and read back
     *  synchronously); the store write is queued behind any other
     *  queued operations — await whenIdle() for it to land. */
    addFileSync(path, content) {
      const bytes = typeof content === "string" ? textEncoder2.encode(content) : content;
      const { p, record } = this._stage(path, bytes);
      this._queueStoreOp(() => this._store.put(p, record));
      return p;
    }
    /** Fetch a file from a URL into the virtual filesystem.
     *  `fetchImpl` is injectable for tests and non-window contexts. */
    async addFileFromUrl(path, url, { fetchImpl = globalThis.fetch } = {}) {
      if (typeof fetchImpl !== "function") {
        throw new TinycError("tinyc: addFileFromUrl needs a fetch implementation");
      }
      const res = await fetchImpl(url);
      if (res && res.ok === false) {
        throw new TinycError(`tinyc: fetching ${url} failed with status ${res.status}`);
      }
      const buf = await res.arrayBuffer();
      const bytes = new Uint8Array(buf);
      const { p, record } = this._stage(path, bytes);
      await this._store.put(p, record);
      return p;
    }
    /** Register a directory as a header search directory. Every state
     *  created afterwards gets it via tcc_add_include_path, in
     *  registration order, ahead of the sysroot. */
    registerIncludeDir(path) {
      const p = normalizeVfsPath(path);
      if (!this.includeDirs.includes(p)) this.includeDirs.push(p);
      return p;
    }
    getIncludeDirs() {
      return [...this.includeDirs];
    }
    hasFile(path) {
      return this.index.has(normalizeVfsPath(path));
    }
    /** Synchronous read, served from the content cache: files written
     *  or read during this session are cached, so this covers the
     *  compile/read-back paths. A persisted file that is not cached
     *  throws, naming the async alternative — use readFileAsync. */
    readFile(path) {
      const p = normalizeVfsPath(path);
      if (!this.index.has(p)) {
        throw new TinycError(`tinyc: no such file in virtual FS: ${path}`);
      }
      const bytes = this._cacheGet(p);
      if (!bytes) {
        throw new TinycError(
          `tinyc: ${p} is persisted in the virtual FS store but is not in the in-memory cache; read it with readFileAsync() instead`
        );
      }
      return bytes;
    }
    readTextFile(path) {
      return textDecoder2.decode(this.readFile(path));
    }
    /** Read a file's content from the store, warming the content
     *  cache, so later synchronous reads of the same file succeed. */
    async readFileAsync(path) {
      const p = normalizeVfsPath(path);
      if (!this.index.has(p)) {
        throw new TinycError(`tinyc: no such file in virtual FS: ${path}`);
      }
      const hit = this._cacheGet(p);
      if (hit) return hit;
      const rec = await this._store.get(p);
      if (!rec) {
        throw new TinycError(`tinyc: no such file in virtual FS: ${path}`);
      }
      this._cachePut(p, rec.bytes);
      return rec.bytes;
    }
    async readTextFileAsync(path) {
      return textDecoder2.decode(await this.readFileAsync(path));
    }
    /** Delete one file. Every copy goes: the index entry and cached
     *  content, the persisted record (queued — see whenIdle), and,
     *  when an Emscripten FS is mounted, the mirrored file inside the
     *  native module (an unlink is attempted even for a path missing
     *  from the index, e.g. a file the compiler created natively).
     *  Returns true when the index or the native copy existed. */
    removeFile(path) {
      const p = normalizeVfsPath(path);
      const had = this.index.delete(p);
      this._cacheDelete(p);
      this._queueStoreOp(() => this._store.delete(p));
      let unlinked = false;
      const FS = this._emscriptenFS;
      if (FS && typeof FS.unlink === "function") {
        try {
          FS.unlink(this._nativePath(p));
          unlinked = true;
        } catch {
        }
      }
      return had || unlinked;
    }
    /** Delete every file tracked by this virtual FS except the
     *  mounted sysroot (paths under /sysroot/), which stays so the
     *  instance can keep compiling. Store records are deleted (queued)
     *  and mirrors in the native module FS are unlinked as well.
     *  Returns the number of files removed. */
    clear() {
      let removed = 0;
      for (const p of [...this.index.keys()]) {
        if (p.startsWith("/sysroot/")) continue;
        if (this.removeFile(p)) removed += 1;
      }
      return removed;
    }
    listFiles(prefix = "/") {
      const p = normalizeVfsPath(prefix);
      return [...this.index.keys()].filter((f) => f.startsWith(p)).sort();
    }
    // === Emscripten FS adaptation point (see header note) ===
    /** Mirror the whole virtual FS into the native module's FS. The
     *  mounted FS is registered synchronously; copying the persisted
     *  files is asynchronous (their bytes may have to come out of the
     *  store), so this returns a promise loadTinyc awaits before the
     *  instance is handed out. Callers that mount an empty or
     *  cache-resident FS can ignore the promise.
     *  `moduleFsPrefix` is the directory inside the module FS the
     *  mirror lands under — the bridge's moduleFsPrefix ("/opfs" when
     *  the WasmFS OPFS backend is mounted there, "" for the memory
     *  root). It is remembered here so syncFile() and removeFile()
     *  hit the same locations; the virtual FS's own paths, index and
     *  store are unaffected by it. */
    async mountTo(emscriptenFS, moduleFsPrefix = "") {
      this._emscriptenFS = emscriptenFS ?? null;
      this._moduleFsPrefix = moduleFsPrefix ?? "";
      if (!this._emscriptenFS) return;
      for (const path of [...this.index.keys()]) {
        let bytes = this._cacheGet(path);
        if (!bytes) {
          const rec = await this._store.get(path);
          if (rec) {
            bytes = rec.bytes;
            this._cachePut(path, bytes);
          }
        }
        if (bytes) this._writeNative(path, bytes);
      }
    }
    /** Mirror one file into the native FS from the content cache.
     *  Files written through this FS are synced as they are written;
     *  a file whose content is neither cached nor already mirrored
     *  cannot be synced synchronously and is left to mountTo. */
    syncFile(path) {
      const FS = this._emscriptenFS;
      if (!FS) return;
      const bytes = this._cacheGet(path);
      if (!bytes) return;
      this._writeNative(path, bytes);
    }
    /** The location of a (normalized) virtual path inside the
     *  native module's filesystem: under the mount prefix when one
     *  was given to mountTo, at the module FS root otherwise. */
    _nativePath(path) {
      return this._moduleFsPrefix ? this._moduleFsPrefix + path : path;
    }
    _writeNative(path, bytes) {
      const FS = this._emscriptenFS;
      const nativePath = this._nativePath(path);
      const dir = nativePath.slice(0, nativePath.lastIndexOf("/")) || "/";
      if (typeof FS.mkdirTree === "function") FS.mkdirTree(dir);
      if (typeof FS.unlink === "function") {
        try {
          FS.unlink(nativePath);
        } catch {
        }
      }
      FS.writeFile(nativePath, bytes);
    }
  };
  var NativeBridge = class {
    constructor(native, io) {
      this.native = native;
      this.io = io;
      this.moduleFsPrefix = "";
      if (native && typeof native.ccall === "function") this.mode = "emscripten";
      else if (native && native.memory instanceof WebAssembly.Memory) this.mode = "raw";
      else if (native && typeof native === "object") this.mode = "mock";
      else {
        throw new TinycError(
          "tinyc: unsupported native module shape; pass an Emscripten module, WebAssembly exports, or a mock object implementing the tcc_* functions"
        );
      }
    }
    /** Translate a virtual FS path to the path the native module
     *  knows it by: the normalized path under moduleFsPrefix. With no
     *  prefix (memory backend) the argument passes through exactly as
     *  given, so native calls are unchanged in that mode. */
    modulePath(path) {
      if (!this.moduleFsPrefix) return path;
      return this.moduleFsPrefix + normalizeVfsPath(path);
    }
    has(name) {
      const n = this.native;
      switch (this.mode) {
        case "emscripten":
          return typeof n["_" + name] === "function" || typeof n[name] === "function";
        case "raw":
          return typeof n[name] === "function" || typeof n["_" + name] === "function";
        default:
          return typeof n[name] === "function";
      }
    }
    requireExport(name) {
      if (!this.has(name)) throw new TinycMissingExportError(name);
    }
    _rawFn(name) {
      return this.native[name] ?? this.native["_" + name];
    }
    /** Call a tcc_* style export. argTypes entries: 'number' | 'string'.
     *  In mock mode arguments pass through as JS values untouched. */
    call(name, returnType, argTypes, args) {
      this.requireExport(name);
      if (this.mode === "mock") return this.native[name](...args);
      if (this.mode === "emscripten") {
        return this.native.ccall(name, returnType, argTypes, args);
      }
      return this._callRaw(name, argTypes, args);
    }
    _mem() {
      return new Uint8Array(this.native.memory.buffer);
    }
    _alloc(bytes) {
      const malloc = this._rawFn("malloc");
      if (typeof malloc !== "function") {
        throw new TinycMissingExportError("malloc (needed to pass strings to the raw WASM module)");
      }
      const ptr = malloc(bytes);
      if (!ptr) throw new TinycError("tinyc: WASM malloc returned null");
      return ptr;
    }
    _free(ptr) {
      const free = this._rawFn("free");
      if (typeof free === "function" && ptr) free(ptr);
    }
    _allocCString(str) {
      const bytes = textEncoder2.encode(str + "\0");
      const ptr = this._alloc(bytes.length);
      this._mem().set(bytes, ptr);
      return ptr;
    }
    _callRaw(name, argTypes, args) {
      const fn = this._rawFn(name);
      const ptrs = [];
      try {
        const conv = args.map((arg, i) => {
          if (argTypes[i] === "string") {
            const p = this._allocCString(arg);
            ptrs.push(p);
            return p;
          }
          return arg;
        });
        return fn(...conv);
      } finally {
        for (const p of ptrs) this._free(p);
      }
    }
    /** Call the exported main / tinyc_main with a fully assembled argv.
     *  argvStrings[0] is the program name slot, already filled by the
     *  caller. tinyc_main is preferred when both are exported. */
    callMain(argvStrings) {
      const entry = this.has("tinyc_main") ? "tinyc_main" : "main";
      this.requireExport(entry);
      const argc = argvStrings.length;
      if (this.mode === "mock") return this.native[entry](argc, [...argvStrings]);
      if (this.mode === "emscripten") return this._callMainEmscripten(entry, argvStrings);
      return this._callMainRaw(entry, argvStrings);
    }
    _callMainRaw(entry, argvStrings) {
      const fn = this._rawFn(entry);
      const strPtrs = argvStrings.map((s) => this._allocCString(s));
      const argvPtr = this._alloc((argvStrings.length + 1) * 4);
      try {
        const view = new DataView(this.native.memory.buffer);
        strPtrs.forEach((p, i) => view.setUint32(argvPtr + i * 4, p, true));
        view.setUint32(argvPtr + argvStrings.length * 4, 0, true);
        return fn(argvStrings.length, argvPtr);
      } finally {
        for (const p of strPtrs) this._free(p);
        this._free(argvPtr);
      }
    }
    _callMainEmscripten(entry, argvStrings) {
      const n = this.native;
      if (typeof n._malloc !== "function" || !n.HEAPU8) {
        throw new TinycError(
          "tinyc: Emscripten module must export _malloc and HEAPU8 to assemble argv"
        );
      }
      const strPtrs = argvStrings.map((s) => {
        const bytes = textEncoder2.encode(s + "\0");
        const p = n._malloc(bytes.length);
        n.HEAPU8.set(bytes, p);
        return p;
      });
      const argvPtr = n._malloc((argvStrings.length + 1) * 4);
      try {
        const view = new DataView(n.HEAPU8.buffer);
        strPtrs.forEach((p, i) => view.setUint32(argvPtr + i * 4, p, true));
        view.setUint32(argvPtr + argvStrings.length * 4, 0, true);
        const fn = n["_" + entry];
        return fn(argvStrings.length, argvPtr);
      } finally {
        for (const p of strPtrs) n._free(p);
        n._free(argvPtr);
      }
    }
  };
  var WasiExit = class extends Error {
    constructor(code) {
      super(`tinyc: wasi proc_exit(${code})`);
      this.name = "WasiExit";
      this.exitCode = code;
    }
  };
  var WASI_ERRNO = {
    SUCCESS: 0,
    BADF: 8,
    FAULT: 21,
    INVAL: 28,
    NOSYS: 52,
    SPIPE: 70
  };
  var WASI_RIGHTS_FD_READ = 1n << 1n;
  var WASI_RIGHTS_FD_WRITE = 1n << 6n;
  function createDefaultByteIO() {
    const outDec = new TextDecoder();
    const errDec = new TextDecoder();
    return {
      out: (bytes) => console.log(outDec.decode(bytes, { stream: true })),
      err: (bytes) => console.error(errDec.decode(bytes, { stream: true }))
    };
  }
  function createWasiShim(options, holder, io) {
    const argv = (options.args ?? []).map(String);
    const envPairs = Object.entries(options.env ?? {}).map(([k, v]) => `${k}=${v}`);
    let stdin = options.stdin ?? new Uint8Array(0);
    if (typeof stdin === "string") stdin = textEncoder2.encode(stdin);
    let stdinPos = 0;
    const mem = () => holder.memory;
    const view = () => new DataView(mem().buffer);
    const bytes = () => new Uint8Array(mem().buffer);
    function writeStringTable(ptrAddr, bufAddr, strings) {
      const v = view();
      const m = bytes();
      let offset = 0;
      strings.forEach((s, i) => {
        const encoded = textEncoder2.encode(s + "\0");
        v.setUint32(ptrAddr + i * 4, bufAddr + offset, true);
        m.set(encoded, bufAddr + offset);
        offset += encoded.length;
      });
      return offset;
    }
    const stringTableSize = (strings) => strings.reduce((n, s) => n + textEncoder2.encode(s).length + 1, 0);
    return {
      fd_write(fd, iovs, iovsLen, nwrittenPtr) {
        if (fd !== 1 && fd !== 2) return WASI_ERRNO.BADF;
        if (!mem()) return WASI_ERRNO.FAULT;
        const v = view();
        const m = bytes();
        const chunks = [];
        let total = 0;
        for (let i = 0; i < iovsLen; i++) {
          const ptr = v.getUint32(iovs + i * 8, true);
          const len = v.getUint32(iovs + i * 8 + 4, true);
          chunks.push(m.slice(ptr, ptr + len));
          total += len;
        }
        const merged = new Uint8Array(total);
        let at = 0;
        for (const c of chunks) {
          merged.set(c, at);
          at += c.length;
        }
        (fd === 1 ? io.out : io.err)(merged);
        v.setUint32(nwrittenPtr, total, true);
        return WASI_ERRNO.SUCCESS;
      },
      fd_read(fd, iovs, iovsLen, nreadPtr) {
        if (fd !== 0) return WASI_ERRNO.BADF;
        if (!mem()) return WASI_ERRNO.FAULT;
        const v = view();
        const m = bytes();
        let total = 0;
        for (let i = 0; i < iovsLen && stdinPos < stdin.length; i++) {
          const ptr = v.getUint32(iovs + i * 8, true);
          const len = v.getUint32(iovs + i * 8 + 4, true);
          const n = Math.min(len, stdin.length - stdinPos);
          m.set(stdin.subarray(stdinPos, stdinPos + n), ptr);
          stdinPos += n;
          total += n;
        }
        v.setUint32(nreadPtr, total, true);
        return WASI_ERRNO.SUCCESS;
      },
      fd_close(fd) {
        return fd >= 0 && fd <= 2 ? WASI_ERRNO.SUCCESS : WASI_ERRNO.BADF;
      },
      fd_seek(fd, _offset, _whence, newoffsetPtr) {
        if (fd < 0 || fd > 2) return WASI_ERRNO.BADF;
        if (!mem()) return WASI_ERRNO.FAULT;
        view().setBigUint64(newoffsetPtr, 0n, true);
        return WASI_ERRNO.SPIPE;
      },
      fd_fdstat_get(fd, ptr) {
        if (fd < 0 || fd > 2) return WASI_ERRNO.BADF;
        if (!mem()) return WASI_ERRNO.FAULT;
        const m = bytes();
        m.fill(0, ptr, ptr + 24);
        const v = view();
        m[ptr] = 2;
        v.setBigUint64(ptr + 8, fd === 0 ? WASI_RIGHTS_FD_READ : WASI_RIGHTS_FD_WRITE, true);
        v.setBigUint64(ptr + 16, 0n, true);
        return WASI_ERRNO.SUCCESS;
      },
      proc_exit(code) {
        throw new WasiExit(code);
      },
      args_sizes_get(argcPtr, argvBufSizePtr) {
        if (!mem()) return WASI_ERRNO.FAULT;
        const v = view();
        v.setUint32(argcPtr, argv.length, true);
        v.setUint32(argvBufSizePtr, stringTableSize(argv), true);
        return WASI_ERRNO.SUCCESS;
      },
      args_get(argvPtr, bufPtr) {
        if (!mem()) return WASI_ERRNO.FAULT;
        writeStringTable(argvPtr, bufPtr, argv);
        return WASI_ERRNO.SUCCESS;
      },
      environ_sizes_get(countPtr, sizePtr) {
        if (!mem()) return WASI_ERRNO.FAULT;
        const v = view();
        v.setUint32(countPtr, envPairs.length, true);
        v.setUint32(sizePtr, stringTableSize(envPairs), true);
        return WASI_ERRNO.SUCCESS;
      },
      environ_get(envPtr, bufPtr) {
        if (!mem()) return WASI_ERRNO.FAULT;
        writeStringTable(envPtr, bufPtr, envPairs);
        return WASI_ERRNO.SUCCESS;
      },
      clock_time_get(id, _precision, resultPtr) {
        if (!mem()) return WASI_ERRNO.FAULT;
        const nowNs = id === 0 ? BigInt(Date.now()) * 1000000n : BigInt(Math.round(performance.now() * 1e6));
        view().setBigUint64(resultPtr, nowNs, true);
        return WASI_ERRNO.SUCCESS;
      },
      random_get(bufPtr, len) {
        if (!mem()) return WASI_ERRNO.FAULT;
        const m = bytes();
        const chunk = new Uint8Array(len);
        if (typeof globalThis.crypto?.getRandomValues === "function") {
          for (let at = 0; at < len; at += 65536) {
            globalThis.crypto.getRandomValues(
              chunk.subarray(at, Math.min(at + 65536, len))
            );
          }
        } else {
          for (let i = 0; i < len; i++) chunk[i] = Math.floor(Math.random() * 256);
        }
        m.set(chunk, bufPtr);
        return WASI_ERRNO.SUCCESS;
      },
      sched_yield() {
        return WASI_ERRNO.SUCCESS;
      }
    };
  }
  var globalShimRegistry = /* @__PURE__ */ new Map();
  function splitShimKey(key) {
    const dot = key.indexOf(".");
    if (dot < 0) return { module: "env", name: key };
    return { module: key.slice(0, dot), name: key.slice(dot + 1) };
  }
  function canonicalShimKey(key) {
    const { module, name } = splitShimKey(key);
    return `${module}.${name}`;
  }
  function setShimEntry(map, name, fn) {
    if (typeof name !== "string" || name.length === 0) {
      throw new TinycError("tinyc: shim name must be a non-empty string");
    }
    if (typeof fn !== "function") {
      throw new TinycError(`tinyc: shim "${name}" must be a function`);
    }
    map.set(canonicalShimKey(name), fn);
  }
  function shimEntriesOf(layer) {
    if (!layer) return [];
    if (layer instanceof Map) return [...layer.entries()];
    return Object.entries(layer);
  }
  function registerGlobalShim(name, fn) {
    setShimEntry(globalShimRegistry, name, fn);
  }
  function registerGlobalShims(shims) {
    for (const [name, fn] of shimEntriesOf(shims)) {
      setShimEntry(globalShimRegistry, name, fn);
    }
  }
  function unregisterGlobalShim(name) {
    globalShimRegistry.delete(canonicalShimKey(name));
  }
  function createShimContext(holder, byteIO) {
    return {
      memory: () => holder.memory,
      u8: () => new Uint8Array(holder.memory.buffer),
      readCString(ptr, maxLength = 1 << 16) {
        const m = new Uint8Array(holder.memory.buffer);
        let end = ptr;
        while (end < m.length && end - ptr < maxLength && m[end] !== 0) end++;
        return textDecoder2.decode(m.slice(ptr, end));
      },
      writeCString(ptr, str) {
        const bytes = textEncoder2.encode(str);
        const m = new Uint8Array(holder.memory.buffer);
        m.set(bytes, ptr);
        m[ptr + bytes.length] = 0;
        return bytes.length + 1;
      },
      readBytes(ptr, length) {
        return new Uint8Array(holder.memory.buffer).slice(ptr, ptr + length);
      },
      writeBytes(ptr, data) {
        new Uint8Array(holder.memory.buffer).set(data, ptr);
      },
      io: byteIO
    };
  }
  function buildWasmImports(options, holder) {
    const defaults = createDefaultByteIO();
    const byteIO = {
      out: options.io?.out ?? defaults.out,
      err: options.io?.err ?? defaults.err,
      in: options.io?.in
    };
    const imports = {};
    const put = (moduleName, name, value) => {
      (imports[moduleName] ??= {})[name] = value;
    };
    const wasi = createWasiShim(options, holder, byteIO);
    for (const [name, fn] of Object.entries(wasi)) {
      put("wasi_snapshot_preview1", name, fn);
    }
    const defaultShims = createDefaultShims({
      io: byteIO,
      holder,
      args: options.args,
      env: options.env,
      stdin: options.stdin,
      heapBase: options.heapBase
    });
    for (const [moduleName, fns] of Object.entries(defaultShims)) {
      for (const [name, fn] of Object.entries(fns)) put(moduleName, name, fn);
    }
    const ctx = createShimContext(holder, byteIO);
    for (const layer of [globalShimRegistry, options._instanceShims, options.shims]) {
      for (const [key, fn] of shimEntriesOf(layer)) {
        const { module: moduleName, name } = splitShimKey(key);
        put(moduleName, name, (...cArgs) => fn(ctx, ...cArgs));
      }
    }
    for (const [moduleName, fns] of Object.entries(options.imports ?? {})) {
      for (const [name, value] of Object.entries(fns)) put(moduleName, name, value);
    }
    return imports;
  }
  var TinycMissingImportError = class extends TinycError {
    constructor(missing) {
      const list = missing.map((m) => `${m.module}.${m.name}`);
      super(
        `tinyc: wasm module imports are not satisfied: ${list.join(", ")}. Provide them as shims: runWasmModule(bytes, { shims: { name: fn } }), instance.registerShim(name, fn) / instance.registerShims({ ... }), or registerGlobalShim(name, fn); use a "module.name" key for imports outside the default "env" module.`
      );
      this.name = "TinycMissingImportError";
      this.missing = list;
    }
  };
  function assertAllImportsSatisfied(module, imports) {
    const missing = [];
    for (const imp of WebAssembly.Module.imports(module)) {
      const provided = imports[imp.module]?.[imp.name];
      const ok = imp.kind === "function" ? typeof provided === "function" : provided !== void 0;
      if (!ok) missing.push(imp);
    }
    if (missing.length > 0) throw new TinycMissingImportError(missing);
  }
  function runWasmInstance(instance, options, holder) {
    holder.memory = instance.exports.memory ?? null;
    holder.instance = instance;
    const exports = instance.exports;
    try {
      if (options.entry !== void 0) {
        const fn = exports[options.entry];
        if (typeof fn !== "function") {
          throw new TinycError(`tinyc: wasm module does not export "${options.entry}"`);
        }
        return fn(...options.callArgs ?? options.args ?? []);
      }
      if (typeof exports._start === "function") {
        exports._start();
        return { exitCode: 0 };
      }
      throw new TinycError(
        "tinyc: wasm module exports no _start; pass options.entry to call an exported function"
      );
    } catch (err) {
      if (err instanceof WasiExit) return { exitCode: err.exitCode };
      if (err instanceof ShimExit) return { exitCode: err.exitCode };
      throw err;
    }
  }
  async function runWasmModule(bytes, options = {}) {
    const holder = { memory: null, instance: null };
    const imports = buildWasmImports(options, holder);
    const module = bytes instanceof WebAssembly.Module ? bytes : await WebAssembly.compile(bytes);
    assertAllImportsSatisfied(module, imports);
    const instance = await WebAssembly.instantiate(module, imports);
    return runWasmInstance(instance, options, holder);
  }
  function runWasmModuleSync(bytes, options = {}) {
    const holder = { memory: null, instance: null };
    const imports = buildWasmImports(options, holder);
    const module = bytes instanceof WebAssembly.Module ? bytes : new WebAssembly.Module(bytes);
    assertAllImportsSatisfied(module, imports);
    const instance = new WebAssembly.Instance(module, imports);
    return runWasmInstance(instance, options, holder);
  }
  var DEFAULT_RUN_OUTPUT = "/a.out";
  function textIOToByteIO(io) {
    const outDec = new TextDecoder();
    const errDec = new TextDecoder();
    return {
      out: (chunk) => io.out(outDec.decode(chunk, { stream: true })),
      err: (chunk) => io.err(errDec.decode(chunk, { stream: true }))
    };
  }
  function writeModuleData(instance, data, options = {}) {
    const memory = instance.exports.memory;
    if (!memory) throw new TinycError("tinyc: writeModuleData: module exports no memory");
    const chunk = typeof data === "string" ? textEncoder2.encode(data) : data;
    const malloc = instance.exports.malloc ?? instance.exports._malloc;
    let ptr;
    let free = null;
    if (typeof malloc === "function" && options.offset === void 0) {
      ptr = malloc(chunk.length);
      if (!ptr && chunk.length > 0) throw new TinycError("tinyc: writeModuleData: malloc returned null");
      const freeFn = instance.exports.free ?? instance.exports._free;
      if (typeof freeFn === "function") free = () => freeFn(ptr);
    } else if (options.offset !== void 0) {
      ptr = typeof options.offset === "function" ? options.offset(chunk.length) : options.offset;
    } else {
      throw new TinycError(
        "tinyc: writeModuleData needs an exported malloc or options.offset"
      );
    }
    new Uint8Array(memory.buffer).set(chunk, ptr);
    return { ptr, length: chunk.length, free };
  }
  function readModuleData(instance, ptr, length) {
    const memory = instance.exports.memory;
    if (!memory) throw new TinycError("tinyc: readModuleData: module exports no memory");
    return new Uint8Array(memory.buffer).slice(ptr, ptr + length);
  }
  function readModuleCString(instance, ptr, maxLength = 1 << 16) {
    const memory = instance.exports.memory;
    if (!memory) throw new TinycError("tinyc: readModuleCString: module exports no memory");
    const m = new Uint8Array(memory.buffer);
    let end = ptr;
    while (end < m.length && end - ptr < maxLength && m[end] !== 0) end++;
    return textDecoder2.decode(m.subarray(ptr, end));
  }
  var TCC_OUTPUT = {
    MEMORY: 1,
    EXE: 2,
    OBJ: 3,
    DLL: 4,
    PREPROCESS: 5
  };
  var TinycState = class {
    constructor(instance, handle) {
      this._instance = instance;
      this._bridge = instance._bridge;
      this.handle = handle;
      this._io = null;
      this._deleted = false;
      this._outputTypeSet = false;
      this._ops = [];
      this._recording = false;
    }
    /** Record a compilation operation for run()'s replay. run()
     *  re-compiles on a fresh state with the dylink flavor set
     *  before compiling, because the flavor is a
     *  code-generation-time property the original state (compiled
     *  standalone) cannot gain afterwards. Only successful
     *  operations are recorded; createState's own setup (-nostdlib,
     *  include directories) is not, because createState redoes it
     *  on the replay state. */
    _record(op) {
      if (this._recording) this._ops.push(op);
    }
    /** Replay one recorded operation on this state. A recorded
     *  -wasm-dylink option is dropped: the flavor is run()'s to
     *  control, and a stale one must not override it. */
    _replayOp(op) {
      switch (op.kind) {
        case "options": {
          const kept = op.value.split(/\s+/).filter((t) => t && !t.startsWith("-wasm-dylink")).join(" ");
          if (kept) this.setOptions(kept);
          break;
        }
        case "includePath":
          this.addIncludePath(op.value);
          break;
        case "sysincludePath":
          this.addSysincludePath(op.value);
          break;
        case "libraryPath":
          this.addLibraryPath(op.value);
          break;
        case "library":
          this.addLibrary(op.value);
          break;
        case "libPath":
          this.setLibPath(op.value);
          break;
        case "define":
          this.defineSymbol(op.sym, op.value);
          break;
        case "undefine":
          this.undefineSymbol(op.sym);
          break;
        case "addFile":
          this.addFile(op.value);
          break;
        case "compileString":
          this.compileString(op.value);
          break;
        case "compileFile":
          this.compileFile(op.value);
          break;
        default:
          throw new TinycError(`tinyc: unknown recorded operation ${op.kind}`);
      }
    }
    _call(name, returnType, argTypes, args) {
      if (this._deleted) throw new TinycError("tinyc: state already deleted");
      return this._bridge.call(
        name,
        returnType,
        ["number", ...argTypes],
        [this.handle, ...args]
      );
    }
    _checkRc(name, rc) {
      if (typeof rc === "number" && rc < 0) {
        throw new TinycError(`tinyc: ${name} failed with code ${rc}`, rc);
      }
      return rc;
    }
    /** Per-state IO. The error channel takes effect immediately through
     *  tcc_set_error_func; out/in are kept for JS-side helpers because
     *  native stdout routing is per-module (see Tinyc.setIO). */
    setIO(partial) {
      this._io = normalizeIO(partial, this._io ?? this._instance.io);
      this._installErrorFunc();
      return this;
    }
    get io() {
      return this._io ?? this._instance.io;
    }
    _installErrorFunc() {
      if (!this._bridge.has("tcc_set_error_func")) return;
      const io = this.io;
      const errorFunc = (opaque, msg) => {
        io.err(String(msg) + "\n");
      };
      this._errorFunc = errorFunc;
      if (this._bridge.mode === "mock") {
        this._bridge.native.tcc_set_error_func(this.handle, null, errorFunc);
      } else if (this._bridge.mode === "emscripten") {
        const n = this._bridge.native;
        if (typeof n.addFunction === "function") {
          const ptr = n.addFunction((opaque, msgPtr) => {
            errorFunc(opaque, n.UTF8ToString(msgPtr));
          }, "vii");
          this._errorFuncPtr = ptr;
          n.ccall(
            "tcc_set_error_func",
            null,
            ["number", "number", "number"],
            [this.handle, 0, ptr]
          );
        }
      }
    }
    setLibPath(path) {
      this._call("tcc_set_lib_path", null, ["string"], [this._bridge.modulePath(path)]);
      this._record({ kind: "libPath", value: path });
      return this;
    }
    setOptions(str) {
      const rc = this._checkRc(
        "tcc_set_options",
        this._call("tcc_set_options", "number", ["string"], [str])
      );
      this._record({ kind: "options", value: str });
      return rc;
    }
    addIncludePath(path) {
      const rc = this._checkRc(
        "tcc_add_include_path",
        this._call("tcc_add_include_path", "number", ["string"], [this._bridge.modulePath(path)])
      );
      this._record({ kind: "includePath", value: path });
      return rc;
    }
    addSysincludePath(path) {
      const rc = this._checkRc(
        "tcc_add_sysinclude_path",
        this._call("tcc_add_sysinclude_path", "number", ["string"], [this._bridge.modulePath(path)])
      );
      this._record({ kind: "sysincludePath", value: path });
      return rc;
    }
    addLibraryPath(path) {
      const rc = this._checkRc(
        "tcc_add_library_path",
        this._call("tcc_add_library_path", "number", ["string"], [this._bridge.modulePath(path)])
      );
      this._record({ kind: "libraryPath", value: path });
      return rc;
    }
    addLibrary(name) {
      const rc = this._checkRc(
        "tcc_add_library",
        this._call("tcc_add_library", "number", ["string"], [name])
      );
      this._record({ kind: "library", value: name });
      return rc;
    }
    defineSymbol(sym, value = "1") {
      this._call("tcc_define_symbol", null, ["string", "string"], [sym, value]);
      this._record({ kind: "define", sym, value });
      return this;
    }
    undefineSymbol(sym) {
      this._call("tcc_undefine_symbol", null, ["string"], [sym]);
      this._record({ kind: "undefine", sym });
      return this;
    }
    setOutputType(type) {
      const rc = this._checkRc(
        "tcc_set_output_type",
        this._call("tcc_set_output_type", "number", ["number"], [type])
      );
      this._outputTypeSet = true;
      this._outputType = type;
      return rc;
    }
    /* tcc_set_output_type is not a plain setter in libtcc: it also
     * creates the state's ELF sections (tccelf_new), and upstream's
     * contract is that it MUST be called before any compilation —
     * compiling without it runs tccelf_end_file over sections that
     * do not exist and traps inside put_elf_sym. Establish the
     * MEMORY default lazily, exactly once, at the first compiling
     * entry point; an explicit setOutputType beforehand suppresses
     * the default, and tccelf_new is never run twice on one state. */
    _ensureOutputType() {
      if (!this._outputTypeSet) this.setOutputType(TCC_OUTPUT.MEMORY);
    }
    /** Add a file (source, object or archive) by virtual path. The file
     *  must exist in the instance's virtual FS; it is synced to the
     *  native filesystem first when an adapter is mounted. The path
     *  handed to tcc_add_file is the file's location inside the
     *  module FS (bridge.modulePath), which differs from the virtual
     *  path when the OPFS backend is mounted. */
    addFile(path) {
      const p = normalizeVfsPath(path);
      if (this._bridge.mode !== "mock" && !this._instance.fs.hasFile(p)) {
        throw new TinycError(`tinyc: addFile: ${p} is not in the virtual FS`);
      }
      this._instance.fs.syncFile(p);
      this._ensureOutputType();
      const rc = this._checkRc(
        "tcc_add_file",
        this._call("tcc_add_file", "number", ["string"], [this._bridge.modulePath(p)])
      );
      this._record({ kind: "addFile", value: p });
      return rc;
    }
    compileString(source) {
      this._ensureOutputType();
      const rc = this._checkRc(
        "tcc_compile_string",
        this._call("tcc_compile_string", "number", ["string"], [source])
      );
      this._record({ kind: "compileString", value: source });
      return rc;
    }
    /** Compile a file from the virtual FS: its content is fed through
     *  tcc_compile_string with a #line marker carrying the path, so no
     *  native filesystem access is needed for plain sources. The
     *  marker carries the virtual path exactly as given — it is what
     *  diagnostics show the user — never the file's location inside
     *  the module FS. */
    compileFile(path) {
      const p = normalizeVfsPath(path);
      const src = this._instance.fs.readTextFile(p);
      const marked = `#line 1 "${p}"
${src}`;
      this._ensureOutputType();
      const rc = this._checkRc(
        "tcc_compile_string",
        this._call("tcc_compile_string", "number", ["string"], [marked])
      );
      this._record({ kind: "compileFile", value: p });
      return rc;
    }
    /** Write the compiled product to a virtual path. TCC writes it
     *  inside the module FS (at bridge.modulePath(path)); exportFile
     *  reads it back from there into the virtual FS. */
    outputFile(path) {
      if (!this._driverLibsAdded && ELF_TARGETS.has(this._instance.target) && (this._outputType === TCC_OUTPUT.EXE || this._outputType === TCC_OUTPUT.DLL)) {
        this._driverLibsAdded = true;
        this.addLibrary("m");
        if (ANDROID_TARGETS.has(this._instance.target)) {
          this.addLibrary("dl");
        }
      }
      return this._checkRc(
        "tcc_output_file",
        this._call(
          "tcc_output_file",
          "number",
          ["string"],
          [this._bridge.modulePath(normalizeVfsPath(path))]
        )
      );
    }
    relocate() {
      return this._checkRc(
        "tcc_relocate",
        this._call("tcc_relocate", "number", [], [])
      );
    }
    getSymbol(name) {
      return this._call("tcc_get_symbol", "number", ["string"], [name]);
    }
    /** Run the compiled program. Only on the wasm32 target, and not
     *  through the native module: a WASM module cannot instantiate
     *  another module from inside itself, and native TCC provides no
     *  tcc_run for the wasm target. Instead the glue layer writes the
     *  product to a file in the virtual FS (options.outputPath,
     *  default DEFAULT_RUN_OUTPUT), reads its bytes back, and
     *  instantiates it in the host with runWasmModuleSync — the WASI
     *  argv is [outputPath, ...args]. Returns the program's exit code
     *  for a _start module, or the named entry's return value when
     *  options.entry is given. On every other target the glue layer
     *  throws TinycRunNotSupportedError before anything is passed to
     *  the native module — those targets compile to files only.
     *  options: { outputPath, entry, callArgs, env, imports, shims,
     *  stdin, heapBase, io } — entry/callArgs/env/imports/shims/stdin/
     *  heapBase forward to runWasmModuleSync (and entry/callArgs to
     *  the dylink process's runMainModule), and the instance's
     *  registered shims apply automatically; io is a text IO partial
     *  (default: this state's IO) whose out/err receive the program's
     *  fd_write output, decoded. A dylink product runs through a
     *  threaded process, so for it run returns a promise of the
     *  exit code or entry return value (threads design §2.3); a
     *  standalone product returns the value synchronously. */
    run(args = [], options = {}) {
      if (!isRunnableTarget(this._instance.target)) {
        throw new TinycRunNotSupportedError(this._instance.target);
      }
      const outputPath = options.outputPath ?? DEFAULT_RUN_OUTPUT;
      if (this._instance._dylinkOptionOk()) {
        const replay = this._instance.createState();
        try {
          replay.setOptions("-wasm-dylink=main");
          replay.setOutputType(TCC_OUTPUT.EXE);
          for (const op of this._ops) replay._replayOp(op);
          replay.outputFile(outputPath);
        } finally {
          replay.delete();
        }
      } else {
        this.setOutputType(TCC_OUTPUT.EXE);
        this.outputFile(outputPath);
      }
      const bytes = this._instance.exportFile(outputPath);
      const io = normalizeIO(options.io, this.io);
      if (isDylinkUnit(bytes)) {
        const proc = this._instance.createProcess({
          io,
          args: [outputPath, ...args.map(String)],
          env: options.env,
          stdin: options.stdin,
          shims: options.shims,
          imports: options.imports,
          threads: true
        });
        return Promise.resolve(proc.runMainModule(bytes, {
          path: outputPath,
          entry: options.entry,
          callArgs: options.callArgs
        })).then((result2) => {
          flushIO(io);
          if (result2 && typeof result2 === "object" && "exitCode" in result2) {
            return result2.exitCode;
          }
          return result2;
        });
      }
      const result = runWasmModuleSync(bytes, {
        args: [outputPath, ...args.map(String)],
        entry: options.entry,
        callArgs: options.callArgs,
        env: options.env,
        imports: options.imports,
        shims: options.shims,
        stdin: options.stdin,
        heapBase: options.heapBase,
        _instanceShims: this._instance._shims,
        io: textIOToByteIO(io)
      });
      flushIO(io);
      if (result && typeof result === "object" && "exitCode" in result) {
        return result.exitCode;
      }
      return result;
    }
    delete() {
      if (this._deleted) return;
      this._deleted = true;
      if (this._bridge.has("tcc_delete")) {
        this._bridge.call("tcc_delete", null, [], [this.handle]);
      }
      if (this._errorFuncPtr && typeof this._bridge.native.removeFunction === "function") {
        this._bridge.native.removeFunction(this._errorFuncPtr);
        this._errorFuncPtr = null;
      }
      this._instance._forgetState(this);
    }
    [Symbol.dispose]() {
      this.delete();
    }
  };
  var PATH_VALUE_OPTIONS = /* @__PURE__ */ new Set([
    "-o",
    "-I",
    "-L",
    "-B",
    "-MF",
    "-include",
    "-isystem",
    "-idirafter",
    "-iquote",
    "-iprefix",
    "-iwithprefix",
    "-iwithprefixbefore"
  ]);
  var PATH_JOINED_OPTIONS = [
    "-iwithprefixbefore",
    "-iwithprefix",
    "-idirafter",
    "-isystem",
    "-iprefix",
    "-iquote",
    "-include",
    "-I",
    "-L",
    "-B",
    "-MF",
    "-o"
  ];
  var Tinyc = class {
    constructor(bridge, { fs, io, target = DEFAULT_TARGET }) {
      this._bridge = bridge;
      this.fs = fs;
      this._io = io;
      this.target = target;
      this._states = /* @__PURE__ */ new Set();
      this._mainRunning = false;
      this._shims = /* @__PURE__ */ new Map();
      this._fsBackend = "memory";
    }
    get io() {
      return this._io;
    }
    /** True only for the wasm32 target: its compiled output is
     *  WebAssembly, which the browser itself instantiates and runs.
     *  All other targets compile to files only. */
    get canRun() {
      return isRunnableTarget(this.target);
    }
    get nativeMode() {
      return this._bridge.mode;
    }
    /** The backend mounted at "/opfs" in the native module's
     *  filesystem: 'opfs' when loadTinyc mounted the WasmFS OPFS
     *  backend there, 'memory' when it did not — the browser main
     *  thread, a browser without OPFS, and every non-Emscripten
     *  module land on 'memory'. Decided once per module by loadTinyc
     *  (see tryMountOpfsBackend). The module filesystem's root is the
     *  WasmFS memory backend in both cases; with 'opfs', the mirrored
     *  compile working set and every path handed to the native module
     *  are rooted at "/opfs" (bridge.moduleFsPrefix), so the working
     *  set lives on disk. */
    get fsBackend() {
      return this._fsBackend;
    }
    /** Replace the instance IO. out/err are module-level on the native
     *  side (Emscripten print/printErr were bound at load time to the
     *  functions this object delegates to), so swapping the delegates
     *  here reroutes native output for everything created afterwards
     *  and for callMain. */
    setIO(partial) {
      this._io = normalizeIO(partial, this._io);
      return this;
    }
    /** Register a custom shim on this instance: a JS function served
     *  as a wasm import for every later run made through this
     *  instance (state.run, callMain "-run"). `name` is a bare
     *  function name (module "env") or "module.name" for any other
     *  import module — including wasi_snapshot_preview1 entries. The
     *  function is called as fn(ctx, ...cArgs); see the runWasmModule
     *  section for the ctx convention and the layer priority.
     *  Registering the same name again replaces the previous shim. */
    registerShim(name, fn) {
      setShimEntry(this._shims, name, fn);
      return this;
    }
    /** Bulk form of registerShim: { name: fn, ... }. */
    registerShims(shims) {
      for (const [name, fn] of shimEntriesOf(shims)) {
        setShimEntry(this._shims, name, fn);
      }
      return this;
    }
    /** Remove an instance shim previously added with registerShim. */
    unregisterShim(name) {
      this._shims.delete(canonicalShimKey(name));
      return this;
    }
    // -- virtual FS shortcuts (full API on instance.fs) --
    addFile(path, content) {
      return this.fs.addFile(path, content);
    }
    addFileSync(path, content) {
      return this.fs.addFileSync(path, content);
    }
    addFileFromUrl(path, url, opts) {
      return this.fs.addFileFromUrl(path, url, opts);
    }
    registerIncludeDir(path) {
      return this.fs.registerIncludeDir(path);
    }
    hasFile(path) {
      return this.fs.hasFile(path);
    }
    readFile(path) {
      return this.fs.readFile(path);
    }
    readTextFile(path) {
      return this.fs.readTextFile(path);
    }
    readFileAsync(path) {
      return this.fs.readFileAsync(path);
    }
    readTextFileAsync(path) {
      return this.fs.readTextFileAsync(path);
    }
    removeFile(path) {
      return this.fs.removeFile(path);
    }
    clearFiles() {
      return this.fs.clear();
    }
    listFiles(prefix) {
      return this.fs.listFiles(prefix);
    }
    /** Resolve once the virtual FS's queued store writes and deletes
     *  (from addFileSync / removeFile / clearFiles) have landed in the
     *  persistent store. */
    whenIdle() {
      return this.fs.whenIdle();
    }
    /** Read a compiled product (object file, executable, library —
     *  including a Windows PE for the x86_64-windows target) out of
     *  the virtual FS as bytes. This is how compile-to-file results
     *  leave the browser package, for every target. With an Emscripten
     *  module, a file that only exists in the module FS is read from
     *  there and mirrored into the virtual FS. A file restored from
     *  the persistent store that is not in the content cache falls
     *  back to the module FS copy; when there is none either, the
     *  read fails naming exportFileAsync as the alternative. */
    exportFile(path) {
      const p = normalizeVfsPath(path);
      const FS = this._bridge.native?.FS;
      if (FS && typeof FS.readFile === "function") {
        try {
          const bytes = FS.readFile(this._bridge.modulePath(p));
          this.fs.addFileSync(p, bytes);
          return this.fs.readFile(p);
        } catch {
        }
      }
      if (this.fs.hasFile(p)) {
        try {
          return this.fs.readFile(p);
        } catch {
          throw new TinycError(
            `tinyc: ${p} is persisted in the virtual FS store but is not in the in-memory cache; read it with exportFileAsync() instead`
          );
        }
      }
      throw new TinycError(`tinyc: no such file in virtual FS: ${p}`);
    }
    /** exportFile, reading the content from the persistent store
     *  when it is not in the content cache. */
    async exportFileAsync(path) {
      const p = normalizeVfsPath(path);
      const FS = this._bridge.native?.FS;
      if (FS && typeof FS.readFile === "function") {
        try {
          const bytes = FS.readFile(this._bridge.modulePath(p));
          this.fs.addFileSync(p, bytes);
          return bytes;
        } catch {
        }
      }
      if (this.fs.hasFile(p)) return this.fs.readFileAsync(p);
      throw new TinycError(`tinyc: no such file in virtual FS: ${p}`);
    }
    /** exportFile as a Blob, e.g. for a download link or a File
     *  handed to another tool. */
    exportBlob(path, type = "application/octet-stream") {
      return new Blob([this.exportFile(path)], { type });
    }
    /** Execute a wasm32 product already sitting in the virtual FS —
     *  a file produced by an earlier compile-to-file callMain, or one
     *  placed there with addFile — without recompiling. The bytes are
     *  read with exportFile and instantiated with runWasmModuleSync,
     *  so the shim stack is exactly the one "-run" uses: default libc
     *  shims, package-global shims, this instance's registered shims,
     *  then options.shims and options.imports. options: { args, entry,
     *  callArgs, env, imports, shims, stdin, heapBase, io } — args
     *  defaults to [path]; io is a text IO partial (default: this
     *  instance's IO). Returns the exit code for a _start module, or
     *  the named entry's return value. */
    runWasmFile(path, options = {}) {
      const p = normalizeVfsPath(path);
      const bytes = this.exportFile(p);
      const io = normalizeIO(options.io, this._io);
      const result = runWasmModuleSync(bytes, {
        args: options.args ?? [p],
        entry: options.entry,
        callArgs: options.callArgs,
        env: options.env,
        shims: options.shims,
        imports: options.imports,
        stdin: options.stdin,
        heapBase: options.heapBase,
        _instanceShims: this._shims,
        io: textIOToByteIO(io)
      });
      flushIO(io);
      if (typeof result === "number") return result;
      return result?.exitCode ?? 0;
    }
    /** Command-line mode: assemble argv exactly like a shell would
     *  (argv[0] filled with the virtual binary path) and call the
     *  exported main / tinyc_main. When the OPFS backend is mounted,
     *  path arguments are relocated under "/opfs" first (see
     *  _moduleArgv). Returns the exit code. main is not
     *  reentrant; a nested call (e.g. from an IO callback) throws.
     *  A "-run" argument on a non-wasm32 target is rejected by the
     *  glue layer with TinycRunNotSupportedError before the native
     *  module is called: that target can only compile to a file here.
     *  On the wasm32 target, "-run" is handled by the glue layer
     *  itself (see _callMainRun): the exported main never sees it.
     *  For "-run", the optional second argument forwards run options
     *  ({ shims, imports, stdin, heapBase }) to that execution. */
    callMain(args = [], options = {}) {
      if (!Array.isArray(args)) throw new TinycError("tinyc: callMain expects an array of arguments");
      if (args.some((a) => a === "-run")) {
        if (!this.canRun) throw new TinycRunNotSupportedError(this.target);
        return this._callMainRun(args.map(String), options);
      }
      if (this._mainRunning) {
        throw new TinycError("tinyc: exported main is not reentrant (callMain already running)");
      }
      const argv = this._moduleArgv(args.map(String));
      this._mainRunning = true;
      try {
        const rc = this._bridge.callMain(argv);
        flushIO(this._io);
        return rc;
      } finally {
        this._mainRunning = false;
      }
    }
    /** Assemble the argv handed to the exported main. argv[0] is the
     *  virtual binary path. When the module FS has no prefix (memory
     *  backend) the arguments pass through exactly as given. When the
     *  OPFS backend is mounted (bridge.moduleFsPrefix set), every
     *  argument that names a file inside the module FS — argv[0], the
     *  values of path options in separate and joined form, --sysroot=
     *  values, and bare input-file arguments — is relocated under the
     *  prefix (bridge.modulePath), where the mirrored files actually
     *  live; all other arguments pass through untouched. */
    _moduleArgv(args) {
      const bridge = this._bridge;
      if (!bridge.moduleFsPrefix) return [VIRTUAL_BIN_PATH, ...args];
      const out = [bridge.modulePath(VIRTUAL_BIN_PATH)];
      let pathValueNext = false;
      for (const arg of args) {
        if (pathValueNext) {
          pathValueNext = false;
          out.push(arg.startsWith("-") ? arg : bridge.modulePath(arg));
          continue;
        }
        if (PATH_VALUE_OPTIONS.has(arg)) {
          pathValueNext = true;
          out.push(arg);
          continue;
        }
        if (arg.startsWith("--sysroot=")) {
          out.push("--sysroot=" + bridge.modulePath(arg.slice("--sysroot=".length)));
          continue;
        }
        const joined = PATH_JOINED_OPTIONS.find(
          (opt) => arg.length > opt.length && arg.startsWith(opt)
        );
        if (joined) {
          out.push(joined + bridge.modulePath(arg.slice(joined.length)));
          continue;
        }
        if (!arg.startsWith("-") && arg.length > 0) {
          out.push(bridge.modulePath(arg));
          continue;
        }
        out.push(arg);
      }
      return out;
    }
    /** wasm32 "-run": the arguments before "-run" are compiler
     *  options, the one after it is the source file, and the rest are
     *  the program's arguments. The source is compiled to
     *  DEFAULT_RUN_OUTPUT through the exported main (without "-run"),
     *  then the product bytes are instantiated and executed in the
     *  host by runWasmModuleSync, with the program arguments fed to
     *  _start through the WASI args shim. The instance's registered
     *  shims apply; options.shims / options.imports / options.stdin /
     *  options.heapBase forward per-run. Returns the compiler's exit
     *  code when compilation fails, otherwise the program's. */
    _callMainRun(args, options = {}) {
      const runIdx = args.indexOf("-run");
      const compileArgs = args.slice(0, runIdx);
      const [src, ...progArgs] = args.slice(runIdx + 1);
      if (!src) {
        throw new TinycError('tinyc: callMain: "-run" must be followed by a source file');
      }
      const dylink = this._dylinkOptionOk();
      const rc = this.callMain([
        ...compileArgs,
        ...dylink ? ["-wasm-dylink=main"] : [],
        "-o",
        DEFAULT_RUN_OUTPUT,
        src
      ]);
      if (rc !== 0) return rc;
      const bytes = this.exportFile(DEFAULT_RUN_OUTPUT);
      if (dylink && isDylinkUnit(bytes)) {
        const proc = this.createProcess({
          io: this._io,
          args: [src, ...progArgs],
          shims: options.shims,
          imports: options.imports,
          stdin: options.stdin,
          threads: true
        });
        return Promise.resolve(
          proc.runMainModule(bytes, { path: DEFAULT_RUN_OUTPUT })
        ).then((result2) => {
          flushIO(this._io);
          if (typeof result2 === "number") return result2;
          return result2?.exitCode ?? 0;
        });
      }
      const result = runWasmModuleSync(bytes, {
        args: [src, ...progArgs],
        shims: options.shims,
        imports: options.imports,
        stdin: options.stdin,
        heapBase: options.heapBase,
        _instanceShims: this._shims,
        io: textIOToByteIO(this._io)
      });
      flushIO(this._io);
      if (typeof result === "number") return result;
      return result?.exitCode ?? 0;
    }
    /** True when this instance's compiler accepts the
     *  -wasm-dylink=main option: probed once per instance on a
     *  throwaway state. A compiler without the option rejects it;
     *  the run paths then keep their standalone form. A product is
     *  only run as a dylink main module when it actually is a
     *  dylink unit file, which the run paths check on the bytes
     *  themselves. */
    _dylinkOptionOk() {
      if (this._dylinkOk !== void 0) return this._dylinkOk;
      let ok = false;
      try {
        const probe = this.createState();
        try {
          probe.setOptions("-wasm-dylink=main");
          ok = true;
        } catch {
          ok = false;
        }
        probe.delete();
      } catch {
        ok = false;
      }
      this._dylinkOk = ok;
      return ok;
    }
    /** Create a dylink process attached to this instance: libraries
     *  resolve in this instance's virtual FS and the instance's
     *  registered shims apply to its modules. */
    createProcess(options = {}) {
      return new TinycProcess({ ...options, tinyc: this });
    }
    /** Fine-grained mode: create a TCCState, pre-loaded with the
     *  virtual FS include directories (registration order, ahead of
     *  the sysroot) and the instance error routing. */
    createState() {
      this._bridge.requireExport("tcc_new");
      const handle = this._bridge.call("tcc_new", "number", [], []);
      if (!handle && this._bridge.mode !== "mock") {
        throw new TinycError("tinyc: tcc_new returned null");
      }
      const state = new TinycState(this, handle ?? {});
      this._states.add(state);
      state._installErrorFunc();
      if (isRunnableTarget(this.target)) {
        state.setOptions("-nostdlib");
      }
      if (this._bridge.has("tcc_add_include_path")) {
        for (const dir of this.fs.getIncludeDirs()) state.addIncludePath(dir);
      }
      state._recording = true;
      return state;
    }
    _forgetState(state) {
      this._states.delete(state);
    }
    /** Convenience: compile one source string in a throwaway state. */
    compileString(source, { outputType = TCC_OUTPUT.MEMORY } = {}) {
      const state = this.createState();
      try {
        state.setOutputType(outputType);
        return state.compileString(source);
      } finally {
        state.delete();
      }
    }
  };
  var TinycDylinkError = class extends TinycError {
    constructor(message) {
      super(message);
      this.name = "TinycDylinkError";
    }
  };
  var RTLD_LAZY = 1;
  var RTLD_NOW = 2;
  var RTLD_GLOBAL = 4;
  var RTLD_LOCAL = 8;
  var RTLD_KNOWN = RTLD_LAZY | RTLD_NOW | RTLD_GLOBAL | RTLD_LOCAL;
  var SHA256_K = [
    1116352408,
    1899447441,
    3049323471,
    3921009573,
    961987163,
    1508970993,
    2453635748,
    2870763221,
    3624381080,
    310598401,
    607225278,
    1426881987,
    1925078388,
    2162078206,
    2614888103,
    3248222580,
    3835390401,
    4022224774,
    264347078,
    604807628,
    770255983,
    1249150122,
    1555081692,
    1996064986,
    2554220882,
    2821834349,
    2952996808,
    3210313671,
    3336571891,
    3584528711,
    113926993,
    338241895,
    666307205,
    773529912,
    1294757372,
    1396182291,
    1695183700,
    1986661051,
    2177026350,
    2456956037,
    2730485921,
    2820302411,
    3259730800,
    3345764771,
    3516065817,
    3600352804,
    4094571909,
    275423344,
    430227734,
    506948616,
    659060556,
    883997877,
    958139571,
    1322822218,
    1537002063,
    1747873779,
    1955562222,
    2024104815,
    2227730452,
    2361852424,
    2428436474,
    2756734187,
    3204031479,
    3329325298
  ];
  function sha256Hex(bytes) {
    const rotr = (x, n) => x >>> n | x << 32 - n;
    let h0 = 1779033703, h1 = 3144134277, h2 = 1013904242, h3 = 2773480762;
    let h4 = 1359893119, h5 = 2600822924, h6 = 528734635, h7 = 1541459225;
    const bitLen = bytes.length * 8;
    const padded = new Uint8Array(bytes.length + 9 + 63 >> 6 << 6);
    padded.set(bytes);
    padded[bytes.length] = 128;
    const pv = new DataView(padded.buffer);
    pv.setUint32(padded.length - 4, bitLen >>> 0);
    pv.setUint32(padded.length - 8, Math.floor(bitLen / 2 ** 32));
    const w = new Int32Array(64);
    for (let block = 0; block < padded.length; block += 64) {
      for (let t = 0; t < 16; t++) w[t] = pv.getInt32(block + t * 4);
      for (let t = 16; t < 64; t++) {
        const s0 = rotr(w[t - 15], 7) ^ rotr(w[t - 15], 18) ^ w[t - 15] >>> 3;
        const s1 = rotr(w[t - 2], 17) ^ rotr(w[t - 2], 19) ^ w[t - 2] >>> 10;
        w[t] = w[t - 16] + s0 + w[t - 7] + s1 | 0;
      }
      let a = h0, b = h1, c = h2, d = h3, e = h4, f = h5, g = h6, h = h7;
      for (let t = 0; t < 64; t++) {
        const s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
        const ch = e & f ^ ~e & g;
        const t1 = h + s1 + ch + SHA256_K[t] + w[t] | 0;
        const s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
        const maj = a & b ^ a & c ^ b & c;
        const t2 = s0 + maj | 0;
        h = g;
        g = f;
        f = e;
        e = d + t1 | 0;
        d = c;
        c = b;
        b = a;
        a = t1 + t2 | 0;
      }
      h0 = h0 + a | 0;
      h1 = h1 + b | 0;
      h2 = h2 + c | 0;
      h3 = h3 + d | 0;
      h4 = h4 + e | 0;
      h5 = h5 + f | 0;
      h6 = h6 + g | 0;
      h7 = h7 + h | 0;
    }
    return [h0, h1, h2, h3, h4, h5, h6, h7].map((x) => (x >>> 0).toString(16).padStart(8, "0")).join("");
  }
  function base64ToBytes(b64) {
    const bin = atob(String(b64).replace(/\s+/g, ""));
    const out = new Uint8Array(bin.length);
    for (let i = 0; i < bin.length; i++) out[i] = bin.charCodeAt(i);
    return out;
  }
  function bytesToBase64(bytes) {
    let bin = "";
    const CHUNK = 32768;
    for (let i = 0; i < bytes.length; i += CHUNK) {
      bin += String.fromCharCode(...bytes.subarray(i, i + CHUNK));
    }
    return btoa(bin);
  }
  var DYLINK_REGISTRY = "__TINYC_DYLINK_REGISTRY__";
  function parseManifestEnvelope(text, where) {
    const start = text.indexOf("{");
    const end = text.lastIndexOf("}");
    if (start < 0 || end <= start) {
      throw new TinycDylinkError(
        `tinyc: manifest at ${where} is not in the envelope form`
      );
    }
    try {
      return JSON.parse(text.slice(start, end + 1));
    } catch (err) {
      throw new TinycDylinkError(
        `tinyc: manifest at ${where} does not parse: ${err.message}`
      );
    }
  }
  function manifestEnvelopeText(canonical, manifest) {
    return `globalThis.${DYLINK_REGISTRY} ??= Object.create(null);
globalThis.${DYLINK_REGISTRY}[${JSON.stringify(canonical)}] = ${JSON.stringify(manifest)};
`;
  }
  function manifestToLibrary(manifest, canonical) {
    if (!manifest || typeof manifest !== "object" || Array.isArray(manifest)) {
      throw new TinycDylinkError(`tinyc: manifest for "${canonical}" is not an object`);
    }
    if (manifest.version !== 1) {
      throw new TinycDylinkError(
        `tinyc: manifest for "${canonical}" has unsupported version ${manifest.version}`
      );
    }
    if (typeof manifest.body !== "string") {
      throw new TinycDylinkError(`tinyc: manifest for "${canonical}" carries no body`);
    }
    const bodyBytes = base64ToBytes(manifest.body);
    if (typeof manifest.hash === "string" && sha256Hex(bodyBytes) !== manifest.hash) {
      throw new TinycDylinkError(
        `tinyc: manifest for "${canonical}": body fails its recorded SHA-256 hash`
      );
    }
    const meta = {
      version: 1,
      flavor: manifest.flavor | 0,
      memSize: manifest.mem_size | 0,
      memAlign: manifest.mem_align || 16,
      bssOff: manifest.bss_off | 0,
      bssSize: manifest.bss_size | 0,
      tableSize: manifest.table_size | 0,
      // The threaded revision's fields (threads design §3.2,
      // §7.2): stackSize is undefined on a pre-revision module,
      // which a threaded process refuses by name; the tls fields
      // are present only when the module has __thread variables.
      stackSize: typeof manifest.stack_size === "number" ? manifest.stack_size | 0 : void 0,
      tlsSize: manifest.tls_size | 0,
      tlsAlign: manifest.tls_align || 4,
      tlsTemplate: typeof manifest.tls_template === "string" ? base64ToBytes(manifest.tls_template) : null,
      needed: Array.isArray(manifest.needed) ? manifest.needed.map(String) : [],
      dataGotFunc: Array.isArray(manifest.data_got_func) ? manifest.data_got_func.map(String) : [],
      exports: (Array.isArray(manifest.exports) ? manifest.exports : []).map((e) => ({
        name: String(e.name),
        kind: e.kind | 0,
        value: e.value | 0,
        tls: e.tls ? 1 : 0
      }))
    };
    if (!(meta.memSize > 0)) {
      throw new TinycDylinkError(
        `tinyc: manifest for "${canonical}" has an invalid layout (mem_size)`
      );
    }
    return { meta, bodyBytes };
  }
  function libraryFromBytes(canonical, bytes) {
    const manifest = parseManifestEnvelope(textDecoder2.decode(bytes), `"${canonical}"`);
    const lib = manifestToLibrary(manifest, canonical);
    lib.manifest = manifest;
    return lib;
  }
  function isDylinkUnit(bytes) {
    try {
      libraryFromBytes("/probe", bytes);
      return true;
    } catch {
      return false;
    }
  }
  function createTransport(option) {
    let kind = option ?? "auto";
    if (kind === "auto") {
      kind = "fetch";
      try {
        if (typeof location !== "undefined" && location.protocol === "file:" && typeof document !== "undefined") {
          kind = "script";
        }
      } catch {
      }
    }
    if (kind !== "fetch" && kind !== "script") {
      throw new TinycError(
        `tinyc: transport must be "auto", "fetch" or "script", not "${option}"`
      );
    }
    return {
      kind,
      /** Retrieve the manifest object served at `url` for the library
       *  whose canonical path (and registry key) is `canonical`. */
      async retrieveManifest(url, canonical) {
        if (kind === "script") {
          const reg = globalThis[DYLINK_REGISTRY];
          if (reg && reg[canonical] !== void 0) return reg[canonical];
          if (typeof document === "undefined" || !document.head) {
            throw new TinycDylinkError(
              `tinyc: the script transport needs a page main thread; cannot retrieve ${url}`
            );
          }
          const before = new Set(Object.keys(globalThis[DYLINK_REGISTRY] ?? {}));
          await new Promise((resolve, reject) => {
            const el = document.createElement("script");
            el.src = url;
            el.async = true;
            el.onload = () => {
              el.remove();
              resolve();
            };
            el.onerror = () => {
              el.remove();
              reject(new TinycDylinkError(
                `tinyc: cannot load library "${canonical}" from ${url} (script transport)`
              ));
            };
            document.head.appendChild(el);
          });
          const registry = globalThis[DYLINK_REGISTRY] ?? {};
          if (registry[canonical] !== void 0) return registry[canonical];
          const fresh = Object.keys(registry).filter((k) => !before.has(k));
          if (fresh.length === 1) {
            registry[canonical] = registry[fresh[0]];
            return registry[canonical];
          }
          throw new TinycDylinkError(
            `tinyc: manifest at ${url} did not register "${canonical}"`
          );
        }
        let text;
        try {
          const res = await fetch(url);
          if (!res.ok) throw new Error(`HTTP ${res.status}`);
          text = await res.text();
        } catch (err) {
          throw new TinycDylinkError(
            `tinyc: cannot load library "${canonical}" from ${url}: ${err.message}`
          );
        }
        return parseManifestEnvelope(text, url);
      }
    };
  }
  var ASSET_REGISTRY = "__TINYC_ASSET_REGISTRY__";
  function assetRegistry() {
    return globalThis[ASSET_REGISTRY] ??= /* @__PURE__ */ Object.create(null);
  }
  function parseAssetEnvelopeText(text, url) {
    const start = text.indexOf("{");
    const end = text.lastIndexOf("}");
    if (start < 0 || end <= start) {
      throw new TinycError(
        `tinyc: asset envelope at ${url} is not in the envelope form`
      );
    }
    try {
      return JSON.parse(text.slice(start, end + 1));
    } catch (err) {
      throw new TinycError(
        `tinyc: asset envelope at ${url} does not parse: ${err.message}`
      );
    }
  }
  async function retrieveAsset(transport, url, key, fetchImpl) {
    const registry = assetRegistry();
    if (registry[key] !== void 0) return registry[key];
    if (transport.kind === "fetch") {
      let text;
      try {
        const response = await (fetchImpl ?? globalThis.fetch)(url);
        if (!response.ok) {
          throw new TinycError(
            `tinyc: cannot load asset "${key}" from ${url}: HTTP ${response.status}`
          );
        }
        text = await response.text();
      } catch (err) {
        if (err instanceof TinycError) throw err;
        throw new TinycError(
          `tinyc: cannot load asset "${key}" from ${url}: ${err.message}`
        );
      }
      const payload = parseAssetEnvelopeText(text, url);
      registry[key] = payload;
      return payload;
    }
    const doc = globalThis.document;
    if (!doc?.createElement || typeof doc.createElement !== "function") {
      throw new TinycError(
        `tinyc: the script transport needs a page main thread; cannot retrieve ${url}`
      );
    }
    await new Promise((resolve, reject) => {
      const el = doc.createElement("script");
      el.src = url;
      el.onload = () => {
        el.remove();
        resolve();
      };
      el.onerror = () => {
        el.remove();
        reject(new TinycError(
          `tinyc: cannot load asset "${key}" from ${url} (script transport)`
        ));
      };
      doc.head.appendChild(el);
    });
    return registry[key] ?? null;
  }
  function canonicalModulePath(path, baseDir = "/sysroot") {
    if (typeof path !== "string" || path.length === 0) {
      throw new TinycDylinkError("tinyc: module path must be a non-empty string");
    }
    return path.startsWith("/") ? normalizeVfsPath(path) : normalizeVfsPath(`${baseDir}/${path}`);
  }
  function vfsDirname(p) {
    const i = p.lastIndexOf("/");
    return i <= 0 ? "/" : p.slice(0, i);
  }
  var VALTYPE_NAMES = {
    127: "i32",
    126: "i64",
    125: "f32",
    124: "f64",
    123: "v128",
    112: "funcref",
    111: "externref"
  };
  function parseImportTypes(bytes) {
    const out = /* @__PURE__ */ new Map();
    if (bytes.length < 8) return out;
    let p = 8;
    const uleb = () => {
      let v = 0, s = 0;
      for (; ; ) {
        const c = bytes[p++];
        v += (c & 127) * 2 ** s;
        if (!(c & 128)) return v;
        s += 7;
      }
    };
    const str = () => {
      const n = uleb();
      const t = textDecoder2.decode(bytes.subarray(p, p + n));
      p += n;
      return t;
    };
    let types = [];
    while (p < bytes.length) {
      const id = bytes[p++];
      const size = uleb();
      const end = p + size;
      if (id === 1) {
        const n = uleb();
        for (let i = 0; i < n; i++) {
          p++;
          const np = uleb();
          const params = [];
          for (let j = 0; j < np; j++) params.push(VALTYPE_NAMES[bytes[p++]] ?? "i32");
          const nr = uleb();
          const results = [];
          for (let j = 0; j < nr; j++) results.push(VALTYPE_NAMES[bytes[p++]] ?? "i32");
          types.push({ params, results });
        }
      } else if (id === 2) {
        const n = uleb();
        for (let i = 0; i < n; i++) {
          const m = str();
          const name = str();
          const kind = bytes[p++];
          if (kind === 0) {
            const t = types[uleb()];
            if (t) out.set(`${m}.${name}`, t);
          } else if (kind === 1 || kind === 3) {
            p++;
            if (kind === 3) p++;
          } else if (kind === 2) {
            p++;
            if (bytes[p - 1] & 1) p++;
          }
        }
      }
      p = end;
    }
    return out;
  }
  var VALTYPE_BYTES = { i32: 127, i64: 126, f32: 125, f64: 124, v128: 123, funcref: 112, externref: 111 };
  var DEFAULT_CALL_TYPE = { params: ["i32", "i32", "i32", "i32"], results: ["i32"] };
  function ulebBytes(v) {
    const out = [];
    do {
      let c = v % 128;
      v = Math.floor(v / 128);
      if (v) c |= 128;
      out.push(c);
    } while (v);
    return out;
  }
  function patchSharedMemoryImport(bytes) {
    const readUleb = (p2) => {
      let v = 0, s = 0;
      for (; ; ) {
        const c = bytes[p2++];
        v += (c & 127) * 2 ** s;
        if (!(c & 128)) return [v, p2];
        s += 7;
      }
    };
    let p = 8;
    while (p < bytes.length) {
      const id = bytes[p];
      const [size, bodyStart] = readUleb(p + 1);
      if (id !== 2) {
        p = bodyStart + size;
        continue;
      }
      let q = bodyStart;
      const [count, afterCount] = readUleb(q);
      q = afterCount;
      for (let i = 0; i < count; i++) {
        const [mlen, mp] = readUleb(q);
        const mod = textDecoder2.decode(bytes.subarray(mp, mp + mlen));
        const [nlen, np] = readUleb(mp + mlen);
        const name = textDecoder2.decode(bytes.subarray(np, np + nlen));
        const kindAt = np + nlen;
        const kind = bytes[kindAt];
        if (kind === 2 && name === "memory") {
          const [flags, minAt] = readUleb(kindAt + 1);
          if (flags & 2) return bytes;
          const [min, afterMin] = readUleb(minAt);
          let entryEnd = afterMin;
          if (flags & 1) entryEnd = readUleb(afterMin)[1];
          const newEntry = [
            ...ulebBytes(mod.length),
            ...textEncoder2.encode(mod),
            ...ulebBytes(name.length),
            ...textEncoder2.encode(name),
            2,
            3,
            ...ulebBytes(min),
            ...ulebBytes(65536)
          ];
          const sectionBody = [
            ...bytes.subarray(bodyStart, q),
            ...newEntry,
            ...bytes.subarray(entryEnd, bodyStart + size)
          ];
          return new Uint8Array([
            ...bytes.subarray(0, p),
            2,
            ...ulebBytes(sectionBody.length),
            ...sectionBody,
            ...bytes.subarray(bodyStart + size)
          ]);
        }
        let e = kindAt + 1;
        if (kind === 0) {
          e = readUleb(e)[1];
        } else if (kind === 1) {
          e += 1;
          const [f, m1] = readUleb(e);
          e = readUleb(m1)[1];
          if (f & 1) e = readUleb(e)[1];
        } else if (kind === 2) {
          const [f, m1] = readUleb(e);
          e = readUleb(m1)[1];
          if (f & 1) e = readUleb(e)[1];
        } else if (kind === 3) {
          e += 2;
        }
        q = e;
      }
      return bytes;
    }
    return bytes;
  }
  function makeWrapperFunction(jsFn, type) {
    const section = (id, body) => [id, ...ulebBytes(body.length), ...body];
    const vec = (items) => [...ulebBytes(items.length), ...items.flat()];
    const typeBody = [
      96,
      ...vec(type.params.map((t) => [VALTYPE_BYTES[t] ?? 127])),
      ...vec(type.results.map((t) => [VALTYPE_BYTES[t] ?? 127]))
    ];
    const name = (s) => [...ulebBytes(s.length), ...[...s].map((c) => c.charCodeAt(0))];
    const importBody = [...name("t"), ...name("f"), 0, 0];
    const codeBody = [
      0,
      // no locals
      ...type.params.flatMap((_, i) => [32, ...ulebBytes(i)]),
      16,
      0,
      // call 0
      11
    ];
    const bytes = new Uint8Array([
      0,
      97,
      115,
      109,
      1,
      0,
      0,
      0,
      ...section(1, vec([typeBody])),
      ...section(2, vec([importBody])),
      ...section(3, vec([[0]])),
      ...section(7, vec([[...name("w"), 0, 1]])),
      ...section(10, vec([[...ulebBytes(codeBody.length), ...codeBody]]))
    ]);
    const inst = new WebAssembly.Instance(new WebAssembly.Module(bytes), { t: { f: jsFn } });
    return inst.exports.w;
  }
  var NULL_STUB_FN = (() => {
    const bytes = new Uint8Array([
      0,
      97,
      115,
      109,
      1,
      0,
      0,
      0,
      1,
      4,
      1,
      96,
      0,
      0,
      3,
      2,
      1,
      0,
      7,
      5,
      1,
      1,
      116,
      0,
      0,
      10,
      5,
      1,
      3,
      0,
      0,
      11
    ]);
    return new WebAssembly.Instance(new WebAssembly.Module(bytes), {}).exports.t;
  })();
  function probeRealTier() {
    if (typeof SharedArrayBuffer !== "function") {
      return { ok: false, reason: "SharedArrayBuffer is not available in this context" };
    }
    try {
      new WebAssembly.Memory({ initial: 1, maximum: 65536, shared: true });
    } catch (err) {
      return {
        ok: false,
        reason: `a shared WebAssembly.Memory cannot be constructed (${err.message})`
      };
    }
    if (typeof Worker !== "function") {
      return { ok: false, reason: "the Worker constructor is not available in this context" };
    }
    return { ok: true };
  }
  var CB_LOAD_SEQ = 0;
  var CB_ACKS_DONE = 4;
  var CB_LOAD_DONE = 8;
  var CB_TASK_ADDR = 12;
  var CB_CALL_SEQ = 16;
  var CB_CALL_TASK_ADDR = 20;
  var CB_STDIN_ADDR = 32;
  var CB_STDIN_LEN = 36;
  var CB_LOAD_FAILED = 40;
  var TR_ACK = 8;
  var TR_DETACHED = 12;
  var DIR_BLOCK_TIDS = 8192;
  var DIR_BLOCKS = 4096;
  var PX_STATUS = 8;
  var PX_NARGS = 12;
  var PX_RESTYPE = 16;
  var PX_ARGTYPES = 20;
  var PX_NAMELEN = 36;
  var PX_NAME = 40;
  var PX_ERRLEN = 168;
  var PX_ERR = 172;
  var PX_ARGS = 688;
  var PX_RESULT = 816;
  var PX_ERR_MAX = 512;
  var PX_DONE = 2;
  var PX_OK = 0;
  var PX_ERROR = 1;
  var REGION_CLASSES2 = 64;
  var REGION_TABLE_SIZE2 = 16 + REGION_CLASSES2 * 16;
  var REGION_ZERO_MAX = 4096;
  var ALLOC_LOCK_WORD = 520;
  var SIM_BLOCKING_SHIMS = /* @__PURE__ */ new Set([
    "pthread_join",
    "pthread_mutex_lock",
    "pthread_mutex_timedlock",
    "pthread_cond_wait",
    "pthread_cond_timedwait",
    "pthread_once",
    "pthread_spin_lock",
    "sched_yield"
  ]);
  var TinycProcess = class {
    constructor(options = {}) {
      this._tinyc = options.tinyc ?? null;
      this._readModule = typeof options.readModule === "function" ? options.readModule : null;
      this._threaded = options.threads === true;
      this._jspi = typeof WebAssembly.promising === "function" && typeof WebAssembly.Suspending === "function";
      this.threadTier = null;
      this._scheduler = null;
      if (this._threaded) {
        const probe = probeRealTier();
        const forced = options.threadTier ?? null;
        if (forced === "real" && !probe.ok) {
          throw new TinycDylinkError(
            `tinyc: cannot create a real-tier threaded process: ${probe.reason}`
          );
        }
        if (forced !== null && forced !== "real" && forced !== "simulated") {
          throw new TinycDylinkError(
            `tinyc: threadTier must be "real" or "simulated", got "${forced}"`
          );
        }
        this.threadTier = forced ?? (probe.ok ? "real" : "simulated");
      }
      this._baseUrl = options.baseUrl ?? null;
      if (this._baseUrl !== null && !/^[a-zA-Z][a-zA-Z0-9+.-]*:/.test(this._baseUrl)) {
        const contextBase = SCRIPT_DIR_HREF ?? MODULE_URL ?? PAGE_DIR_HREF;
        if (contextBase) {
          try {
            this._baseUrl = new URL(dirHref(this._baseUrl), contextBase).href;
          } catch {
          }
        }
      }
      this._transport = createTransport(options.transport);
      this._args = (options.args ?? []).map(String);
      this.memory = this.threadTier === "real" ? new WebAssembly.Memory({ initial: 1, maximum: 65536, shared: true }) : new WebAssembly.Memory({ initial: 1 });
      this.table = new WebAssembly.Table({ element: "anyfunc", initial: 2 });
      this.table.set(1, NULL_STUB_FN);
      this._tableFree = 2;
      this._holder = { memory: this.memory, instance: null };
      this._io = options.io ? normalizeIO(options.io) : null;
      this._byteIO = this._io ? textIOToByteIO(this._io) : createDefaultByteIO();
      this._wasi = createWasiShim(
        { args: this._args, env: options.env ?? {}, stdin: options.stdin },
        this._holder,
        this._byteIO
      );
      const def = createDefaultShims({
        io: this._byteIO,
        holder: this._holder,
        stdin: options.stdin,
        heapBase: 1024,
        memoryAllocator: true,
        processTable: this.table,
        // A process attached to an instance shares the instance's
        // virtual FS as its stdio file space: guest fopen reads and
        // writes land in the same files the host stages and reads
        // back. A standalone process gets the shim layer's private
        // in-memory file space instead.
        fileSystem: this._tinyc ? {
          read: (p) => {
            try {
              return this._tinyc.fs.hasFile(p) ? this._tinyc.fs.readFile(p) : null;
            } catch {
              return null;
            }
          },
          write: (p, bytes) => {
            this._tinyc.fs.addFileSync(p, bytes);
          },
          remove: (p) => this._tinyc.fs.removeFile(p)
        } : void 0
      });
      this._defaultEnv = def.env;
      this._allocatorState = def.allocatorState;
      this._reserveStatic = def.reserveStatic;
      this._shimsOption = options.shims ?? null;
      this._importsOption = options.imports ?? null;
      this._hostProvidedFns = /* @__PURE__ */ new Map();
      this._regionTableAddr = 0;
      this._modules = /* @__PURE__ */ new Map();
      this._handles = /* @__PURE__ */ new Map();
      this._nextHandle = 1;
      this._globalSymbols = /* @__PURE__ */ new Map();
      this._got = /* @__PURE__ */ new Map();
      this._stubSlots = /* @__PURE__ */ new Map();
      this._shimSlots = /* @__PURE__ */ new Map();
      this._wrapperSlots = /* @__PURE__ */ new Map();
      this._libraries = /* @__PURE__ */ new Map();
      this._libraryErrors = /* @__PURE__ */ new Map();
      this._loadOrder = [];
      this._main = null;
      this._dlerror = null;
      this._ended = false;
      if (this._threaded) {
        this._writeI32(HDR_TID_COUNTER, 1);
        if (this.threadTier === "simulated") {
          this._scheduler = new Scheduler(this);
          this._scheduler.createMainThread();
          this._pthreadEnv = createSimPthreadShims(this, this._scheduler);
        } else {
          this._pthreadEnv = createCoordinatorPthreadShims();
          this._workerUrlOption = options.workerUrl ?? null;
          this._stdinOption = options.stdin ?? null;
          this._real = {
            workers: /* @__PURE__ */ new Map(),
            // tid -> { worker, recordAddr, alive, isMain }
            mainRun: null,
            // { resolve } while a run is driven
            mainExited: false,
            // main thread pthread_exited
            loadsInFlight: 0,
            loadQueue: [],
            // pending load requests (serialized)
            spawnQueue: [],
            // spawns gated on a load (§5.4)
            nextLoadSeq: 1,
            lastTaskIndex: 0,
            // _loadOrder length at the last task
            callWaiters: /* @__PURE__ */ new Map(),
            // call seq -> { resolve, reject }
            envMap: options.env ?? {},
            nextCallSeq: 1,
            control: 0,
            // control block address (0 until setup)
            keyTableAddr: 0,
            dirAddr: 0
            // thread record directory
          };
          this._slotDesc = /* @__PURE__ */ new Map();
        }
      }
    }
    /** The process's main module record, or null. */
    get mainModule() {
      return this._main;
    }
    /** The transport kind in use: "fetch" or "script". */
    get transportKind() {
      return this._transport.kind;
    }
    // -- memory / table frontiers -------------------------------------
    _ensureMemory(end) {
      const have = this.memory.buffer.byteLength;
      if (have < end) this.memory.grow(Math.ceil((end - have) / 65536));
    }
    _allocTable(n) {
      const base = this._tableFree;
      if (this.table.length < base + n) this.table.grow(base + n - this.table.length);
      this._tableFree = base + n;
      return base;
    }
    _allocTableSlot(fn) {
      const slot = this._allocTable(1);
      this.table.set(slot, fn);
      return slot;
    }
    _reserveForModule(meta, isMain) {
      if (isMain) {
        this._ensureMemory(meta.memSize);
        if (this._allocatorState.heapTop < meta.memSize) {
          this._allocatorState.heapTop = meta.memSize;
        }
        return 0;
      }
      return this._reserveStatic(meta.memSize, meta.memAlign || 16);
    }
    // -- memory words ---------------------------------------------------
    _readI32(addr) {
      return new DataView(this.memory.buffer).getInt32(addr, true);
    }
    _writeI32(addr, value) {
      new DataView(this.memory.buffer).setInt32(addr, value | 0, true);
    }
    // -- per-(thread, module) state (threads design §3.1, §7.2) ---------
    /** The thread's TLS area for a module, allocated and
     *  initialized (template bytes, zeroed tail) on first use —
     *  the later of thread creation and module load is when the
     *  callers invoke this. Areas come from the thread-region
     *  allocator (§3.1) and return to it when the thread is
     *  reaped. */
    _tlsAreaFor(tid, record) {
      let addr = record.tlsAreas.get(tid);
      if (addr === void 0) {
        addr = this._regionAlloc(record.meta.tlsSize, record.meta.tlsAlign || 4);
        const m = new Uint8Array(this.memory.buffer);
        m.fill(0, addr, addr + record.meta.tlsSize);
        if (record.meta.tlsTemplate) {
          m.set(
            record.meta.tlsTemplate.subarray(
              0,
              Math.min(record.meta.tlsTemplate.length, record.meta.tlsSize)
            ),
            addr
          );
        }
        record.tlsAreas.set(tid, addr);
      }
      return addr;
    }
    /** A created thread's stack region for a module (§3.1): the
     *  main thread uses the module's baked span and has no
     *  region. Regions come from the thread-region allocator
     *  and return to it when the thread is reaped. */
    _stackRegionFor(tid, record) {
      let base = record.stackRegions.get(tid);
      if (base === void 0) {
        base = this._regionAlloc(record.meta.stackSize, 16);
        record.stackRegions.set(tid, base);
      }
      return base;
    }
    /** Point every GOT.tls global at the given thread's areas:
     *  (the defining module's area for the thread) + (the
     *  symbol's offset in the definer's span). */
    _refreshGotTls(tid) {
      for (const [key, e] of this._got) {
        if (!key.startsWith("tls:") || !e.definer) continue;
        e.global.value = this._tlsAreaFor(tid, e.definer) + e.offset;
      }
    }
    /** The address a symbol-table entry names for the given
     *  thread: a TLS variable lives in the definer's per-thread
     *  area at its span offset; anything else is its recorded
     *  value. */
    _symbolAddress(entry, tid) {
      if (entry.tls) {
        const area = entry.record.tlsAreas.get(tid);
        return (area ?? 0) + entry.value;
      }
      return entry.value;
    }
    /** The thread id a host-side lookup acts for: the scheduler's
     *  context owner in a threaded process, else the single
     *  thread. */
    _actingTid() {
      return this._scheduler?.contextOwner?.id ?? 1;
    }
    /** Make a thread's execution state current (§4.3): for every
     *  loaded module, swap the instance's stack pointer to the
     *  thread's saved value (capturing the outgoing thread's),
     *  install its TLS base, and refresh the GOT.tls globals. */
    _installThreadContext(thread) {
      const sched = this._scheduler;
      if (!sched || sched.contextOwner === thread) return;
      const prev = sched.contextOwner;
      for (const record of this._loadOrder) {
        const inst = record.instance;
        if (!inst) continue;
        const swap = inst.exports.__tinyc_dylink_swap_sp;
        if (typeof swap === "function") {
          let incoming = thread.savedSp.get(record);
          if (incoming === void 0 && record.stackRegions.has(thread.id)) {
            incoming = record.stackRegions.get(thread.id) + record.meta.stackSize;
          }
          if (incoming !== void 0) {
            const prevSp = swap(incoming);
            if (prev && prev !== thread) prev.savedSp.set(record, prevSp);
          }
        }
        if (record.tlsBaseGlobal) {
          const area = record.tlsAreas.get(thread.id);
          if (area !== void 0) record.tlsBaseGlobal.value = area;
        }
      }
      this._refreshGotTls(thread.id);
      sched.contextOwner = thread;
    }
    /** Allocate a created thread's per-module state and queue it
     *  for the rotation (simulated tier). The thread's entry is
     *  the start routine reached through the process table. */
    _createThread(startSlot, arg, detached) {
      const sched = this._scheduler;
      const id = this._readI32(HDR_TID_COUNTER) + 1;
      this._writeI32(HDR_TID_COUNTER, id);
      const thread = new ThreadRecord(id);
      thread.detached = detached;
      thread.keyValuesAddr = this._regionAlloc(128 * 4, 4);
      for (const record of this._loadOrder) {
        if (record.meta.stackSize !== void 0 && id !== 1) {
          this._stackRegionFor(id, record);
        }
        if (record.meta.tlsSize > 0) this._tlsAreaFor(id, record);
      }
      thread.invoke = () => {
        const fn = this.table.get(startSlot);
        if (typeof fn !== "function") {
          throw new TinycDylinkError(
            `tinyc: pthread_create: table slot ${startSlot} does not hold a function`
          );
        }
        if (this._jspi) return WebAssembly.promising(fn)(arg);
        return Promise.resolve(fn(arg));
      };
      sched.threads.set(id, thread);
      sched.runnable.push(thread);
      sched.kick();
      sched._nudge();
      return id;
    }
    // -- dlerror --------------------------------------------------------
    _setDlerror(message) {
      this._dlerror = String(message);
      const bytes = textEncoder2.encode(this._dlerror.slice(0, 253));
      const m = new Uint8Array(this.memory.buffer);
      m.set(bytes, 256);
      m[256 + bytes.length] = 0;
    }
    _takeDlerrorPtr() {
      if (this._dlerror == null) return 0;
      this._dlerror = null;
      return 256;
    }
    _readCString(ptr, max = 1 << 16) {
      const m = new Uint8Array(this.memory.buffer);
      let end = ptr;
      while (end < m.length && end - ptr < max && m[end] !== 0) end++;
      return textDecoder2.decode(m.slice(ptr, end));
    }
    // -- library resolution (design §13) --------------------------------
    /** A library from local sources only — the resolved cache, the
     *  script registry, the attached instance's virtual FS, the
     *  readModule callback. A local file that fails its integrity
     *  check counts as absent; the reason is remembered for error
     *  messages. Returns null when nothing usable is local. */
    _libraryLocal(canonical) {
      const cached = this._libraries.get(canonical);
      if (cached) return cached;
      let lib = null;
      const reg = globalThis[DYLINK_REGISTRY];
      if (reg && reg[canonical] !== void 0) {
        try {
          lib = manifestToLibrary(reg[canonical], canonical);
          lib.manifest = reg[canonical];
        } catch (err) {
          this._libraryErrors.set(canonical, err.message);
        }
      }
      if (!lib && this._tinyc?.fs.hasFile(canonical)) {
        let bytes = null;
        try {
          bytes = this._tinyc.fs.readFile(canonical);
        } catch {
          bytes = null;
        }
        if (bytes) {
          try {
            lib = libraryFromBytes(canonical, bytes);
          } catch (err) {
            this._libraryErrors.set(canonical, err.message);
          }
        }
      }
      if (!lib && this._readModule) {
        const bytes = this._readModule(canonical);
        if (bytes) {
          try {
            lib = libraryFromBytes(canonical, bytes);
          } catch (err) {
            this._libraryErrors.set(canonical, err.message);
          }
        }
      }
      if (lib) {
        this._libraries.set(canonical, lib);
        this._libraryErrors.delete(canonical);
      }
      return lib;
    }
    /** Whether a library and its whole needed closure are
     *  available from local sources — the guest dlopen fast path
     *  in a threaded process (§9.1). */
    _closureLocal(canonical, seen = /* @__PURE__ */ new Set()) {
      if (seen.has(canonical)) return true;
      seen.add(canonical);
      const lib = this._libraryLocal(canonical);
      if (!lib) return false;
      return lib.meta.needed.every((n) => this._closureLocal(canonicalModulePath(n, vfsDirname(canonical)), seen));
    }
    /** A library, retrieving its manifest through the transport when
     *  no usable copy is local; a retrieval is written back into the
     *  virtual FS, best-effort. */
    async _libraryFetched(canonical) {
      const local = this._libraryLocal(canonical);
      if (local) return local;
      const origin = canonical.startsWith("/sysroot/") ? canonical.slice("/sysroot/".length) : canonical.replace(/^\//, "");
      const url = assetUrl(origin, this._baseUrl ?? void 0);
      let manifest;
      try {
        manifest = await this._transport.retrieveManifest(url, canonical);
      } catch (err) {
        const why = this._libraryErrors.get(canonical);
        if (why) {
          throw new TinycDylinkError(`${err.message} (the local copy is unusable: ${why})`);
        }
        throw err;
      }
      const lib = manifestToLibrary(manifest, canonical);
      lib.manifest = manifest;
      this._libraries.set(canonical, lib);
      this._libraryErrors.delete(canonical);
      if (this._tinyc) {
        try {
          this._tinyc.fs.addFileSync(
            canonical,
            manifestEnvelopeText(canonical, manifest)
          );
        } catch {
        }
      }
      return lib;
    }
    /** Resolve the whole needed closure of a root library through
     *  _libraryFetched, so the synchronous load below never waits. */
    async _prefetchClosure(rootCanonical) {
      const seen = /* @__PURE__ */ new Set();
      const walk = async (canonical) => {
        if (seen.has(canonical)) return;
        seen.add(canonical);
        const lib = await this._libraryFetched(canonical);
        for (const need of lib.meta.needed) {
          await walk(canonicalModulePath(need, vfsDirname(canonical)));
        }
      };
      await walk(rootCanonical);
    }
    // -- symbols --------------------------------------------------------
    _gotEntry(kind, name) {
      const key = `${kind}:${name}`;
      let e = this._got.get(key);
      if (!e) {
        e = {
          global: new WebAssembly.Global({ value: "i32", mutable: true }, 0),
          filled: false,
          definer: null
        };
        if (kind === "func") e.global.value = this._stubSlot(name);
        this._got.set(key, e);
      }
      return e;
    }
    /** A table slot holding a wasm wrapper of a JS function (the
     *  table accepts only wasm functions), typed for the calls the
     *  process's modules make through it. */
    _jsFunctionSlot(fn, type) {
      const typeKey = `${type.params.join(",")}->${type.results.join(",")}`;
      let byType = this._wrapperSlots.get(fn);
      if (!byType) {
        byType = /* @__PURE__ */ new Map();
        this._wrapperSlots.set(fn, byType);
      }
      let slot = byType.get(typeKey);
      if (slot === void 0) {
        slot = this._allocTableSlot(makeWrapperFunction(fn, type));
        byType.set(typeKey, slot);
      }
      return slot;
    }
    _stubSlot(name) {
      let slot = this._stubSlots.get(name);
      if (slot === void 0) {
        const trap = () => {
          throw new Error(`tinyc: call to unresolved function "${name}"`);
        };
        slot = this._jsFunctionSlot(trap, DEFAULT_CALL_TYPE);
        this._stubSlots.set(name, slot);
        if (this._slotDesc) {
          this._slotDesc.set(slot, {
            kind: "stub",
            name,
            params: [...DEFAULT_CALL_TYPE.params],
            results: [...DEFAULT_CALL_TYPE.results]
          });
        }
      }
      return slot;
    }
    _shimSlot(name, fn, requester = null, isHost = false) {
      let slot = this._shimSlots.get(name);
      if (slot === void 0) {
        let type = requester?.importTypes?.get(`env.${name}`) ?? null;
        if (!type) {
          for (const rec of this._modules.values()) {
            type = rec.importTypes?.get(`env.${name}`) ?? null;
            if (type) break;
          }
        }
        const t = type ?? DEFAULT_CALL_TYPE;
        slot = this._jsFunctionSlot(fn, t);
        this._shimSlots.set(name, slot);
        if (this._slotDesc) {
          this._slotDesc.set(slot, {
            kind: isHost ? "host" : "shim",
            name,
            params: [...t.params],
            results: [...t.results]
          });
        }
      }
      return slot;
    }
    /** Fill a GOT entry from a definition, first-wins: an entry that
     *  already holds a definition keeps it. */
    _fillGot(kind, name, value, definer) {
      const e = this._gotEntry(kind, name);
      if (!e.filled) {
        e.global.value = value;
        e.filled = true;
        e.definer = definer;
      }
    }
    _registerGlobals(record) {
      for (const [name, e] of record.exports) {
        if (!this._globalSymbols.has(name)) {
          this._globalSymbols.set(name, {
            kind: e.kind,
            value: e.value,
            record,
            tls: e.tls ? 1 : 0
          });
        }
      }
      record.global = true;
    }
    _withdrawGlobals(record) {
      for (const [name, e] of this._globalSymbols) {
        if (e.record === record) this._globalSymbols.delete(name);
      }
      for (const [name, e] of record.exports) {
        const entry = this._got.get(
          `${e.tls ? "tls" : e.kind === 0 ? "func" : "mem"}:${name}`
        );
        if (entry && entry.definer === record) {
          entry.global.value = e.kind === 0 && !e.tls ? this._stubSlot(name) : 0;
          entry.filled = false;
          entry.definer = null;
        }
      }
      record.global = false;
    }
    /** Modules a requester has opened and not yet closed. */
    _openedBy(requester) {
      const out = [];
      for (const rec of this._modules.values()) {
        if ((rec.handleHolders.get(requester) ?? 0) > 0) out.push(rec);
      }
      return out;
    }
    /** A record's visibility closure (design §9.4): itself, its
     *  needed closure transitively, and — for a module loaded by a
     *  runtime dlopen — its requester's exports, the requester's
     *  closure and the modules the requester has open. */
    _closureRecords(record) {
      const out = [];
      const seen = /* @__PURE__ */ new Set();
      const visit = (rec) => {
        if (!rec || seen.has(rec)) return;
        seen.add(rec);
        out.push(rec);
        for (const dep of rec.neededRecords) visit(dep);
        if (rec.scopeFrom) {
          visit(rec.scopeFrom);
          for (const dep of this._openedBy(rec.scopeFrom)) visit(dep);
        }
      };
      visit(record);
      return out;
    }
    _lookupClosure(name, record, includeSelf) {
      const records = this._closureRecords(record);
      for (const rec of records) {
        if (!includeSelf && rec === record) continue;
        const e = rec.exports.get(name);
        if (e) return { record: rec, entry: e };
      }
      return null;
    }
    /** The callable for a function symbol provided by a module:
     *  the exported function itself when the definer is
     *  instantiated, a call-time forwarding stub while it is still
     *  in flight (a dependency cycle). */
    _functionValue(record, name, requester) {
      const fn = record.instance?.exports?.[name];
      if (typeof fn === "function") return fn;
      return (...args) => {
        const f = record.instance?.exports?.[name];
        if (typeof f !== "function") {
          throw new Error(
            `tinyc: call to "${name}" of "${record.canonical}", which is not instantiated yet (imported by "${requester.canonical}")`
          );
        }
        return f(...args);
      };
    }
    // -- import assembly --------------------------------------------------
    _buildImports(record, wasmModule) {
      const imports = {};
      const provided = /* @__PURE__ */ new Map();
      const put = (m, n, v) => {
        (imports[m] ??= {})[n] = v;
        provided.set(`${m}.${n}`, v);
      };
      for (const [name, fn] of Object.entries(this._wasi)) {
        put("wasi_snapshot_preview1", name, fn);
      }
      if (this._pthreadEnv && typeof this._pthreadEnv.sched_yield === "function") {
        put(
          "wasi_snapshot_preview1",
          "sched_yield",
          SIM_BLOCKING_SHIMS.has("sched_yield") && this._scheduler && this._jspi ? new WebAssembly.Suspending(this._pthreadEnv.sched_yield) : this._pthreadEnv.sched_yield
        );
      }
      for (const [name, fn] of Object.entries(this._defaultEnv)) {
        put("env", name, fn);
      }
      const dl = this._dlfcnFor(record);
      for (const [name, fn] of Object.entries(dl)) {
        if (name === "dlopen" && this._scheduler && this._jspi) {
          put("env", name, new WebAssembly.Suspending(fn));
        } else {
          put("env", name, fn);
        }
      }
      if (this._pthreadEnv) {
        for (const [name, fn] of Object.entries(this._pthreadEnv)) {
          put("env", name, SIM_BLOCKING_SHIMS.has(name) && this._scheduler && this._jspi ? new WebAssembly.Suspending(fn) : fn);
        }
      }
      const ctx = createShimContext(this._holder, this._byteIO);
      const hostFns = /* @__PURE__ */ new Map();
      for (const layer of [globalShimRegistry, this._tinyc?._shims, this._shimsOption]) {
        if (!layer) continue;
        for (const [key, fn] of shimEntriesOf(layer)) {
          const { module: mn, name } = splitShimKey(key);
          const bound = (...a) => fn(ctx, ...a);
          put(mn, name, bound);
          hostFns.set(`${mn}.${name}`, bound);
        }
      }
      for (const [mn, fns] of Object.entries(this._importsOption ?? {})) {
        for (const [name, v] of Object.entries(fns)) {
          put(mn, name, v);
          if (typeof v === "function") hostFns.set(`${mn}.${name}`, v);
          else hostFns.delete(`${mn}.${name}`);
        }
      }
      this._hostProvidedFns = hostFns;
      put("env", "memory", this.memory);
      put("env", "__indirect_function_table", this.table);
      put(
        "env",
        "__memory_base",
        new WebAssembly.Global({ value: "i32", mutable: false }, record.memoryBase)
      );
      put(
        "env",
        "__table_base",
        new WebAssembly.Global({ value: "i32", mutable: false }, record.tableBase)
      );
      const moduleImports = WebAssembly.Module.imports(wasmModule);
      if (moduleImports.some((i) => i.module === "env" && i.name === "__tls_base" && i.kind === "global")) {
        const g = new WebAssembly.Global(
          { value: "i32", mutable: true },
          record.meta.tlsSize > 0 ? this._tlsAreaFor(this._actingTid(), record) : 0
        );
        record.tlsBaseGlobal = g;
        put("env", "__tls_base", g);
      }
      const unresolvedFunctions = [];
      const unresolvedGotMem = [];
      const unresolvedGotTls = [];
      const unresolvedOther = [];
      for (const imp of moduleImports) {
        const key = `${imp.module}.${imp.name}`;
        if (imp.kind === "global" && (imp.module === "GOT.func" || imp.module === "GOT.mem" || imp.module === "GOT.tls")) {
          const kind = imp.module === "GOT.func" ? "func" : imp.module === "GOT.mem" ? "mem" : "tls";
          const entry = this._gotEntry(kind, imp.name);
          if (!entry.filled) {
            if (kind === "tls") {
              const hit = this._resolveGotTls(imp.name, record);
              if (hit) this._fillGotTls(imp.name, hit.definer, hit.offset);
              else unresolvedGotTls.push(imp.name);
            } else {
              const hit = this._resolveGot(kind, imp.name, record, provided, hostFns);
              if (hit) this._fillGot(kind, imp.name, hit.value, hit.definer);
              else if (kind === "mem") unresolvedGotMem.push(imp.name);
            }
          }
          put(imp.module, imp.name, entry.global);
          continue;
        }
        if (provided.has(key)) continue;
        if (imp.kind === "function") {
          const g = this._globalSymbols.get(imp.name);
          if (g && g.kind === 0) {
            put(imp.module, imp.name, this._functionValue(g.record, imp.name, record));
            continue;
          }
          const hit = this._lookupClosure(imp.name, record, false);
          if (hit && hit.entry.kind === 0) {
            put(imp.module, imp.name, this._functionValue(hit.record, imp.name, record));
            continue;
          }
          unresolvedFunctions.push(imp.name);
          put(imp.module, imp.name, () => {
            throw new Error(
              `tinyc: call to unresolved function "${imp.name}" imported by "${record.canonical}"`
            );
          });
          continue;
        }
        unresolvedOther.push(key);
      }
      const unresolvedDataGotFunc = [];
      for (const name of record.meta.dataGotFunc ?? []) {
        const entry = this._got.get(`func:${name}`);
        if (!entry || !entry.filled) unresolvedDataGotFunc.push(name);
      }
      return {
        imports,
        unresolvedFunctions,
        unresolvedGotMem,
        unresolvedOther,
        unresolvedDataGotFunc,
        unresolvedGotTls
      };
    }
    /** A GOT value for a not-yet-filled entry, in the §5 order:
     *  shim layer (functions only), global table, requester closure. */
    _resolveGot(kind, name, record, provided, hostFns) {
      const wantKind = kind === "func" ? 0 : 1;
      if (kind === "func") {
        const shim = provided.get(`env.${name}`);
        if (typeof shim === "function") {
          return {
            value: this._shimSlot(
              name,
              shim,
              record,
              hostFns?.has(`env.${name}`) ?? false
            ),
            definer: "shim"
          };
        }
      }
      const g = this._globalSymbols.get(name);
      if (g && g.kind === wantKind) return { value: g.value, definer: g.record };
      const hit = this._lookupClosure(name, record, false);
      if (hit && hit.entry.kind === wantKind) {
        return { value: hit.entry.value, definer: hit.record };
      }
      return null;
    }
    // -- the load procedure (design §9) -----------------------------------
    /** Load (or re-reference) a module by canonical path.
     *  opts: { flags, requester, runtimeEdge, hostEdge, isMain, lib }.
     *  Reference accounting: the call itself is one reference — a
     *  needed edge, a dlopen handle reference, or the process's main
     *  reference. Throws TinycDylinkError on failure, with the
     *  partial load rolled back. */
    _loadModule(canonical, opts = {}) {
      const flags = opts.flags ?? RTLD_LOCAL;
      const requester = opts.requester ?? null;
      const holderKey = opts.runtimeEdge || opts.hostEdge ? requester ?? "host" : null;
      const existing = this._modules.get(canonical);
      if (existing) {
        existing.refcount++;
        if (holderKey !== null) {
          existing.handleHolders.set(
            holderKey,
            (existing.handleHolders.get(holderKey) ?? 0) + 1
          );
        }
        if (flags & RTLD_GLOBAL && !existing.global) {
          this._registerGlobals(existing);
        }
        return existing;
      }
      if (this._ended) {
        throw new TinycDylinkError("tinyc: the process has ended");
      }
      const lib = opts.lib ?? this._libraryLocal(canonical);
      if (!lib) {
        const why = this._libraryErrors.get(canonical);
        throw new TinycDylinkError(why ? `tinyc: cannot load "${canonical}": ${why}` : `tinyc: cannot load "${canonical}": not present locally (a host loadModule call retrieves it from the base URL first)`);
      }
      const meta = lib.meta;
      if (this._threaded && meta.stackSize === void 0) {
        throw new TinycDylinkError(
          `tinyc: cannot load "${canonical}" into a threaded process: the module's metadata carries no stack_size; it predates the threaded module revision (threads design \xA73.3)`
        );
      }
      const record = {
        canonical,
        meta,
        lib,
        handle: this._nextHandle++,
        state: "loading",
        memoryBase: 0,
        tableBase: 0,
        instance: null,
        wasmModule: null,
        exports: /* @__PURE__ */ new Map(),
        neededRecords: [],
        handleHolders: /* @__PURE__ */ new Map(),
        scopeFrom: opts.runtimeEdge ? requester : null,
        refcount: 1,
        global: false,
        isMain: Boolean(opts.isMain),
        tlsAreas: /* @__PURE__ */ new Map(),
        // thread id -> TLS area address
        stackRegions: /* @__PURE__ */ new Map(),
        // thread id -> stack region base
        tlsBaseGlobal: null
        // the env.__tls_base global, if imported
      };
      if (holderKey !== null) record.handleHolders.set(holderKey, 1);
      this._modules.set(canonical, record);
      this._handles.set(record.handle, record);
      try {
        record.memoryBase = this._reserveForModule(meta, record.isMain);
        record.tableBase = this._allocTable(meta.tableSize);
        for (const e of meta.exports) {
          if (record.exports.has(e.name)) continue;
          const value = e.kind === 0 ? record.tableBase + e.value : e.tls ? e.value : record.memoryBase + e.value;
          record.exports.set(e.name, { kind: e.kind, value, tls: e.tls ? 1 : 0 });
        }
        for (const [name, e] of record.exports) {
          if (e.tls) this._fillGotTls(name, record, e.value);
          else this._fillGot(e.kind === 0 ? "func" : "mem", name, e.value, record);
        }
        if (flags & RTLD_GLOBAL || record.isMain) this._registerGlobals(record);
        const childFlags = flags & RTLD_NOW ? RTLD_NOW : RTLD_LAZY;
        for (const need of meta.needed) {
          const depCanonical = canonicalModulePath(need, vfsDirname(canonical));
          const dep = this._loadModule(depCanonical, {
            flags: childFlags,
            requester: record
          });
          record.neededRecords.push(dep);
        }
        const bodyBytes = this.threadTier === "real" ? patchSharedMemoryImport(lib.bodyBytes) : lib.bodyBytes;
        record.bodyBytes = bodyBytes;
        const wasmModule = this.threadTier === "real" ? new WebAssembly.Module(bodyBytes) : lib.wasmModule ?? new WebAssembly.Module(bodyBytes);
        record.wasmModule = wasmModule;
        record.importTypes = parseImportTypes(bodyBytes);
        const {
          imports,
          unresolvedFunctions,
          unresolvedGotMem,
          unresolvedOther,
          unresolvedDataGotFunc,
          unresolvedGotTls
        } = this._buildImports(record, wasmModule);
        if (unresolvedGotMem.length > 0) {
          throw new TinycDylinkError(
            `tinyc: cannot load "${canonical}": unresolved variable(s) ${unresolvedGotMem.map((n) => `"${n}"`).join(", ")} (GOT.mem references bind eagerly)`
          );
        }
        if (unresolvedGotTls.length > 0) {
          throw new TinycDylinkError(
            `tinyc: cannot load "${canonical}": unresolved TLS variable(s) ${unresolvedGotTls.map((n) => `"${n}"`).join(", ")} (GOT.tls references bind eagerly)`
          );
        }
        if (unresolvedDataGotFunc.length > 0) {
          throw new TinycDylinkError(
            `tinyc: cannot load "${canonical}": unresolved function(s) ${unresolvedDataGotFunc.map((n) => `"${n}"`).join(", ")} (GOT.func references from static data bind eagerly)`
          );
        }
        if (flags & RTLD_NOW && unresolvedFunctions.length > 0) {
          throw new TinycDylinkError(
            `tinyc: cannot load "${canonical}": unresolved function(s) ${unresolvedFunctions.map((n) => `"${n}"`).join(", ")} under RTLD_NOW`
          );
        }
        if (unresolvedOther.length > 0) {
          throw new TinycDylinkError(
            `tinyc: cannot load "${canonical}": unsupported import(s) ` + unresolvedOther.join(", ")
          );
        }
        let instance;
        try {
          instance = new WebAssembly.Instance(wasmModule, imports);
        } catch (err) {
          throw new TinycDylinkError(
            `tinyc: cannot instantiate "${canonical}": ${err.message}`
          );
        }
        record.instance = instance;
        if (typeof instance.exports.__tinyc_dylink_init !== "function" || typeof instance.exports.__tinyc_dylink_fini !== "function") {
          throw new TinycDylinkError(
            `tinyc: module "${canonical}" does not export __tinyc_dylink_init/__tinyc_dylink_fini; it is not a loadable dylink module`
          );
        }
        if (this._threaded && typeof instance.exports.__tinyc_dylink_swap_sp !== "function") {
          throw new TinycDylinkError(
            `tinyc: module "${canonical}" does not export __tinyc_dylink_swap_sp; a threaded process requires it (threads design \xA73.1)`
          );
        }
        if (this.threadTier === "real") {
          record.state = "instantiated";
          this._loadOrder.push(record);
          this._afterModuleLoaded(record);
          return record;
        }
        this._initRecord(record);
        this._loadOrder.push(record);
        this._afterModuleLoaded(record);
        return record;
      } catch (err) {
        this._rollbackLoad(record);
        throw err;
      }
    }
    /** Run a record's init on this context's instance — the
     *  single-context path, and the coordinator's replica for a
     *  host-initiated load in the real tier (§5.4). */
    _initRecord(record) {
      record.state = "initializing";
      let rc;
      try {
        rc = record.instance.exports.__tinyc_dylink_init(record.handle);
      } catch (err) {
        throw new TinycDylinkError(
          `tinyc: initialization of "${record.canonical}" trapped: ${err.message}`
        );
      }
      if (rc !== 0) {
        throw new TinycDylinkError(
          `tinyc: initialization of "${record.canonical}" failed: its DllMain returned FALSE`
        );
      }
      record.state = "loaded";
    }
    /** Per-(thread, module) state for a freshly loaded module
     *  (threads design §3.1, §7.2): TLS areas for every live
     *  thread (the module-load side of the later-of-two-events
     *  rule), stack regions for live created threads, and the
     *  __tls_base installation for the current context. In a
     *  non-threaded process the same machinery runs with the
     *  single thread. */
    _afterModuleLoaded(record) {
      const meta = record.meta;
      const tids = this._scheduler ? [...this._scheduler.threads.keys()] : [1];
      if (meta.tlsSize > 0) {
        for (const tid of tids) this._tlsAreaFor(tid, record);
        if (record.tlsBaseGlobal) {
          const area = record.tlsAreas.get(this._actingTid());
          if (area !== void 0) record.tlsBaseGlobal.value = area;
        }
      }
      if (!this._scheduler) return;
      for (const tid of tids) {
        if (tid !== 1 && meta.stackSize !== void 0) {
          this._stackRegionFor(tid, record);
        }
      }
      const owner = this._scheduler.contextOwner;
      if (owner && owner.id !== 1 && meta.stackSize !== void 0 && !owner.savedSp.has(record)) {
        const swap = record.instance?.exports?.__tinyc_dylink_swap_sp;
        if (typeof swap === "function") {
          const top = this._stackRegionFor(owner.id, record) + meta.stackSize;
          const baked = swap(top);
          this._scheduler.mainThread.savedSp.set(record, baked);
          owner.savedSp.set(record, top);
        }
      }
      this._refreshGotTls(this._actingTid());
    }
    /** Fill a GOT.tls entry from a TLS definition, first-wins.
     *  The entry records the definer and the span offset; its
     *  global's value is the acting thread's address and is
     *  refreshed at every context install (§7.2). */
    _fillGotTls(name, definer, offset) {
      const e = this._gotEntry("tls", name);
      if (!e.filled) {
        e.filled = true;
        e.definer = definer;
        e.offset = offset;
        e.global.value = this._tlsAreaFor(this._actingTid(), definer) + offset;
      }
    }
    /** A GOT.tls resolution: the global table, then the
     *  requester's closure — TLS entries only. */
    _resolveGotTls(name, record) {
      const g = this._globalSymbols.get(name);
      if (g && g.tls) return { definer: g.record, offset: g.value };
      const hit = this._lookupClosure(name, record, false);
      if (hit && hit.entry.tls) {
        return { definer: hit.record, offset: hit.entry.value };
      }
      return null;
    }
    /** Undo a failed load: release the references the record took,
     *  withdraw its registrations, discard the record. Its memory
     *  region and table slots are retired, never reused. */
    _rollbackLoad(record) {
      const deps = record.neededRecords;
      record.neededRecords = [];
      for (let i = deps.length - 1; i >= 0; i--) this._releaseRef(deps[i]);
      this._withdrawGlobals(record);
      this._modules.delete(record.canonical);
      this._handles.delete(record.handle);
      record.state = "discarded";
    }
    _releaseRef(record) {
      record.refcount--;
      if (record.refcount > 0 || record.isMain) return;
      this._closeModule(record);
    }
    /** Close a module whose count reached zero (design §8/§9.5):
     *  finalize, withdraw its symbols, then release its own
     *  references — needed edges in reverse order, then the modules
     *  it had opened. The instance and its regions persist. */
    _closeModule(record) {
      if (record.state === "loaded") {
        record.state = "closed";
        try {
          record.instance.exports.__tinyc_dylink_fini(record.handle, 0);
        } catch (err) {
          this._setDlerror(
            `tinyc: finalizer of "${record.canonical}" trapped: ${err.message}`
          );
        }
      }
      this._withdrawGlobals(record);
      this._modules.delete(record.canonical);
      this._handles.delete(record.handle);
      const li = this._loadOrder.indexOf(record);
      if (li >= 0) this._loadOrder.splice(li, 1);
      const deps = record.neededRecords;
      record.neededRecords = [];
      for (let i = deps.length - 1; i >= 0; i--) this._releaseRef(deps[i]);
      for (const dep of this._openedBy(record)) {
        const n = dep.handleHolders.get(record) ?? 0;
        dep.handleHolders.delete(record);
        for (let i = 0; i < n; i++) this._releaseRef(dep);
      }
    }
    // -- dlfcn --------------------------------------------------------------
    _dlsym(handle, name) {
      const tid = this._actingTid();
      if (!handle) {
        const g = this._globalSymbols.get(name);
        if (g) return this._symbolAddress(g, tid);
        this._setDlerror(
          `tinyc: dlsym: symbol "${name}" not found in the global scope`
        );
        return 0;
      }
      const record = this._handles.get(handle);
      if (!record) {
        this._setDlerror(`tinyc: dlsym: unknown handle ${handle}`);
        return 0;
      }
      const own = record.exports.get(name);
      if (own) return this._symbolAddress({ ...own, record }, tid);
      const hit = this._lookupClosure(name, record, false);
      if (hit) return this._symbolAddress({ ...hit.entry, record: hit.record }, tid);
      this._setDlerror(
        `tinyc: dlsym: symbol "${name}" not found in "${record.canonical}" or its dependency closure`
      );
      return 0;
    }
    _consumeHolder(record, requester) {
      const holders = record.handleHolders;
      const key = requester ?? "host";
      if ((holders.get(key) ?? 0) > 0) {
        const n = holders.get(key) - 1;
        if (n === 0) holders.delete(key);
        else holders.set(key, n);
        return true;
      }
      for (const [k, n] of holders) {
        if (n > 0) {
          if (n === 1) holders.delete(k);
          else holders.set(k, n - 1);
          return true;
        }
      }
      return false;
    }
    _dlclose(handle, requester) {
      const record = this._handles.get(handle);
      if (!record) {
        this._setDlerror(`tinyc: dlclose: unknown handle ${handle}`);
        return -1;
      }
      if (record.isMain) {
        this._consumeHolder(record, requester);
        return 0;
      }
      if (!this._consumeHolder(record, requester)) {
        this._setDlerror(
          `tinyc: dlclose: handle ${handle} ("${record.canonical}") has no outstanding dlopen reference`
        );
        return -1;
      }
      this._releaseRef(record);
      return 0;
    }
    /** The dlfcn shim set for one requesting module: its dlopen
     *  calls carry the requester for scoping and edge accounting. */
    _dlfcnFor(requester) {
      const proc = this;
      return {
        dlopen(pathPtr, flags) {
          flags = flags | 0;
          if (pathPtr === 0) {
            if (proc._main) return proc._main.handle;
            proc._setDlerror("tinyc: dlopen(NULL): the process has no main module");
            return 0;
          }
          const bad = flags & ~RTLD_KNOWN;
          if (bad) {
            proc._setDlerror(
              `tinyc: dlopen: unsupported flag bit(s) 0x${(bad >>> 0).toString(16)}`
            );
            return 0;
          }
          const path = proc._readCString(pathPtr);
          let canonical;
          try {
            canonical = canonicalModulePath(path, "/sysroot");
          } catch (err) {
            proc._setDlerror(err.message);
            return 0;
          }
          const load = () => proc._loadModule(canonical, {
            flags,
            requester,
            runtimeEdge: true
          });
          if (proc._scheduler && !proc._closureLocal(canonical)) {
            if (!proc._jspi) {
              throw new Error(
                `tinyc: dlopen: loading "${canonical}" requires a retrieval, which suspends the calling thread; that needs WebAssembly.promising (JSPI), which this engine does not provide (threads design \xA74.2)`
              );
            }
            const thread = proc._scheduler.actingThread();
            return proc._scheduler.parkOnHost(
              thread,
              () => proc._prefetchClosure(canonical)
            ).then(() => {
              try {
                return load().handle;
              } catch (err) {
                proc._setDlerror(err.message);
                return 0;
              }
            });
          }
          try {
            return load().handle;
          } catch (err) {
            proc._setDlerror(err.message);
            return 0;
          }
        },
        dlsym(handle, namePtr) {
          return proc._dlsym(handle | 0, proc._readCString(namePtr));
        },
        dlclose(handle) {
          return proc._dlclose(handle | 0, requester);
        },
        dlerror() {
          return proc._takeDlerrorPtr();
        }
      };
    }
    // -- host surface -------------------------------------------------------
    /** Load a library and its whole needed closure, retrieving
     *  manifests through the transport where no usable copy is
     *  local. Returns the module's handle. */
    async loadModule(path, flags = RTLD_LOCAL) {
      if (this._ended) throw new TinycDylinkError("tinyc: the process has ended");
      const canonical = canonicalModulePath(path, "/sysroot");
      await this._prefetchClosure(canonical);
      if (this._scheduler && this._scheduler.drivingRun && !this._scheduler.hostDriving) {
        return this._scheduler.enqueueHostOp(() => {
          return this._loadModule(canonical, { flags, hostEdge: true }).handle;
        });
      }
      if (this.threadTier === "real") {
        this._realSetup();
        const before = this._loadOrder.length;
        const rec2 = this._loadModule(canonical, { flags, hostEdge: true });
        const delta = this._loadOrder.slice(before);
        if (delta.length > 0) {
          if (this._real.workers.size > 0) {
            await this._realBroadcastLoad(delta, "coordinator", 0);
          } else {
            for (const r of delta) this._initRecord(r);
          }
        }
        return rec2.handle;
      }
      const rec = this._loadModule(canonical, { flags, hostEdge: true });
      return rec.handle;
    }
    /** Synchronous dlopen: the library and its closure must already
     *  be local (see loadModule for the retrieving form). Returns
     *  the handle; throws TinycDylinkError on failure. In a
     *  threaded process the synchronous form does not exist — a
     *  load can run constructors on another context and wait for
     *  replicas — and it fails by name (§2.3). */
    dlopen(path, flags = RTLD_LAZY) {
      if (this._threaded) {
        throw new TinycDylinkError(
          "tinyc: dlopen: a threaded process loads asynchronously; use loadModule (threads design \xA72.3)"
        );
      }
      const canonical = canonicalModulePath(path, "/sysroot");
      return this._loadModule(canonical, { flags, hostEdge: true }).handle;
    }
    /** The handle of an already-loaded module by path, or 0. */
    moduleHandle(path) {
      const rec = this._modules.get(canonicalModulePath(path, "/sysroot"));
      return rec ? rec.handle : 0;
    }
    dlsym(handle, name) {
      const value = this._dlsym(handle, name);
      if (value === 0) {
        const msg = this._dlerror ?? "tinyc: dlsym failed";
        this._dlerror = null;
        throw new TinycDylinkError(msg);
      }
      return value;
    }
    dlclose(handle) {
      const rc = this._dlclose(handle, null);
      if (rc !== 0) {
        const msg = this._dlerror ?? "tinyc: dlclose failed";
        this._dlerror = null;
        throw new TinycDylinkError(msg);
      }
      return 0;
    }
    /** Call an exported function of a loaded module by handle and
     *  name — the host counterpart of a guest function pointer.
     *  In a threaded process the call executes guest code, so it
     *  returns a promise: through the scheduler as a host
     *  operation in the simulated tier, delegated to the main
     *  thread's worker in the real tier (§2.3). */
    callExport(handle, name, ...args) {
      const record = this._handles.get(handle);
      if (!record?.instance) {
        throw new TinycDylinkError(`tinyc: callExport: unknown handle ${handle}`);
      }
      const fn = record.instance.exports[name];
      if (typeof fn !== "function") {
        throw new TinycDylinkError(
          `tinyc: module "${record.canonical}" does not export "${name}"`
        );
      }
      if (this._scheduler) {
        return this._scheduler.enqueueHostOp(async () => {
          if (this._jspi) return WebAssembly.promising(fn)(...args);
          return fn(...args);
        });
      }
      if (this._threaded) return this._realCallExport(record, name, args);
      return fn(...args);
    }
    /** Read a variable exported by a loaded module, as an i32.
     *  A TLS variable reads the acting thread's area (§7.2). */
    readExportI32(handle, name) {
      const record = this._handles.get(handle);
      if (!record) throw new TinycDylinkError(`tinyc: readExportI32: unknown handle ${handle}`);
      const e = record.exports.get(name);
      if (!e || e.kind !== 1) {
        throw new TinycDylinkError(
          `tinyc: module "${record.canonical}" does not export variable "${name}"`
        );
      }
      const addr = this._symbolAddress({ ...e, record }, this._actingTid());
      return new DataView(this.memory.buffer).getInt32(addr, true);
    }
    writeExportI32(handle, name, value) {
      const record = this._handles.get(handle);
      if (!record) throw new TinycDylinkError(`tinyc: writeExportI32: unknown handle ${handle}`);
      const e = record.exports.get(name);
      if (!e || e.kind !== 1) {
        throw new TinycDylinkError(
          `tinyc: module "${record.canonical}" does not export variable "${name}"`
        );
      }
      const addr = this._symbolAddress({ ...e, record }, this._actingTid());
      new DataView(this.memory.buffer).setInt32(addr, value | 0, true);
    }
    /** Load raw module bytes as the process's main module and run
     *  it: the product of a dylink-main compile, whose exports enter
     *  the global symbol table. The entry is options.entry when
     *  given, else _start, else main(argc, argv) with the process's
     *  args. Running the main module ends the process: every loaded
     *  module is finalized in reverse load order. Returns the
     *  entry's return value, or { exitCode } for a _start main.
     *  In a threaded process the result comes back as a promise
     *  for the same shape (§2.3). */
    runMainModule(bytes, options = {}) {
      if (this._threaded) {
        if (this.threadTier === "real") {
          return this._runMainModuleReal(bytes, options);
        }
        return this._runMainModuleSim(bytes, options);
      }
      return this._runMainModuleSync(bytes, options);
    }
    /** The main thread's step in a simulated-tier process: load
     *  the main module and run its entry on a parkable stack
     *  (§4.2), so even load-time constructors may block or
     *  dlopen. Resolves with the runMainModule result shape. */
    _invokeMainThread(bytes, options) {
      if (this._main) {
        throw new TinycDylinkError("tinyc: this process already has a main module");
      }
      if (this._modules.size > 0) {
        throw new TinycDylinkError(
          "tinyc: a main module must be the first module loaded into a process"
        );
      }
      const canonical = canonicalModulePath(options.path ?? "/a.out", "/");
      const lib = libraryFromBytes(canonical, bytes);
      this._libraries.set(canonical, lib);
      const record = this._loadModule(canonical, {
        flags: RTLD_GLOBAL | RTLD_NOW,
        isMain: true,
        lib
      });
      this._main = record;
      this._holder.instance = record.instance;
      const mainThread = this._scheduler?.mainThread;
      if (mainThread && !mainThread.keyValuesAddr) {
        mainThread.keyValuesAddr = this._regionAlloc(128 * 4, 4);
      }
      const exports = record.instance.exports;
      const invoke = (fn, args) => this._jspi ? WebAssembly.promising(fn)(...args) : Promise.resolve(fn(...args));
      if (options.entry !== void 0) {
        const fn = exports[options.entry];
        if (typeof fn !== "function") {
          throw new TinycDylinkError(
            `tinyc: main module does not export "${options.entry}"`
          );
        }
        return invoke(fn, options.callArgs ?? []);
      }
      if (typeof exports._start === "function") {
        return invoke(exports._start, []).then(() => ({ exitCode: 0 }));
      }
      if (typeof exports.main === "function") {
        const { argc, argv } = this._mainArgv(options);
        return invoke(exports.main, [argc, argv]).then((rc) => ({ exitCode: rc | 0 }));
      }
      throw new TinycDylinkError(
        "tinyc: main module exports neither _start nor main"
      );
    }
    /** Drive a simulated-tier run to process end (§2.3's three
     *  rules, classified by the scheduler), then finalize in
     *  reverse load order on the main thread's context. */
    async _runMainModuleSim(bytes, options) {
      const sched = this._scheduler;
      const main = sched.mainThread;
      main.mainEntry = true;
      main.invoke = () => this._invokeMainThread(bytes, options);
      sched.drivingRun = true;
      sched.runnable.push(main);
      const completion = sched.runCompletion();
      sched.kick();
      const end = await completion;
      sched.drivingRun = false;
      sched.contextOwner = null;
      this._installThreadContext(main);
      this._teardown();
      if (this._io) flushIO(this._io);
      switch (end.type) {
        case "exit":
          return { exitCode: end.code };
        case "entry":
          return end.result;
        case "all-exited":
          return { exitCode: 0 };
        case "fault":
          throw end.error;
        case "deadlock":
          throw end.error;
        default:
          throw new TinycDylinkError("tinyc: the run ended abnormally");
      }
    }
    // -- the real tier: coordinator (threads design §5) ------------------
    _workerUrl() {
      if (this._real.workerUrl) return this._real.workerUrl;
      this._real.workerUrl = this._workerUrlOption ?? new URL("./worker.mjs", import_meta.url).href;
      return this._real.workerUrl;
    }
    /** Shared-memory setup for the real tier, done once, lazily:
     *  the control block, the pthread key destructor table
     *  (§7.1), the thread-record directory, and the stdin byte
     *  string with its atomic cursor (§5.6). */
    _realSetup() {
      const R = this._real;
      if (R.control) return;
      R.control = this._reserveStatic(64, 4);
      R.keyTableAddr = this._reserveStatic(128 * 4, 4);
      R.dirAddr = this._reserveStatic(DIR_BLOCKS * 4, 4);
      this._regionTable();
      const stdin = this._stdinOption;
      if (stdin !== null && stdin !== void 0) {
        const bytes = typeof stdin === "string" ? textEncoder2.encode(stdin) : stdin;
        const addr = this._reserveStatic(bytes.length, 4);
        new Uint8Array(this.memory.buffer).set(bytes, addr);
        this._writeI32(R.control + CB_STDIN_ADDR, addr);
        this._writeI32(R.control + CB_STDIN_LEN, bytes.length);
      }
      R.mainRecordAddr = this._allocThreadRecord(1);
    }
    _allocThreadRecord(tid) {
      const addr = this._reserveStatic(32, 4);
      const dv = new DataView(this.memory.buffer);
      for (let i = 0; i < 32; i += 4) dv.setInt32(addr + i, 0, true);
      this._dirSet(tid, addr);
      return addr;
    }
    /** The directory block covering `tid` (§6.6), or 0 when the
     *  tid is past the table's arithmetic cap. With `create`, a
     *  missing block is reserved, zeroed and published by CAS —
     *  the same protocol the workers use (threads.mjs), so a
     *  race between a worker registering its thread and the
     *  coordinator registering the main record has one winner
     *  and the loser discards its block. */
    _dirBlock(tid, create) {
      const idx = tid >> 13;
      if (idx < 0 || idx >= DIR_BLOCKS) return 0;
      const view = new Int32Array(this.memory.buffer);
      const at = this._real.dirAddr / 4 + idx;
      let block = Atomics.load(view, at);
      if (block || !create) return block;
      const fresh = this._reserveStatic(DIR_BLOCK_TIDS * 4, 4);
      new Uint8Array(this.memory.buffer).fill(0, fresh, fresh + DIR_BLOCK_TIDS * 4);
      const prev = Atomics.compareExchange(view, at, 0, fresh);
      return prev === 0 ? fresh : prev;
    }
    _dirSet(tid, recordAddr) {
      const block = this._dirBlock(tid, true);
      if (!block) return;
      Atomics.store(
        new Int32Array(this.memory.buffer),
        block / 4 + (tid & DIR_BLOCK_TIDS - 1),
        recordAddr
      );
    }
    _dirClear(tid) {
      const block = this._dirBlock(tid, false);
      if (!block) return;
      Atomics.store(
        new Int32Array(this.memory.buffer),
        block / 4 + (tid & DIR_BLOCK_TIDS - 1),
        0
      );
    }
    // -- the thread-region allocator (§3.1) ---------------------
    /** The shared class table for thread-region size classes,
     *  reserved once per threaded process. */
    _regionTable() {
      if (!this._regionTableAddr) {
        const addr = this._reserveStatic(REGION_TABLE_SIZE2, 16);
        new Uint8Array(this.memory.buffer).fill(0, addr, addr + REGION_TABLE_SIZE2);
        this._regionTableAddr = addr;
      }
      return this._regionTableAddr;
    }
    _regionLock() {
      const view = new Int32Array(this.memory.buffer);
      while (Atomics.compareExchange(view, ALLOC_LOCK_WORD / 4, 0, 1) !== 0) {
      }
    }
    _regionUnlock() {
      const view = new Int32Array(this.memory.buffer);
      Atomics.store(view, ALLOC_LOCK_WORD / 4, 0);
      Atomics.notify(view, ALLOC_LOCK_WORD / 4, 1);
    }
    /** Find (or create) the class entry for (size, align); the
     *  caller holds the region lock. */
    _regionClass(size, align, create) {
      const table = this._regionTableAddr;
      const dv = new DataView(this.memory.buffer);
      const count = dv.getInt32(table, true);
      for (let i = 0; i < count; i++) {
        const at2 = table + 16 + i * 16;
        if (dv.getInt32(at2, true) === size && dv.getInt32(at2 + 4, true) === align) return at2;
      }
      if (!create || count >= REGION_CLASSES2) return 0;
      const at = table + 16 + count * 16;
      dv.setInt32(at, size, true);
      dv.setInt32(at + 4, align, true);
      dv.setInt32(at + 8, 0, true);
      dv.setInt32(at + 12, 0, true);
      dv.setInt32(table, count + 1, true);
      return at;
    }
    /** Allocate a thread region of an exact size class: a
     *  reaped thread's region of the same class is recycled
     *  before the bump frontier advances. In a non-threaded
     *  process there is nothing to reclaim — the single thread
     *  lives to process end — and the plain reserve serves, so
     *  the 0.4.0 layout is untouched. */
    _regionAlloc(size, align) {
      align = align || 4;
      if (!this._threaded) return this._reserveStatic(size, align);
      this._regionTable();
      let addr = 0;
      this._regionLock();
      try {
        const cls = this._regionClass(size, align, true);
        if (cls) {
          const head = this._readI32(cls + 8);
          if (head) {
            addr = head;
            this._writeI32(cls + 8, this._readI32(head));
          }
        }
      } finally {
        this._regionUnlock();
      }
      if (!addr) return this._reserveStatic(size, align);
      if (size <= REGION_ZERO_MAX) {
        new Uint8Array(this.memory.buffer).fill(0, addr, addr + size);
      }
      return addr;
    }
    /** Return a thread region to its class free list. */
    _regionFree(addr, size, align) {
      if (!addr || !this._threaded) return;
      align = align || 4;
      this._regionTable();
      this._regionLock();
      try {
        const cls = this._regionClass(size, align, true);
        if (cls) {
          this._writeI32(addr, this._readI32(cls + 8));
          this._writeI32(cls + 8, addr);
        }
      } finally {
        this._regionUnlock();
      }
    }
    _bootData(tid, entry, extra = {}) {
      const R = this._real;
      const modules = this._loadOrder.map((record) => ({
        canonical: record.canonical,
        memoryBase: record.memoryBase,
        tableBase: record.tableBase,
        handle: record.handle,
        global: record.global ? 1 : 0,
        manifest: record.lib.manifest,
        bodyBytes: record.bodyBytes ?? record.lib.bodyBytes,
        exports: [...record.exports].map(([name, e]) => [name, { kind: e.kind, value: e.value, tls: e.tls ? 1 : 0 }]),
        needed: [...record.meta.needed]
      }));
      const got = [];
      for (const [key, e] of this._got) {
        if (!e.filled) continue;
        const [kind, name] = [key.slice(0, key.indexOf(":")), key.slice(key.indexOf(":") + 1)];
        got.push(kind === "tls" ? { kind, name, definer: e.definer?.canonical ?? "", offset: e.offset } : { kind, name, value: e.global.value });
      }
      const slots = [...this._slotDesc].map(([slot, d]) => ({ slot, ...d }));
      return {
        type: "tinyc-thread-boot",
        threadId: tid,
        isMain: tid === 1,
        memory: this.memory,
        controlAddr: R.control,
        keyTableAddr: R.keyTableAddr,
        dirAddr: R.dirAddr,
        regionTableAddr: this._regionTableAddr,
        hostImports: [...this._hostProvidedFns.keys()],
        recordAddr: extra.recordAddr ?? 0,
        keyArrayAddr: extra.keyArrayAddr ?? 0,
        args: [...this._args],
        env: { ...R.envMap },
        tableFree: this._tableFree,
        modules,
        got,
        slots,
        entry,
        regions: extra.regions ?? {}
      };
    }
    _spawnWorker(tid, boot, info = {}) {
      const worker = new Worker(this._workerUrl(), { type: "module" });
      const entry = {
        worker,
        recordAddr: info.recordAddr ?? 0,
        alive: true,
        isMain: Boolean(info.isMain)
      };
      this._real.workers.set(tid, entry);
      worker.addEventListener("message", (ev) => this._onWorkerMessage(tid, ev.data));
      worker.addEventListener("error", (ev) => {
        this._settleMainRun({
          type: "fault",
          error: new Error(
            `tinyc: thread ${tid}'s worker failed: ${ev.message ?? "unknown error"}`
          )
        });
      });
      if (entry.recordAddr) {
        this._writeI32(entry.recordAddr + TR_ACK, this._real.nextLoadSeq - 1);
      }
      worker.postMessage(boot);
      return entry;
    }
    _settleMainRun(end) {
      const R = this._real;
      if (!R || !R.mainRun) return;
      const { resolve } = R.mainRun;
      R.mainRun = null;
      resolve(end);
    }
    /** Drive a real-tier run: lay out the main module, boot the
     *  main thread's worker with it, and wait for the process
     *  end the workers report (§2.3). Finalization runs on the
     *  coordinator's replicas (§5.5). */
    async _runMainModuleReal(bytes, options) {
      if (this._main) {
        throw new TinycDylinkError("tinyc: this process already has a main module");
      }
      if (this._modules.size > 0) {
        throw new TinycDylinkError(
          "tinyc: a main module must be the first module loaded into a process"
        );
      }
      const canonical = canonicalModulePath(options.path ?? "/a.out", "/");
      const lib = libraryFromBytes(canonical, bytes);
      this._libraries.set(canonical, lib);
      const record = this._loadModule(canonical, {
        flags: RTLD_GLOBAL | RTLD_NOW,
        isMain: true,
        lib
      });
      this._main = record;
      this._holder.instance = record.instance;
      this._realSetup();
      const regions = {};
      for (const rec of this._loadOrder) {
        regions[rec.canonical] = {
          stackBase: null,
          tlsArea: rec.meta.tlsSize > 0 ? this._tlsAreaFor(1, rec) : 0
        };
      }
      const boot = this._bootData(1, {
        kind: "main",
        canonical,
        runOptions: {
          entry: options.entry ?? null,
          callArgs: options.callArgs ?? []
        }
      }, { recordAddr: this._real.mainRecordAddr, regions });
      const settled = new Promise((resolve) => {
        this._real.mainRun = { resolve };
      });
      this._spawnWorker(1, boot, { isMain: true, recordAddr: this._real.mainRecordAddr });
      const end = await settled;
      this._realTeardown();
      if (this._io) flushIO(this._io);
      switch (end.type) {
        case "exit":
          return { exitCode: end.code };
        case "entry":
          return end.result;
        case "all-exited":
          return { exitCode: 0 };
        case "fault":
          throw end.error;
        default:
          throw new TinycDylinkError("tinyc: the run ended abnormally");
      }
    }
    /** Process end in the real tier is a coordinator act (§5.5):
     *  finalize every module on the coordinator's replica — with
     *  the module's stack top installed through the swap export
     *  first (§5.1) — in reverse load order, then terminate the
     *  remaining workers. */
    _realTeardown() {
      if (this._ended) return;
      this._ended = true;
      for (let i = this._loadOrder.length - 1; i >= 0; i--) {
        const rec = this._loadOrder[i];
        if (rec.state !== "loaded" && rec.state !== "instantiated") continue;
        rec.state = "closed";
        try {
          const swap = rec.instance.exports.__tinyc_dylink_swap_sp;
          if (typeof swap === "function") {
            swap(rec.memoryBase + rec.meta.memSize);
          }
          rec.instance.exports.__tinyc_dylink_fini(rec.handle, 1);
        } catch {
        }
      }
      for (const { worker } of this._real.workers.values()) {
        try {
          worker.terminate();
        } catch {
        }
      }
      this._real.workers.clear();
    }
    /** §5.4: broadcast a load delta to every live worker and
     *  collect acknowledgements. initBy 'coordinator' runs the
     *  inits here once all replicas exist; 'requester' leaves
     *  init to the requesting worker, which signals through the
     *  load-done word. Resolves when the load is complete
     *  process-wide. */
    async _realBroadcastLoad(delta, initBy, requesterTid) {
      const R = this._real;
      const seq = R.nextLoadSeq++;
      const payloadAddr = this._writeLoadTask(seq, delta, initBy, requesterTid);
      const view = new Int32Array(this.memory.buffer);
      Atomics.store(view, (R.control + CB_TASK_ADDR) / 4, payloadAddr);
      Atomics.store(view, (R.control + CB_LOAD_SEQ) / 4, seq);
      Atomics.notify(view, (R.control + CB_LOAD_SEQ) / 4);
      for (; ; ) {
        let done = true;
        for (const [tid, w] of R.workers) {
          if (!w.alive) continue;
          if (!w.recordAddr) continue;
          if (this._readI32(w.recordAddr + TR_ACK) < seq) {
            done = false;
            break;
          }
        }
        if (done) break;
        await new Promise((r) => setTimeout(r, 2));
      }
      Atomics.store(view, (R.control + CB_ACKS_DONE) / 4, seq);
      Atomics.notify(view, (R.control + CB_ACKS_DONE) / 4);
      if (initBy === "coordinator") {
        const topsAddr = this._reserveStatic(delta.length * 4, 4);
        delta.forEach((rec, i) => {
          this._initRecord(rec);
          const swap = rec.instance.exports.__tinyc_dylink_swap_sp;
          let baked = 0;
          if (typeof swap === "function") {
            baked = swap(0);
            swap(baked);
          }
          this._writeI32(topsAddr + i * 4, baked);
        });
        this._writeI32(payloadAddr + 36, topsAddr);
        Atomics.store(view, (R.control + CB_LOAD_DONE) / 4, seq);
        Atomics.notify(view, (R.control + CB_LOAD_DONE) / 4);
      } else {
        while (this._readI32(R.control + CB_LOAD_DONE) < seq) {
          if (this._readI32(R.control + CB_LOAD_FAILED) === seq) {
            throw new TinycDylinkError(this._readTaskError(payloadAddr));
          }
          await new Promise((r) => setTimeout(r, 2));
        }
        for (const rec of delta) rec.state = "loaded";
      }
      R.lastTaskIndex = this._loadOrder.length;
      for (const spawn of R.spawnQueue.splice(0)) {
        this._spawnWorker(
          spawn.tid,
          this._bootData(
            spawn.tid,
            { kind: "start", slot: spawn.slot, arg: spawn.arg },
            {
              recordAddr: spawn.recordAddr,
              regions: spawn.regions,
              keyArrayAddr: spawn.keyArrayAddr
            }
          ),
          { recordAddr: spawn.recordAddr }
        );
      }
    }
    _readTaskError(payloadAddr) {
      const dv = new DataView(this.memory.buffer);
      const len = dv.getInt32(payloadAddr + 40, true);
      const bytes = new Uint8Array(this.memory.buffer).subarray(payloadAddr + 44, payloadAddr + 44 + len);
      return `tinyc: load failed in the requesting worker: ` + textDecoder2.decode(bytes);
    }
    /** Serialize a load task into shared memory (see threads.mjs
     *  for the reader). Returns the payload address. */
    _writeLoadTask(seq, delta, initBy, requesterTid) {
      const enc = textEncoder2;
      const chunks = [];
      let size = 44;
      const push = (bytes) => {
        chunks.push(bytes);
        size += bytes.length;
      };
      const gotEntries = [];
      for (const [key, e] of this._got) {
        if (!e.filled) continue;
        const idx = key.indexOf(":");
        gotEntries.push({
          kind: key.slice(0, idx),
          name: key.slice(idx + 1),
          value: e.global.value,
          definer: e.definer?.canonical ?? "",
          offset: e.offset ?? 0
        });
      }
      const head = new DataView(new ArrayBuffer(44));
      const requested = delta[delta.length - 1];
      head.setInt32(0, seq, true);
      head.setInt32(4, initBy === "requester" ? 1 : 0, true);
      head.setInt32(8, requesterTid, true);
      head.setInt32(12, requested ? requested.handle : 0, true);
      head.setInt32(16, delta.length, true);
      head.setInt32(20, this._tableFree, true);
      head.setInt32(24, gotEntries.length, true);
      head.setInt32(28, this._slotDesc.size, true);
      push(new Uint8Array(head.buffer));
      const w32 = (v) => {
        const b = new Uint8Array(4);
        new DataView(b.buffer).setInt32(0, v | 0, true);
        push(b);
      };
      const wstr = (s) => {
        const bytes = enc.encode(s);
        w32(bytes.length);
        push(bytes);
        const pad = (4 - bytes.length % 4) % 4;
        if (pad) push(new Uint8Array(pad));
      };
      for (const g of gotEntries) {
        w32(g.kind === "func" ? 0 : g.kind === "mem" ? 1 : 2);
        wstr(g.name);
        w32(g.value);
        wstr(g.definer);
        w32(g.offset);
      }
      for (const [slot, d] of this._slotDesc) {
        w32(slot);
        w32(d.kind === "stub" ? 0 : d.kind === "shim" ? 1 : 2);
        wstr(d.name);
        wstr(d.params.join(","));
        wstr(d.results.join(","));
      }
      for (const rec of delta) {
        wstr(rec.canonical);
        w32(rec.memoryBase);
        w32(rec.tableBase);
        w32(rec.handle);
        w32(rec.global ? 1 : 0);
        const manifestJson = enc.encode(JSON.stringify(rec.lib.manifest));
        w32(manifestJson.length);
        push(manifestJson);
        const pad = (4 - manifestJson.length % 4) % 4;
        if (pad) push(new Uint8Array(pad));
        const body = rec.bodyBytes ?? rec.lib.bodyBytes;
        w32(body.length);
        push(body);
        const pad2 = (4 - body.length % 4) % 4;
        if (pad2) push(new Uint8Array(pad2));
      }
      const hostImports = [...this._hostProvidedFns.keys()];
      w32(hostImports.length);
      for (const key of hostImports) wstr(key);
      const addr = this._reserveStatic(size, 4);
      const m = new Uint8Array(this.memory.buffer);
      let at = addr;
      for (const c of chunks) {
        m.set(c, at);
        at += c.length;
      }
      return addr;
    }
    /** A guest dlopen from a worker (§5.4): resolve and lay out
     *  here, broadcast, and let the requesting worker run init
     *  and complete its own call. Serialized with other loads. */
    async _realGuestLoad(msg) {
      const R = this._real;
      R.loadQueue.push(msg);
      if (R.loadsInFlight > 0) return;
      while (R.loadQueue.length > 0) {
        const req = R.loadQueue[0];
        R.loadsInFlight++;
        try {
          const canonical = canonicalModulePath(req.path, "/sysroot");
          await this._prefetchClosure(canonical);
          const before = this._loadOrder.length;
          this._loadModule(canonical, {
            flags: req.flags,
            requester: null,
            runtimeEdge: true
          });
          const delta = this._loadOrder.slice(before);
          await this._realBroadcastLoad(delta, "requester", req.fromTid);
        } catch (err) {
          const seq = R.nextLoadSeq;
          const bytes = textEncoder2.encode(err.message);
          const addr = this._reserveStatic(44 + bytes.length + 4, 4);
          const dv = new DataView(this.memory.buffer);
          dv.setInt32(addr + 8, req.fromTid, true);
          dv.setInt32(addr + 40, bytes.length, true);
          new Uint8Array(this.memory.buffer).set(bytes, addr + 44);
          const view = new Int32Array(this.memory.buffer);
          Atomics.store(view, (R.control + CB_TASK_ADDR) / 4, addr);
          Atomics.store(view, (R.control + CB_LOAD_SEQ) / 4, seq);
          Atomics.store(view, (R.control + CB_LOAD_FAILED) / 4, seq);
          Atomics.notify(view, (R.control + CB_LOAD_SEQ) / 4);
          R.nextLoadSeq++;
        } finally {
          R.loadsInFlight--;
          R.loadQueue.shift();
        }
      }
    }
    /** Delegate a host callExport to the main thread's worker
     *  (§5.5): the call runs on the main thread's context at its
     *  next wait point, and the result returns by message. */
    _realCallExport(record, name, args) {
      const R = this._real;
      const main = R.workers.get(1);
      if (!main || !main.alive) {
        throw new TinycDylinkError(
          "tinyc: callExport: a real-tier process delegates calls to its main thread worker, and none is live"
        );
      }
      for (const a of args) {
        if (typeof a !== "number") {
          throw new TinycDylinkError(
            "tinyc: callExport: real-tier delegation carries i32 arguments only"
          );
        }
      }
      const seq = R.nextCallSeq++;
      const enc = textEncoder2;
      const cBytes = enc.encode(record.canonical);
      const nBytes = enc.encode(name);
      const size = 16 + cBytes.length + 4 + nBytes.length + 4 + args.length * 4;
      const addr = this._reserveStatic(size + 8, 4);
      const dv = new DataView(this.memory.buffer);
      const m = new Uint8Array(this.memory.buffer);
      let at = addr;
      dv.setInt32(at, seq, true);
      at += 4;
      dv.setInt32(at, 0, true);
      at += 4;
      dv.setInt32(at, cBytes.length, true);
      at += 4;
      m.set(cBytes, at);
      at += cBytes.length;
      at += (4 - cBytes.length % 4) % 4;
      dv.setInt32(at, nBytes.length, true);
      at += 4;
      m.set(nBytes, at);
      at += nBytes.length;
      at += (4 - nBytes.length % 4) % 4;
      dv.setInt32(at, args.length, true);
      at += 4;
      for (const a of args) {
        dv.setInt32(at, a | 0, true);
        at += 4;
      }
      const view = new Int32Array(this.memory.buffer);
      Atomics.store(view, (R.control + CB_CALL_TASK_ADDR) / 4, addr);
      Atomics.store(view, (R.control + CB_CALL_SEQ) / 4, seq);
      Atomics.notify(view, (R.control + CB_CALL_SEQ) / 4);
      return new Promise((resolve, reject) => {
        R.callWaiters.set(seq, { resolve, reject });
      });
    }
    /** Reap a created thread in the real tier (§6.6). The
     *  exiting worker has already returned the thread's big
     *  regions — its per-module stacks and TLS areas and its
     *  key array — before its exit became observable; what
     *  remains here is the record itself, freed last, and its
     *  directory entry. The main thread's record is never
     *  reaped; it lives to teardown. */
    _reapRealThread(tid) {
      const w = this._real?.workers.get(tid);
      if (!w || w.reaped || !w.recordAddr) return;
      w.reaped = true;
      this._regionFree(w.recordAddr, 32, 4);
      this._dirClear(tid);
    }
    /** Execute one proxied host call (§5.7). The worker that
     *  owns the block is blocked on its futex word; the
     *  coordinator never blocks — a synchronous result is
     *  written back at once, a thenable's when it settles, and
     *  in both cases the answer lands in the block before the
     *  word is set and notified. Blocks are per thread, so
     *  concurrent calls from different workers proceed and
     *  complete in any order. A throw or a rejection crosses
     *  back as the error text the worker's stub rethrows. */
    _realHostCall(msg) {
      const base = msg.slot;
      if (!base) return;
      const dv = new DataView(this.memory.buffer);
      const m = new Uint8Array(this.memory.buffer);
      const view = new Int32Array(this.memory.buffer);
      const finish = (status, apply) => {
        try {
          if (apply) apply();
        } finally {
          dv.setInt32(base + PX_STATUS, status, true);
          Atomics.store(view, base / 4, PX_DONE);
          Atomics.notify(view, base / 4);
        }
      };
      const fail = (text) => finish(PX_ERROR, () => {
        const bytes = textEncoder2.encode(String(text)).subarray(0, PX_ERR_MAX);
        m.set(bytes, base + PX_ERR);
        dv.setInt32(base + PX_ERRLEN, bytes.length, true);
      });
      try {
        const nameLen = dv.getInt32(base + PX_NAMELEN, true);
        const key = textDecoder2.decode(
          m.slice(base + PX_NAME, base + PX_NAME + nameLen)
        );
        const fn = this._hostProvidedFns.get(key);
        if (typeof fn !== "function") {
          fail(`tinyc: host function "${key}" is not registered with this process`);
          return;
        }
        const nargs = dv.getInt32(base + PX_NARGS, true);
        const args = [];
        for (let i = 0; i < nargs; i++) {
          const code = m[base + PX_ARGTYPES + i];
          const at = base + PX_ARGS + i * 8;
          if (code === 1) args.push(dv.getInt32(at, true));
          else if (code === 2) args.push(dv.getBigInt64(at, true));
          else if (code === 3) args.push(dv.getFloat32(at, true));
          else if (code === 4) args.push(dv.getFloat64(at, true));
          else {
            fail(`tinyc: host function "${key}": argument ${i} carries an unknown type code ${code}`);
            return;
          }
        }
        const resCode = dv.getInt32(base + PX_RESTYPE, true);
        const writeResult = (value) => finish(PX_OK, () => {
          const at = base + PX_RESULT;
          if (resCode === 1) dv.setInt32(at, value | 0, true);
          else if (resCode === 2) {
            let b;
            try {
              b = typeof value === "bigint" ? value : BigInt(value);
            } catch {
              b = BigInt(Math.trunc(Number(value) || 0));
            }
            dv.setBigInt64(at, b, true);
          } else if (resCode === 3) dv.setFloat32(at, Number(value), true);
          else if (resCode === 4) dv.setFloat64(at, Number(value), true);
        });
        const out = fn(...args);
        if (out && typeof out.then === "function") {
          out.then(
            writeResult,
            (err) => fail(err && err.message ? err.message : String(err))
          );
        } else {
          writeResult(out);
        }
      } catch (err) {
        fail(err && err.message ? err.message : String(err));
      }
    }
    _onWorkerMessage(tid, msg) {
      const R = this._real;
      if (!R || !msg || typeof msg !== "object") return;
      switch (msg.type) {
        case "io": {
          const bytes = msg.bytes instanceof Uint8Array ? msg.bytes : new Uint8Array(msg.bytes);
          if (msg.stream === "err") this._byteIO.err(bytes);
          else this._byteIO.out(bytes);
          break;
        }
        case "thread-spawn": {
          const spawn = {
            tid: msg.threadId,
            slot: msg.slot,
            arg: msg.arg,
            recordAddr: msg.recordAddr,
            regions: msg.regions,
            keyArrayAddr: msg.keyArrayAddr
          };
          if (R.loadsInFlight > 0) {
            R.spawnQueue.push(spawn);
            break;
          }
          this._spawnWorker(
            spawn.tid,
            this._bootData(
              spawn.tid,
              { kind: "start", slot: spawn.slot, arg: spawn.arg },
              {
                recordAddr: spawn.recordAddr,
                regions: spawn.regions,
                keyArrayAddr: spawn.keyArrayAddr
              }
            ),
            { recordAddr: spawn.recordAddr }
          );
          break;
        }
        case "thread-exit": {
          const w = R.workers.get(tid);
          if (w) w.alive = false;
          if (tid === 1) R.mainExited = true;
          if (w && tid !== 1) {
            w.exited = true;
            const detached = w.recordAddr && this._readI32(w.recordAddr + TR_DETACHED) !== 0;
            if (detached || w.reapAsked) this._reapRealThread(tid);
          }
          if (R.mainRun && R.mainExited) {
            let anyAlive = false;
            for (const o of R.workers.values()) if (o.alive) anyAlive = true;
            if (!anyAlive) this._settleMainRun({ type: "all-exited" });
          }
          break;
        }
        case "thread-reap": {
          const w = R.workers.get(msg.threadId);
          if (w && msg.threadId !== 1) {
            w.reapAsked = true;
            if (w.exited) this._reapRealThread(msg.threadId);
          }
          break;
        }
        case "host-call": {
          this._realHostCall(msg);
          break;
        }
        case "main-returned":
          this._settleMainRun({ type: "entry", result: msg.result });
          break;
        case "process-exit":
          this._settleMainRun({ type: "exit", code: msg.code | 0 });
          break;
        case "thread-fault":
          this._settleMainRun({
            type: "fault",
            error: new Error(`tinyc: thread ${tid}: ${msg.message}`)
          });
          break;
        case "dlopen-request":
          void this._realGuestLoad(msg);
          break;
        case "dlclose-note": {
          const record = this._handles.get(msg.handle);
          if (record) {
            try {
              this._dlclose(msg.handle, null);
            } catch {
            }
          }
          break;
        }
        case "call-done": {
          const waiter = R.callWaiters.get(msg.seq);
          if (waiter) {
            R.callWaiters.delete(msg.seq);
            if (msg.ok) waiter.resolve(msg.value);
            else waiter.reject(new TinycDylinkError(msg.message));
          }
          break;
        }
        default:
          break;
      }
    }
    _runMainModuleSync(bytes, options = {}) {
      if (this._main) {
        throw new TinycDylinkError("tinyc: this process already has a main module");
      }
      if (this._modules.size > 0) {
        throw new TinycDylinkError(
          "tinyc: a main module must be the first module loaded into a process"
        );
      }
      const canonical = canonicalModulePath(options.path ?? "/a.out", "/");
      const lib = libraryFromBytes(canonical, bytes);
      this._libraries.set(canonical, lib);
      const record = this._loadModule(canonical, {
        flags: RTLD_GLOBAL | RTLD_NOW,
        isMain: true,
        lib
      });
      this._main = record;
      this._holder.instance = record.instance;
      const exports = record.instance.exports;
      let result;
      try {
        if (options.entry !== void 0) {
          const fn = exports[options.entry];
          if (typeof fn !== "function") {
            throw new TinycDylinkError(
              `tinyc: main module does not export "${options.entry}"`
            );
          }
          result = fn(...options.callArgs ?? []);
        } else if (typeof exports._start === "function") {
          exports._start();
          result = { exitCode: 0 };
        } else if (typeof exports.main === "function") {
          result = { exitCode: this._callMain(record, options) };
        } else {
          throw new TinycDylinkError(
            "tinyc: main module exports neither _start nor main"
          );
        }
      } catch (err) {
        if (err instanceof WasiExit || err instanceof ShimExit) {
          result = { exitCode: err.exitCode };
        } else {
          this._teardown();
          throw err;
        }
      }
      this._teardown();
      if (this._io) flushIO(this._io);
      return result;
    }
    /** Build the (argc, argv) pair for a main entry in the
     *  process's memory. */
    _mainArgv(options) {
      const args = (options.args ?? this._args).map(String);
      const strBytes = args.map((a) => textEncoder2.encode(a + "\0"));
      const total = strBytes.reduce((n, b) => n + b.length, 0) + (args.length + 1) * 4;
      const block = this._reserveStatic(total, 4);
      const m = new Uint8Array(this.memory.buffer);
      const dv = new DataView(this.memory.buffer);
      let sp = block + (args.length + 1) * 4;
      args.forEach((a, i) => {
        dv.setUint32(block + i * 4, sp, true);
        m.set(strBytes[i], sp);
        sp += strBytes[i].length;
      });
      dv.setUint32(block + args.length * 4, 0, true);
      return { argc: args.length, argv: block };
    }
    _callMain(record, options) {
      const { argc, argv } = this._mainArgv(options);
      return record.instance.exports.main(argc, argv);
    }
    /** End the process: finalize every loaded module in reverse
     *  load order with the reserved flag set (process ending). */
    _teardown() {
      if (this._ended) return;
      this._ended = true;
      for (let i = this._loadOrder.length - 1; i >= 0; i--) {
        const rec = this._loadOrder[i];
        if (rec.state !== "loaded") continue;
        rec.state = "closed";
        try {
          rec.instance.exports.__tinyc_dylink_fini(rec.handle, 1);
        } catch {
        }
      }
    }
    dispose() {
      if (this.threadTier === "real") {
        for (const { worker } of this._real.workers.values()) {
          try {
            worker.terminate();
          } catch {
          }
        }
        this._real.workers.clear();
      }
      this._teardown();
    }
  };
  function createProcess(options = {}) {
    return new TinycProcess(options);
  }
  var targetModuleCache = /* @__PURE__ */ new Map();
  function getLoadedTargets() {
    return [...targetModuleCache.keys()];
  }
  function clearTargetModuleCache() {
    targetModuleCache.clear();
  }
  async function createModuleFromAssetEnvelope(transport, envelopeUrl, target, io) {
    const asset = await retrieveAsset(
      transport,
      envelopeUrl,
      `tcc-${target}`
    );
    return buildModuleFromAsset(asset, envelopeUrl, target, io);
  }
  async function buildModuleFromAsset(asset, envelopeUrl, target, io) {
    if (!asset || typeof asset.glue !== "string" || typeof asset.wasm !== "string") {
      throw new TinycError(
        `tinyc: asset envelope at ${envelopeUrl} carries no compiler module for target "${target}"`
      );
    }
    let factory;
    const blobUrl = URL.createObjectURL(
      new Blob([asset.glue], { type: "text/javascript" })
    );
    try {
      factory = (await import(
        /* @vite-ignore */
        blobUrl
      )).default;
    } catch {
      try {
        const dataUrl = "data:text/javascript;base64," + bytesToBase64(textEncoder2.encode(asset.glue));
        factory = (await import(
          /* @vite-ignore */
          dataUrl
        )).default;
      } catch (err) {
        throw new TinycError(
          `tinyc: cannot build the compiler factory from ${envelopeUrl}: ${err.message}`
        );
      }
    } finally {
      URL.revokeObjectURL(blobUrl);
    }
    if (typeof factory !== "function") {
      throw new TinycError(
        `tinyc: compiler glue in ${envelopeUrl} does not default-export a module factory`
      );
    }
    const wasmBytes = base64ToBytes(asset.wasm);
    return factory({
      print: (text) => io.out(text + "\n"),
      printErr: (text) => io.err(text + "\n"),
      /* The glue's own instantiation override: it hands us the
       * import object and takes the instance back, so the module
       * bytes come from the envelope and the glue never fetches. */
      instantiateWasm: (imports, success) => {
        WebAssembly.instantiate(wasmBytes, imports).then(
          ({ instance }) => success(instance)
        );
        return {};
      },
      locateFile: (path, prefix) => prefix + path
    });
  }
  async function buildModuleFromHookContent(content, envelopeUrl, target, io) {
    let text;
    if (typeof content === "string") {
      text = content;
    } else if (content instanceof ArrayBuffer || ArrayBuffer.isView(content)) {
      text = textDecoder2.decode(content);
    } else {
      throw new TinycError(
        `tinyc: the loadModule hook for target "${target}" returned ${content === null ? "null" : typeof content}; it must return the asset envelope's content \u2014 the envelope file's text or its bytes \u2014 and the package builds the module from it`
      );
    }
    const asset = parseAssetEnvelopeText(text, envelopeUrl);
    return buildModuleFromAsset(asset, envelopeUrl, target, io);
  }
  async function loadTargetModule(target, options, io) {
    const cached = targetModuleCache.get(target);
    if (cached) return cached;
    const files = targetModuleFiles(target);
    const promise = (async () => {
      if (typeof options.loadModule === "function") {
        const envelopeUrl2 = assetUrl(files.envelopeFile, options.baseUrl);
        const content = await options.loadModule(target, {
          ...files,
          envelopeUrl: envelopeUrl2
        });
        return buildModuleFromHookContent(content, envelopeUrl2, target, io);
      }
      const transport = createTransport(options.transport);
      const envelopeUrl = assetUrl(files.envelopeFile, options.baseUrl);
      return createModuleFromAssetEnvelope(transport, envelopeUrl, target, io);
    })();
    targetModuleCache.set(target, promise);
    promise.catch(() => {
      if (targetModuleCache.get(target) === promise) targetModuleCache.delete(target);
    });
    return promise;
  }
  async function loadSysrootManifest(target = DEFAULT_TARGET) {
    assertKnownTarget(target);
    return SYSROOT_MANIFESTS[target];
  }
  async function loadSysroot(instance, sysrootOpt, transportOption) {
    if (!sysrootOpt) return;
    let manifest;
    let baseUrl = "./sysroot/";
    let fetchImpl;
    let includeBinaries = false;
    if (sysrootOpt === true) {
      manifest = await loadSysrootManifest(instance.target);
    } else if (Array.isArray(sysrootOpt.includeFiles)) {
      manifest = sysrootOpt;
    } else {
      manifest = sysrootOpt.manifest ?? await loadSysrootManifest(instance.target);
      baseUrl = sysrootOpt.baseUrl ?? baseUrl;
      fetchImpl = sysrootOpt.fetchImpl;
      includeBinaries = Boolean(sysrootOpt.includeBinaries);
    }
    const baseHref = /^[a-z][a-z0-9+.-]*:/i.test(baseUrl) ? baseUrl : assetUrl(baseUrl);
    const entries = [...manifest.includeFiles ?? [], ...manifest.libFiles ?? []];
    const transport = createTransport(
      sysrootOpt?.transport ?? transportOption ?? (fetchImpl ? "fetch" : void 0)
    );
    let envelope = null;
    let envelopeUrl = null;
    for (const entry of entries) {
      if (entry.binary && !includeBinaries) continue;
      if (instance.fs.hasFile(entry.path)) continue;
      if (!envelope) {
        envelopeUrl = new URL(
          `sysroot-${instance.target}.tinyc.js`,
          baseHref
        ).href;
        envelope = await retrieveAsset(
          transport,
          envelopeUrl,
          `sysroot-${instance.target}`,
          fetchImpl
        );
        if (!envelope || typeof envelope.files !== "object" || !envelope.files) {
          throw new TinycError(
            `tinyc: asset envelope at ${envelopeUrl} carries no sysroot for target "${instance.target}"`
          );
        }
      }
      const b64 = envelope.files[entry.source];
      if (typeof b64 !== "string") {
        throw new TinycError(
          `tinyc: sysroot file "${entry.source}" is missing from the asset envelope at ${envelopeUrl}`
        );
      }
      await instance.fs.addFile(entry.path, base64ToBytes(b64));
    }
    if (manifest.includeDir) instance.fs.registerIncludeDir(manifest.includeDir);
  }
  var OPFS_MOUNT_POINT = "/opfs";
  var opfsBackendByModule = /* @__PURE__ */ new WeakMap();
  var OPFS_PROBE_DIR = OPFS_MOUNT_POINT + "/.tinyc-probe";
  var OPFS_PROBE_FILE = OPFS_PROBE_DIR + "/probe";
  var OPFS_PROBE_BYTES = new Uint8Array([116, 105, 110, 121, 99]);
  function probeOpfsBackend(FS) {
    if (!FS || typeof FS.writeFile !== "function" || typeof FS.readFile !== "function") {
      return false;
    }
    try {
      if (typeof FS.mkdirTree === "function") FS.mkdirTree(OPFS_PROBE_DIR);
      FS.writeFile(OPFS_PROBE_FILE, OPFS_PROBE_BYTES);
      const back = FS.readFile(OPFS_PROBE_FILE);
      if (!back || back.length !== OPFS_PROBE_BYTES.length) return false;
      for (let i = 0; i < OPFS_PROBE_BYTES.length; i++) {
        if (back[i] !== OPFS_PROBE_BYTES[i]) return false;
      }
      return true;
    } catch {
      return false;
    } finally {
      try {
        FS.unlink(OPFS_PROBE_FILE);
      } catch {
      }
      if (typeof FS.rmdir === "function") {
        try {
          FS.rmdir(OPFS_PROBE_DIR);
        } catch {
        }
      }
    }
  }
  async function tryMountOpfsBackend(native, bridge) {
    const canMemo = native !== null && (typeof native === "object" || typeof native === "function");
    if (canMemo) {
      const memo = opfsBackendByModule.get(native);
      if (memo) return memo;
    }
    let result = "memory";
    try {
      if (bridge.mode === "emscripten" && typeof native.ccall === "function" && typeof native._wasmfs_create_opfs_backend === "function" && typeof native._wasmfs_create_directory === "function" && typeof WorkerGlobalScope !== "undefined" && typeof self !== "undefined" && self instanceof WorkerGlobalScope) {
        const backend = await native.ccall(
          "wasmfs_create_opfs_backend",
          "number",
          [],
          [],
          { async: true }
        );
        if (backend) {
          const rc = await native.ccall(
            "wasmfs_create_directory",
            "number",
            ["string", "number", "number"],
            [OPFS_MOUNT_POINT, 511, backend],
            { async: true }
          );
          if (rc === 0 && probeOpfsBackend(native.FS)) result = "opfs";
        }
      }
    } catch {
      result = "memory";
    }
    if (canMemo) opfsBackendByModule.set(native, result);
    return result;
  }
  async function loadTinycWithNative(native, options = {}) {
    const target = options.target ?? DEFAULT_TARGET;
    assertKnownTarget(target);
    const io = normalizeIO(options.io);
    const store = await openFsStore(target, options);
    const fs = new VirtualFS(store);
    const bridge = new NativeBridge(native, io);
    const instance = new Tinyc(bridge, { fs, io, target });
    instance._fsBackend = await tryMountOpfsBackend(native, bridge);
    bridge.moduleFsPrefix = instance._fsBackend === "opfs" ? OPFS_MOUNT_POINT : "";
    if (native && native.FS) await fs.mountTo(native.FS, bridge.moduleFsPrefix);
    await loadSysroot(instance, options.sysroot, options.transport);
    return instance;
  }
  async function loadTinyc(options = {}) {
    const target = options.target ?? DEFAULT_TARGET;
    assertKnownTarget(target);
    const io = normalizeIO(options.io);
    const native = await loadTargetModule(target, options, io);
    return loadTinycWithNative(native, options);
  }
  var index_default = loadTinyc;
  return __toCommonJS(web_entry_exports);
})();
