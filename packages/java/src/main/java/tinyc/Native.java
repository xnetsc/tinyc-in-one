package tinyc;

import java.io.IOException;
import java.io.InputStream;
import java.lang.foreign.Arena;
import java.lang.foreign.FunctionDescriptor;
import java.lang.foreign.Linker;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.SymbolLookup;
import java.lang.foreign.ValueLayout;
import java.lang.invoke.MethodHandle;
import java.net.JarURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;
import java.util.Properties;
import java.util.jar.JarEntry;
import java.util.jar.JarFile;

import static java.lang.foreign.ValueLayout.ADDRESS;
import static java.lang.foreign.ValueLayout.JAVA_INT;

/**
 * 原生文件定位与全部 downcall 句柄。只在这里加载原生文件，其他类不直接碰。
 *
 * <p>分发模型：包类型 x 宿主 x 目标。宿主（TCC 在哪运行：OS x 64 位 CPU）
 * 决定加载哪个双形态二进制；目标（一二进制一目标，靠并列变体）决定用哪个
 * 变体与哪份 sysroot。Jar 内统一布局：
 *
 * <pre>
 *   packaging-manifest.json
 *   natives/&lt;host-triple&gt;/toolchain/&lt;target-triple&gt;/tinyc[.exe]
 *   sysroot/&lt;target-triple&gt;/lib/tcc/...   （每个目标一份，各宿主共享）
 * </pre>
 *
 * <p>定位顺序：
 * <ol>
 *   <li>系统属性 {@code tinyc.native.path} 直接指定本地原生文件（开发/测试用）；
 *       配套的 TCC 运行时目录取系统属性 {@code tinyc.lib.path}，
 *       未给时若原生文件同目录下有 {@code libtcc1.a} 就用该目录。</li>
 *   <li>否则探测宿主 triple（环境变量 {@code TINYC_HOST_TRIPLE} 可覆盖探测，
 *       目标 triple 用 {@code TINYC_TARGET_TRIPLE}，缺省等于宿主），在
 *       {@code packaging-manifest.json} 里查 (host, target) 组合，把该组合的
 *       二进制与目标 sysroot 解压到 {@code ~/.cache/tinyc-java/<version>/}，
 *       已解压且标记完整时复用。查不到组合时报错并列出包内可用组合；
 *       32 位宿主直接报不支持，不做任何回退。</li>
 * </ol>
 *
 * <p>调用的 C 符号出自 libtcc.h 与 tinyc_io.h 并经导出表核对；libtcc.h 没有
 * {@code tcc_compile_file}，编译文件由 {@code tcc_add_file} 承担，见 {@link TccSession#compileFile}。
 */
final class Native {

    static final Path FILE;
    static final Path RUNTIME_DIR; // 可为 null：无从得知时不自动设置 lib 路径
    static final String HOST_TRIPLE;
    static final String TARGET_TRIPLE;
    /** 已解析组合的 outputs 字段（manifest 原始 JSON 片段）：目标能生成的
     * 输出格式以 dist manifest 为唯一权威，各包清单逐字复制，加载器只读
     * 不写死。开发模式（tinyc.native.path）或清单缺该字段时为 null。 */
    static final String OUTPUTS_JSON;

    static final MethodHandle TCC_NEW;
    static final MethodHandle TCC_DELETE;
    static final MethodHandle TCC_SET_LIB_PATH;
    static final MethodHandle TCC_SET_ERROR_FUNC;
    static final MethodHandle TCC_SET_OPTIONS;
    static final MethodHandle TCC_ADD_INCLUDE_PATH;
    static final MethodHandle TCC_ADD_SYSINCLUDE_PATH;
    static final MethodHandle TCC_DEFINE_SYMBOL;
    static final MethodHandle TCC_UNDEFINE_SYMBOL;
    static final MethodHandle TCC_ADD_FILE;
    static final MethodHandle TCC_COMPILE_STRING;
    static final MethodHandle TCC_SET_OUTPUT_TYPE;
    static final MethodHandle TCC_ADD_LIBRARY_PATH;
    static final MethodHandle TCC_ADD_LIBRARY;
    static final MethodHandle TCC_ADD_SYMBOL;
    static final MethodHandle TCC_OUTPUT_FILE;
    static final MethodHandle TCC_RUN;
    static final MethodHandle TCC_RELOCATE;
    static final MethodHandle TCC_GET_SYMBOL;
    static final MethodHandle TCC_IO_CALLBACK;
    static final MethodHandle TCC_IO_FREE;

