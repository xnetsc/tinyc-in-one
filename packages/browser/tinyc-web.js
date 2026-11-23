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
    SUPPORTED_TARGETS: () => SUPPORTED_TARGETS,
    TARGETS: () => TARGETS,
    TCC_OUTPUT: () => TCC_OUTPUT,
    Tinyc: () => Tinyc,
    TinycError: () => TinycError,
    TinycMissingExportError: () => TinycMissingExportError,
    TinycMissingImportError: () => TinycMissingImportError,
    TinycRunNotSupportedError: () => TinycRunNotSupportedError,
    TinycState: () => TinycState,
    VFS_CACHE_MAX_BYTES: () => VFS_CACHE_MAX_BYTES,
    VFS_CACHE_MAX_ENTRIES: () => VFS_CACHE_MAX_ENTRIES,
    VIRTUAL_BIN_PATH: () => VIRTUAL_BIN_PATH,
    VirtualFS: () => VirtualFS,
    WasiExit: () => WasiExit,
    clearTargetModuleCache: () => clearTargetModuleCache,
    createDefaultIO: () => createDefaultIO,
    createFsStore: () => createFsStore,
    default: () => index_default,
    flushIO: () => flushIO,
    getLoadedTargets: () => getLoadedTargets,
    isRunnableTarget: () => isRunnableTarget,
    loadSysrootManifest: () => loadSysrootManifest,
    loadTinyc: () => loadTinyc,
    normalizeIO: () => normalizeIO,
    normalizeVfsPath: () => normalizeVfsPath,
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
        path: "/sysroot/include/float.h",
        source: "include/float.h"
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
        source: "lib/x86_64-windows/libtcc1.a"
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
      return textDecoder.decode(m.subarray(ptr, end));
    };
    const writeCStringBytes = (ptr, bytes) => {
      writeBytes(ptr, bytes);
      u8()[ptr + bytes.length] = 0;
      return bytes.length;
    };
    let heapTop = 0;
    let freeHead = 0;
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
    function rawMalloc(size) {
      size = Math.max(8, size + 7 & ~7);
      let prev = 0;
      let cur = freeHead;
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
            else freeHead = rem;
          } else {
            if (prev) writeU32(prev + 4, cnext);
            else freeHead = cnext;
          }
          writeU32(cur + 4, ALLOC_USED);
          return cur + 8;
        }
        prev = cur;
        cur = cnext;
      }
      if (!heapTop) heapTop = heapBase;
      const header = heapTop;
      const end = header + 8 + size;
      if (!ensureCapacity(end)) return 0;
      writeU32(header, size);
      writeU32(header + 4, ALLOC_USED);
      heapTop = end;
      return header + 8;
    }
    function rawFree(ptr) {
      if (!ptr) return;
      const header = ptr - 8;
      if (readU32(header + 4) !== ALLOC_USED) return;
      writeU32(header + 4, freeHead);
      freeHead = header;
    }
    function malloc(size) {
      const own = moduleAllocator();
      return own ? own.malloc(size) : rawMalloc(size);
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
        if (!np2) return 0;
        writeBytes(np2, readBytes(ptr, size));
        if (typeof own.free === "function") own.free(ptr);
        return np2;
      }
      const oldSize = readU32(ptr - 8);
      if (size <= oldSize) return ptr;
      const np = rawMalloc(size);
      if (!np) return 0;
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
      const table = holder?.instance?.exports?.__indirect_function_table;
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
      const result = new Uint8Array(got);
      let at = 0;
      for (const p of parts) {
        result.set(p, at);
        at += p.length;
      }
      return result;
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
      return emitText(formatString(fmt, va), stream === 2);
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
    function puts(ptr) {
      return emitText(readCString(ptr) + "\n", false) >= 0 ? 0 : -1;
    }
    function putchar(c) {
      emitOut(Uint8Array.of(c & 255));
      return c;
    }
    function fputs(ptr, stream) {
      emitText(readCString(ptr), stream === 2);
      return 0;
    }
    function putc(c, stream) {
      (stream === 2 ? emitErr : emitOut)(Uint8Array.of(c & 255));
      return c;
    }
    function fwrite(ptr, size, nmemb, stream) {
      const total = size * nmemb;
      if (total === 0) return 0;
      (stream === 2 ? emitErr : emitOut)(readBytes(ptr, total));
      return nmemb;
    }
    function fread(ptr, size, nmemb, _stream) {
      if (size === 0 || nmemb === 0) return 0;
      const data = stdinRead(size * nmemb);
      if (data.length) writeBytes(ptr, data);
      return Math.floor(data.length / size);
    }
    function fgetc(_stream) {
      const data = stdinRead(1);
      return data.length ? data[0] : -1;
    }
    function ungetc(c, _stream) {
      if (c >= 0) pushback.push(c & 255);
      return c;
    }
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
      // math
      sin: Math.sin,
      cos: Math.cos,
      tan: Math.tan,
      asin: Math.asin,
      acos: Math.acos,
      atan: Math.atan,
      atan2: Math.atan2,
      exp: Math.exp,
      log: Math.log,
      log10: Math.log10,
      pow: Math.pow,
      sqrt: Math.sqrt,
      floor: Math.floor,
      ceil: Math.ceil,
      fabs: Math.abs,
      round: Math.round,
      trunc: Math.trunc,
      fmod: (a, b) => a % b,
      // time
      time: (tloc) => {
        const sec = Math.floor(Date.now() / 1e3);
        if (tloc) writeU32(tloc, sec);
        return sec;
      },
      clock: () => Math.floor(performance.now() * 1e3),
      difftime: (a, b) => a - b
    };
    return { env };
  }

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
  function isRunnableTarget(target) {
    return Boolean(TARGETS[target]?.runnable);
  }
  function targetModuleFiles(target) {
    return { wasmFile: `tcc-${target}.wasm`, glueFile: `tcc-${target}.mjs` };
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
        return textDecoder2.decode(m.subarray(ptr, end));
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
    }
    _call(name, returnType, argTypes, args) {
      if (this._deleted) throw new TinycError("tinyc: state already deleted");
      return this._bridge.call(name, returnType, argTypes, [this.handle, ...args]);
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
      return this;
    }
    setOptions(str) {
      return this._checkRc(
        "tcc_set_options",
        this._call("tcc_set_options", "number", ["string"], [str])
      );
    }
    addIncludePath(path) {
      return this._checkRc(
        "tcc_add_include_path",
        this._call("tcc_add_include_path", "number", ["string"], [this._bridge.modulePath(path)])
      );
    }
    addSysincludePath(path) {
      return this._checkRc(
        "tcc_add_sysinclude_path",
        this._call("tcc_add_sysinclude_path", "number", ["string"], [this._bridge.modulePath(path)])
      );
    }
    addLibraryPath(path) {
      return this._checkRc(
        "tcc_add_library_path",
        this._call("tcc_add_library_path", "number", ["string"], [this._bridge.modulePath(path)])
      );
    }
    addLibrary(name) {
      return this._checkRc(
        "tcc_add_library",
        this._call("tcc_add_library", "number", ["string"], [name])
      );
    }
    defineSymbol(sym, value = "1") {
      this._call("tcc_define_symbol", null, ["string", "string"], [sym, value]);
      return this;
    }
    undefineSymbol(sym) {
      this._call("tcc_undefine_symbol", null, ["string"], [sym]);
      return this;
    }
    setOutputType(type) {
      return this._checkRc(
        "tcc_set_output_type",
        this._call("tcc_set_output_type", "number", ["number"], [type])
      );
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
      return this._checkRc(
        "tcc_add_file",
        this._call("tcc_add_file", "number", ["string"], [this._bridge.modulePath(p)])
      );
    }
    compileString(source) {
      return this._checkRc(
        "tcc_compile_string",
        this._call("tcc_compile_string", "number", ["string"], [source])
      );
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
      return this._checkRc(
        "tcc_compile_string",
        this._call("tcc_compile_string", "number", ["string"], [marked])
      );
    }
    /** Write the compiled product to a virtual path. TCC writes it
     *  inside the module FS (at bridge.modulePath(path)); exportFile
     *  reads it back from there into the virtual FS. */
    outputFile(path) {
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
     *  options.entry is given. This switches the state to EXE output
     *  type. On every other target the glue layer throws
     *  TinycRunNotSupportedError before anything is passed to the
     *  native module — those targets compile to files only.
     *  options: { outputPath, entry, callArgs, env, imports, shims,
     *  stdin, heapBase, io } — entry/callArgs/env/imports/shims/stdin/
     *  heapBase forward to runWasmModuleSync, and the instance's
     *  registered shims apply automatically; io is a text IO partial
     *  (default: this state's IO) whose out/err receive the program's
     *  fd_write output, decoded. */
    run(args = [], options = {}) {
      if (!isRunnableTarget(this._instance.target)) {
        throw new TinycRunNotSupportedError(this._instance.target);
      }
      const outputPath = options.outputPath ?? DEFAULT_RUN_OUTPUT;
      this.setOutputType(TCC_OUTPUT.EXE);
      this.outputFile(outputPath);
      const bytes = this._instance.exportFile(outputPath);
      const io = normalizeIO(options.io, this.io);
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
      let cacheMiss = null;
      if (this.fs.hasFile(p)) {
        try {
          return this.fs.readFile(p);
        } catch (err) {
          cacheMiss = err;
        }
      }
      const FS = this._bridge.native?.FS;
      if (FS && typeof FS.readFile === "function") {
        try {
          const bytes = FS.readFile(this._bridge.modulePath(p));
          this.fs.addFileSync(p, bytes);
          return this.fs.readFile(p);
        } catch {
        }
      }
      if (cacheMiss) {
        throw new TinycError(
          `tinyc: ${p} is persisted in the virtual FS store but is not in the in-memory cache; read it with exportFileAsync() instead`
        );
      }
      throw new TinycError(`tinyc: no such file in virtual FS: ${p}`);
    }
    /** exportFile, reading the content from the persistent store
     *  when it is not in the content cache. */
    async exportFileAsync(path) {
      const p = normalizeVfsPath(path);
      if (this.fs.hasFile(p)) return this.fs.readFileAsync(p);
      const FS = this._bridge.native?.FS;
      if (FS && typeof FS.readFile === "function") {
        try {
          const bytes = FS.readFile(this._bridge.modulePath(p));
          this.fs.addFileSync(p, bytes);
          return bytes;
        } catch {
        }
      }
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
      const rc = this.callMain([...compileArgs, "-o", DEFAULT_RUN_OUTPUT, src]);
      if (rc !== 0) return rc;
      const bytes = this.exportFile(DEFAULT_RUN_OUTPUT);
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
      if (this._bridge.has("tcc_add_include_path")) {
        for (const dir of this.fs.getIncludeDirs()) state.addIncludePath(dir);
      }
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
  var targetModuleCache = /* @__PURE__ */ new Map();
  function getLoadedTargets() {
    return [...targetModuleCache.keys()];
  }
  function clearTargetModuleCache() {
    targetModuleCache.clear();
  }
  async function createModuleFromGlue(glueUrl, wasmUrl, io) {
    let factory;
    try {
      factory = (await import(
        /* @vite-ignore */
        glueUrl
      )).default;
    } catch {
      throw new TinycError(
        `tinyc: the Emscripten glue ${glueUrl} could not be loaded. Build it with build.sh, or pass module / createModule / wasmExports / loadModule.`
      );
    }
    if (typeof factory !== "function") {
      throw new TinycError(`tinyc: Emscripten glue ${glueUrl} does not default-export a module factory`);
    }
    return factory({
      print: (text) => io.out(text + "\n"),
      printErr: (text) => io.err(text + "\n"),
      locateFile: (path, prefix) => wasmUrl && path.endsWith(".wasm") ? wasmUrl : prefix + path
    });
  }
  async function loadTargetModule(target, options, io) {
    const cached = targetModuleCache.get(target);
    if (cached) return cached;
    const files = targetModuleFiles(target);
    const promise = (async () => {
      if (typeof options.loadModule === "function") {
        return options.loadModule(target, {
          ...files,
          wasmUrl: options.wasmUrl ?? assetUrl(files.wasmFile, options.baseUrl),
          glueUrl: options.glueUrl ?? assetUrl(files.glueFile, options.baseUrl)
        });
      }
      const glueUrl = options.glueUrl ?? assetUrl(files.glueFile, options.baseUrl);
      const wasmUrl = options.wasmUrl ?? assetUrl(files.wasmFile, options.baseUrl);
      return createModuleFromGlue(glueUrl, wasmUrl, io);
    })();
    targetModuleCache.set(target, promise);
    promise.catch(() => {
      if (targetModuleCache.get(target) === promise) targetModuleCache.delete(target);
    });
    return promise;
  }
  async function resolveNative(options, io, target) {
    if (options.module) return options.module;
    if (options.wasmExports) return options.wasmExports;
    if (typeof options.createModule === "function") {
      return await options.createModule({
        print: (text) => io.out(text + "\n"),
        printErr: (text) => io.err(text + "\n"),
        locateFile: (path, prefix) => options.wasmUrl && path.endsWith(".wasm") ? options.wasmUrl : prefix + path
      });
    }
    return loadTargetModule(target, options, io);
  }
  async function loadSysrootManifest(target = DEFAULT_TARGET) {
    assertKnownTarget(target);
    return SYSROOT_MANIFESTS[target];
  }
  async function loadSysroot(instance, sysrootOpt) {
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
    for (const entry of entries) {
      if (entry.binary && !includeBinaries) continue;
      if (instance.fs.hasFile(entry.path)) continue;
      const url = entry.url ?? new URL(entry.source, baseHref).href;
      await instance.fs.addFileFromUrl(entry.path, url, { fetchImpl });
    }
    if (manifest.includeDir) instance.fs.registerIncludeDir(manifest.includeDir);
  }
  var OPFS_MOUNT_POINT = "/opfs";
  var opfsBackendByModule = /* @__PURE__ */ new WeakMap();
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
          if (rc === 0) result = "opfs";
        }
      }
    } catch {
      result = "memory";
    }
    if (canMemo) opfsBackendByModule.set(native, result);
    return result;
  }
  async function loadTinyc(options = {}) {
    const target = options.target ?? DEFAULT_TARGET;
    assertKnownTarget(target);
    const io = normalizeIO(options.io);
    const store = await openFsStore(target, options);
    const fs = new VirtualFS(store);
    const native = await resolveNative(options, io, target);
    const bridge = new NativeBridge(native, io);
    const instance = new Tinyc(bridge, { fs, io, target });
    instance._fsBackend = await tryMountOpfsBackend(native, bridge);
    bridge.moduleFsPrefix = instance._fsBackend === "opfs" ? OPFS_MOUNT_POINT : "";
    if (native && native.FS) await fs.mountTo(native.FS, bridge.moduleFsPrefix);
    await loadSysroot(instance, options.sysroot);
    return instance;
  }
  var index_default = loadTinyc;
  return __toCommonJS(web_entry_exports);
})();