    /** 导出的 main 入口：tinyc_main 优先，无则 main；原生文件没有时为 null。 */
    static final MethodHandle MAIN;
    static final String MAIN_NAME;

    /** 带 IO 的 main 入口：tinyc_main_io；原生文件没有时为 null。 */
    static final MethodHandle MAIN_IO;

    /** libc 的 fflush，用于 main 调用返回后冲刷 C stdio；取不到时为 null。 */
    static final MethodHandle FFLUSH;

    private static final List<String> ARCH_32BIT =
            List.of("x86", "i386", "i486", "i586", "i686", "ia32", "arm");

    static {
        try {
            Path file;
            Path runtime;
            String propPath = System.getProperty("tinyc.native.path");
            if (propPath != null && !propPath.isBlank()) {
                HOST_TRIPLE = hostTriple();
                TARGET_TRIPLE = targetTriple(HOST_TRIPLE);
                OUTPUTS_JSON = null;
                file = Path.of(propPath).toAbsolutePath();
                if (!Files.isRegularFile(file)) {
                    throw new TccException("tinyc.native.path is not a file: " + file);
                }
                String libProp = System.getProperty("tinyc.lib.path");
                if (libProp != null && !libProp.isBlank()) {
                    runtime = Path.of(libProp).toAbsolutePath();
                } else {
                    Path parent = file.getParent();
                    runtime = parent != null && Files.isRegularFile(parent.resolve("libtcc1.a"))
                            ? parent : null;
                }
            } else {
                HOST_TRIPLE = hostTriple();
                TARGET_TRIPLE = targetTriple(HOST_TRIPLE);
                Combo combo = resolveCombination(HOST_TRIPLE, TARGET_TRIPLE);
                OUTPUTS_JSON = combo.outputsJson();
                Path cache = extractCombination(combo);
                file = cache.resolve(combo.binary);
                runtime = cache.resolve(combo.sysroot).resolve("lib").resolve("tcc");
            }
            FILE = file;
            RUNTIME_DIR = runtime;

            SymbolLookup lookup = SymbolLookup.libraryLookup(file.toString(), Arena.global());
            Linker linker = Linker.nativeLinker();

            FunctionDescriptor ptr = FunctionDescriptor.of(ADDRESS);
            FunctionDescriptor vA = FunctionDescriptor.ofVoid(ADDRESS);
            FunctionDescriptor vAA = FunctionDescriptor.ofVoid(ADDRESS, ADDRESS);
            FunctionDescriptor iAA = FunctionDescriptor.of(JAVA_INT, ADDRESS, ADDRESS);

            TCC_NEW = required(linker, lookup, "tcc_new", ptr);
            TCC_DELETE = required(linker, lookup, "tcc_delete", vA);
            TCC_SET_LIB_PATH = required(linker, lookup, "tcc_set_lib_path", vAA);
            TCC_SET_ERROR_FUNC = required(linker, lookup, "tcc_set_error_func",
                    FunctionDescriptor.ofVoid(ADDRESS, ADDRESS, ADDRESS));
            TCC_SET_OPTIONS = required(linker, lookup, "tcc_set_options", iAA);
            TCC_ADD_INCLUDE_PATH = required(linker, lookup, "tcc_add_include_path", iAA);
            TCC_ADD_SYSINCLUDE_PATH = required(linker, lookup, "tcc_add_sysinclude_path", iAA);
            TCC_DEFINE_SYMBOL = required(linker, lookup, "tcc_define_symbol",
                    FunctionDescriptor.ofVoid(ADDRESS, ADDRESS, ADDRESS));
            TCC_UNDEFINE_SYMBOL = required(linker, lookup, "tcc_undefine_symbol", vAA);
            TCC_ADD_FILE = required(linker, lookup, "tcc_add_file", iAA);
            TCC_COMPILE_STRING = required(linker, lookup, "tcc_compile_string", iAA);
            TCC_SET_OUTPUT_TYPE = required(linker, lookup, "tcc_set_output_type",
                    FunctionDescriptor.of(JAVA_INT, ADDRESS, JAVA_INT));
            TCC_ADD_LIBRARY_PATH = required(linker, lookup, "tcc_add_library_path", iAA);
            TCC_ADD_LIBRARY = required(linker, lookup, "tcc_add_library", iAA);
            TCC_ADD_SYMBOL = required(linker, lookup, "tcc_add_symbol",
                    FunctionDescriptor.of(JAVA_INT, ADDRESS, ADDRESS, ADDRESS));
            TCC_OUTPUT_FILE = required(linker, lookup, "tcc_output_file", iAA);
            TCC_RUN = optional(linker, lookup, "tcc_run",
                    FunctionDescriptor.of(JAVA_INT, ADDRESS, JAVA_INT, ADDRESS));
            TCC_RELOCATE = optional(linker, lookup, "tcc_relocate",
                    FunctionDescriptor.of(JAVA_INT, ADDRESS));
            TCC_GET_SYMBOL = required(linker, lookup, "tcc_get_symbol",
                    FunctionDescriptor.of(ADDRESS, ADDRESS, ADDRESS));
            TCC_IO_CALLBACK = required(linker, lookup, "tcc_io_callback",
                    FunctionDescriptor.of(ADDRESS, ADDRESS, ADDRESS, ADDRESS, ADDRESS));
            TCC_IO_FREE = required(linker, lookup, "tcc_io_free", vA);

            MemorySegment mainSeg = lookup.find("tinyc_main").orElse(null);
            String mainName = "tinyc_main";
            if (mainSeg == null) {
                mainSeg = lookup.find("main").orElse(null);
                mainName = "main";
            }
            MAIN = mainSeg != null
                    ? linker.downcallHandle(mainSeg, FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS))
                    : null;
            MAIN_NAME = MAIN != null ? mainName : null;

            MAIN_IO = lookup.find("tinyc_main_io")
                    .map(seg -> linker.downcallHandle(seg,
                            FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS, ADDRESS)))
                    .orElse(null);

            FFLUSH = linker.defaultLookup().find("fflush")
                    .map(seg -> linker.downcallHandle(seg, FunctionDescriptor.of(JAVA_INT, ADDRESS)))
                    .orElse(null);
        } catch (IOException e) {
            throw new TccException("failed to prepare TinyC native file", e);
        }
    }

    private Native() {
    }

    private static MethodHandle required(Linker linker, SymbolLookup lookup,
                                         String name, FunctionDescriptor descriptor) {
        MemorySegment seg = lookup.find(name)
                .orElseThrow(() -> new TccException("native symbol not found: " + name + " in " + FILE));
        return linker.downcallHandle(seg, descriptor);
    }

    /** 交叉变体（host != target）不导出 tcc_relocate/tcc_run：把目标代码
     * 重定位进本进程只有原生组合成立。这两个符号缺失时返回 null，
     * 由会话层在调用点给出指名错误。 */
    private static MethodHandle optional(Linker linker, SymbolLookup lookup,
                                         String name, FunctionDescriptor descriptor) {
        return lookup.find(name)
                .map(seg -> linker.downcallHandle(seg, descriptor))
                .orElse(null);
    }

    // ------------------------------------------------------------ triple 探测

    private static String check64Bit(String triple, String what) {
        String arch = triple.split("-", 2)[0].toLowerCase();
        if (ARCH_32BIT.contains(arch) || arch.startsWith("armv")) {
            throw new TccException("tinyc: 32-bit hosts are not supported (" + what
                    + " triple: " + triple + "); tinyc ships 64-bit host binaries only");
        }
        return triple;
    }

    /** 宿主 triple：环境变量 TINYC_HOST_TRIPLE（或系统属性 tinyc.host.triple）
     * 覆盖探测结果，供测试模拟其它宿主。 */
    static String hostTriple() {
        String override = System.getenv("TINYC_HOST_TRIPLE");
        if (override == null || override.isBlank()) {
            override = System.getProperty("tinyc.host.triple");
        }
        if (override != null && !override.isBlank()) {
            return check64Bit(override.trim(), "host");
        }
        String os = System.getProperty("os.name", "");
        String arch = System.getProperty("os.arch", "").toLowerCase();
        if (ARCH_32BIT.contains(arch) || arch.startsWith("armv")) {
            throw new TccException("tinyc: 32-bit hosts are not supported (os.arch: "
                    + arch + "); tinyc ships 64-bit host binaries only");
        }
        if (os.contains("Linux")) {
            return switch (arch) {
                case "amd64", "x86_64" -> "x86_64-linux-gnu";
                case "aarch64" -> "aarch64-linux-gnu";
                case "riscv64" -> "riscv64-linux-gnu";
                default -> throw new TccException("tinyc: unsupported host arch: " + arch);
            };
        }
        if (os.contains("Windows")) {
            if (arch.equals("amd64") || arch.equals("x86_64")) return "x86_64-w64-mingw32";
            throw new TccException("tinyc: unsupported host arch: " + arch);
        }
        if (os.contains("Mac OS X") || os.contains("Darwin")) {
            if (arch.equals("aarch64")) return "arm64-apple-darwin";
            if (arch.equals("x86_64") || arch.equals("amd64")) return "x86_64-apple-darwin";
            throw new TccException("tinyc: unsupported host arch: " + arch);
        }
        throw new TccException("tinyc: unsupported host OS: " + os);
    }

    static String targetTriple(String host) {
        String override = System.getenv("TINYC_TARGET_TRIPLE");
        if (override == null || override.isBlank()) {
            override = System.getProperty("tinyc.target.triple");
        }
        if (override != null && !override.isBlank()) {
            return check64Bit(override.trim(), "target");
        }
        return host;
    }

    // ------------------------------------------------------------ 组合解析

    private record Combo(String host, String target, String binary, String sysroot,
                         String outputsJson) {
    }

    private static Combo resolveCombination(String host, String target) throws IOException {
        List<Combo> combos = readManifest();
        for (Combo c : combos) {
            if (c.host().equals(host) && c.target().equals(target)) {
                return c;
            }
        }
        StringBuilder available = new StringBuilder();
        for (Combo c : combos) {
            if (available.length() > 0) available.append(", ");
            available.append(c.host()).append(" x ").append(c.target());
        }
        throw new TccException("tinyc: no native build for host " + host + " / target "
                + target + " in this package; available combinations: "
                + (available.length() == 0 ? "none" : available));
    }

    /** 读 classpath 上的 packaging-manifest.json，取出 combinations 数组。
     * 清单由 packaging/stage_layout.py 生成。组合对象的值可含嵌套数组与
     * 对象（如 isa 字段的对象值、outputs 对象），截取数组与对象边界时按
     * 嵌套深度扫描，字符串字面量内的括号不计数。 */
    private static List<Combo> readManifest() throws IOException {
        String text;
        try (InputStream in = Native.class.getResourceAsStream("/packaging-manifest.json")) {
            if (in == null) {
                throw new TccException("packaging-manifest.json not found on classpath, "
                        + "and tinyc.native.path is not set");
            }
            text = new String(in.readAllBytes(), StandardCharsets.UTF_8);
        }
        int start = text.indexOf("\"combinations\"");
        if (start < 0) throw new TccException("packaging-manifest.json has no combinations");
        int arrayStart = text.indexOf('[', start);
        if (arrayStart < 0) {
            throw new TccException("packaging-manifest.json: malformed combinations");
        }
        String body = text.substring(arrayStart + 1, jsonValueEnd(text, arrayStart) - 1);
        List<Combo> combos = new ArrayList<>();
        int i = 0;
        while (true) {
            int objStart = body.indexOf('{', i);
            if (objStart < 0) break;
            int objEnd = jsonValueEnd(body, objStart);
            String obj = body.substring(objStart, objEnd);
            combos.add(new Combo(jsonField(obj, "host"), jsonField(obj, "target"),
                    jsonField(obj, "binary"), jsonField(obj, "sysroot"),
                    jsonRawValue(obj, "outputs")));
            i = objEnd;
        }
        return combos;
    }

    /** text[start] 处的 JSON 对象/数组值的结束下标（不含）：按嵌套深度
     * 扫描，字符串字面量内的括号不计数，反斜杠转义跳过下一字符。 */
    private static int jsonValueEnd(String text, int start) {
        int depth = 0;
        boolean inString = false;
        for (int i = start; i < text.length(); i++) {
            char ch = text.charAt(i);
            if (inString) {
                if (ch == '\\') i++;
                else if (ch == '"') inString = false;
            } else if (ch == '"') {
                inString = true;
            } else if (ch == '{' || ch == '[') {
                depth++;
            } else if (ch == '}' || ch == ']') {
                if (--depth == 0) return i + 1;
            }
        }
        throw new TccException("packaging-manifest.json: unterminated JSON value");
    }

    /** 提取字段的原始 JSON 值片段（字符串/数组/对象/标量），找不到返回 null。 */
    private static String jsonRawValue(String obj, String key) {
        String needle = "\"" + key + "\"";
        int at = obj.indexOf(needle);
        if (at < 0) return null;
        int colon = obj.indexOf(':', at + needle.length());
        if (colon < 0) return null;
        int i = colon + 1;
        while (i < obj.length() && Character.isWhitespace(obj.charAt(i))) i++;
        if (i >= obj.length()) return null;
        char first = obj.charAt(i);
        if (first == '"') {
            int end = obj.indexOf('"', i + 1);
            return end < 0 ? null : obj.substring(i, end + 1);
        }
        if (first == '{' || first == '[') {
            return obj.substring(i, jsonValueEnd(obj, i));
        }
        int end = i;
        while (end < obj.length() && ",}]".indexOf(obj.charAt(end)) < 0) end++;
        return obj.substring(i, end).trim();
    }

    private static String jsonField(String obj, String key) {
        String needle = "\"" + key + "\"";
        int at = obj.indexOf(needle);
        if (at < 0) throw new TccException("packaging-manifest.json: missing field " + key);
        int colon = obj.indexOf(':', at + needle.length());
        int q1 = obj.indexOf('"', colon + 1);
        int q2 = obj.indexOf('"', q1 + 1);
        if (colon < 0 || q1 < 0 || q2 < 0) {
            throw new TccException("packaging-manifest.json: malformed field " + key);
        }
        return obj.substring(q1 + 1, q2);
    }

    /** 包版本，来自 build 时写进 jar 的 tinyc-native.properties。 */
    private static String packageVersion() {
        Properties p = new Properties();
        try (InputStream in = Native.class.getResourceAsStream("/tinyc-native.properties")) {
            if (in != null) p.load(in);
        } catch (IOException e) {
            throw new TccException("cannot read tinyc-native.properties", e);
        }
        return p.getProperty("version", "unknown");
    }

    // ------------------------------------------------------------ 解压

    private static Path extractCombination(Combo combo) throws IOException {
        Path cache = Path.of(System.getProperty("user.home"),
                ".cache", "tinyc-java", packageVersion());
        Path marker = cache.resolve(".ok-" + combo.host() + "-" + combo.target());
        Path binary = cache.resolve(combo.binary());
        Path libtcc1 = cache.resolve(combo.sysroot()).resolve("lib").resolve("tcc")
                .resolve("libtcc1.a");
        if (Files.isRegularFile(marker) && Files.isRegularFile(binary)
                && Files.isRegularFile(libtcc1)) {
            return cache;
        }
        Files.createDirectories(cache);

        URL anchor = Native.class.getResource("/packaging-manifest.json");
        if (anchor == null) {
            throw new TccException("packaging-manifest.json not found on classpath");
        }
        if ("jar".equals(anchor.getProtocol())) {
            JarURLConnection conn = (JarURLConnection) anchor.openConnection();
            try (JarFile jar = conn.getJarFile()) {
                extractFromJar(jar, cache, combo.binary());
                extractFromJar(jar, cache, combo.sysroot() + "/");
            }
        } else if ("file".equals(anchor.getProtocol())) {
            try {
                Path root = Path.of(anchor.toURI()).getParent();
                copyTree(root.resolve(combo.binary()), cache.resolve(combo.binary()));
                copyTree(root.resolve(combo.sysroot()), cache.resolve(combo.sysroot()));
            } catch (java.net.URISyntaxException e) {
                throw new TccException("bad packaging-manifest resource URL: " + anchor, e);
            }
        } else {
            throw new TccException("unsupported packaging-manifest protocol: "
                    + anchor.getProtocol());
        }
        if (!Files.isRegularFile(binary)) {
            throw new TccException("native resource not found: " + combo.binary());
        }
        if (!binary.toFile().setExecutable(true)) {
            throw new TccException("cannot mark native file executable: " + binary);
        }
        Files.writeString(marker, combo.host() + " x " + combo.target());
        return cache;
    }

    /** 把 jar 里以 prefix 开头的条目（或与 prefix 恰好同名的单个条目）解压到 cache。 */
    private static void extractFromJar(JarFile jar, Path cache, String prefix) throws IOException {
        Enumeration<JarEntry> entries = jar.entries();
        while (entries.hasMoreElements()) {
            JarEntry entry = entries.nextElement();
            String name = entry.getName();
            if (entry.isDirectory()) continue;
            if (!name.equals(prefix) && !name.startsWith(prefix)) continue;
            Path target = cache.resolve(name);
            Files.createDirectories(target.getParent());
            try (InputStream in = jar.getInputStream(entry)) {
                Files.copy(in, target, StandardCopyOption.REPLACE_EXISTING);
            }
        }
    }

    private static void copyTree(Path src, Path dst) throws IOException {
        if (Files.isRegularFile(src)) {
            Files.createDirectories(dst.getParent());
            Files.copy(src, dst, StandardCopyOption.REPLACE_EXISTING);
            return;
        }
        try (var walk = Files.walk(src)) {
            for (Path p : walk.filter(Files::isRegularFile).toList()) {
                Path target = dst.resolve(src.relativize(p).toString());
                Files.createDirectories(target.getParent());
                Files.copy(p, target, StandardCopyOption.REPLACE_EXISTING);
            }
        }
    }

    /** 在给定 arena 里分配一个 C 字符串。 */
    static MemorySegment cstr(Arena arena, String s) {
        return arena.allocateFrom(s);
    }

    /** main 入口调用返回后冲刷 C stdio（正常执行链里由 exit 完成的那一步）。 */
    static void flushStdio() {
        if (FFLUSH != null) {
            try {
                FFLUSH.invokeExact(MemorySegment.NULL);
            } catch (Throwable ignored) {
                // 冲刷失败不影响退出码语义
            }
        }
    }
}
