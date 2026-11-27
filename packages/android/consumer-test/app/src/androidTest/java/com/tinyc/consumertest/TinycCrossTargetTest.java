package com.tinyc.consumertest;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import android.content.Context;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import com.tinyc.Tinyc;
import com.tinyc.TinycAssets;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.File;
import java.nio.file.Files;

/**
 * AAR cross-target test: for each of the seven non-android targets,
 * compile a hello program that includes a system header (which
 * exercises the staged sysroot) and check the output file's format
 * magic for that target's machine.
 *
 * <p>Output type per target (the link-time reality on the device):
 * the linux-gnu targets compile to object only, because linking a
 * hosted executable needs the target's runtime libc.so.6 and the
 * device has no glibc runtime; their ELF objects carry the target's
 * e_machine. The darwin and win32 targets link a full executable
 * (darwin via the bundled .tbd stubs, win32 via msvcrt.dll), so
 * their Mach-O / PE magic is checked. wasm32 links a module with
 * -nostdlib (it has no crt objects; its libc is host-provided
 * imports), so its \0asm magic is checked. TCC writes ELF objects
 * for every target (ELF_OBJ_ONLY), so -c cannot tell win32 or
 * darwin apart from the same-CPU linux-gnu target.</p>
 *
 * <p>One target per process: the JNI layer loads exactly one target's
 * .so per process (see TinycNative.ensureLoaded), so each test method
 * runs in its own instrumentation process. This module therefore uses
 * the AndroidX Test Orchestrator (see app/build.gradle); without it
 * the methods would share one process and the second target load
 * would throw IllegalStateException.</p>
 */
@RunWith(AndroidJUnit4.class)
public class TinycCrossTargetTest {

    private Context ctx() {
        return InstrumentationRegistry.getInstrumentation().getTargetContext();
    }

    private static final String HELLO =
            "#include <stdio.h>\n"
            + "int main(void) { printf(\"hi\\n\"); return 0; }\n";

    private byte[] compileHello(String triple, int outputType, String options,
                                String name) throws Exception {
        TinycAssets.Paths paths = TinycAssets.init(ctx(), triple);
        assertTrue(paths.toolchainDir.isDirectory());
        assertTrue(paths.sysrootDir.isDirectory());
        File out = new File(ctx().getCacheDir(), name);
        try (Tinyc t = Tinyc.create(paths)) {
            if (options != null) {
                t.setOptions(options);
            }
            t.setOutputType(outputType);
            t.compileString(HELLO);
            t.outputFile(out.getAbsolutePath());
        }
        byte[] b = Files.readAllBytes(out.toPath());
        assertTrue("empty output for " + triple, b.length > 20);
        return b;
    }

    /** ELF magic + e_machine (little-endian u16 at offset 18). */
    private static void assertElfMachine(byte[] b, int machine, String triple) {
        assertEquals(0x7f, b[0] & 0xff);
        assertEquals('E', b[1]);
        assertEquals('L', b[2]);
        assertEquals('F', b[3]);
        int got = (b[18] & 0xff) | ((b[19] & 0xff) << 8);
        assertEquals("unexpected e_machine for " + triple, machine, got);
    }

    @Test
    public void compileX86_64LinuxGnuHello() throws Exception {
        assertElfMachine(compileHello("x86_64-linux-gnu", Tinyc.OUTPUT_OBJ, null,
                "hello-x86_64-linux-gnu.o"), 62, "x86_64-linux-gnu");
    }

    @Test
    public void compileAarch64LinuxGnuHello() throws Exception {
        assertElfMachine(compileHello("aarch64-linux-gnu", Tinyc.OUTPUT_OBJ, null,
                "hello-aarch64-linux-gnu.o"), 183, "aarch64-linux-gnu");
    }

    @Test
    public void compileRiscv64LinuxGnuHello() throws Exception {
        assertElfMachine(compileHello("riscv64-linux-gnu", Tinyc.OUTPUT_OBJ, null,
                "hello-riscv64-linux-gnu.o"), 243, "riscv64-linux-gnu");
    }

    @Test
    public void compileWin32Hello() throws Exception {
        byte[] b = compileHello("x86_64-w64-mingw32", Tinyc.OUTPUT_EXE, null,
                "hello-win32.exe");
        // PE executable: the "MZ" magic.
        assertEquals(0x4d, b[0] & 0xff);
        assertEquals(0x5a, b[1] & 0xff);
    }

    @Test
    public void compileArm64DarwinHello() throws Exception {
        byte[] b = compileHello("arm64-apple-darwin", Tinyc.OUTPUT_EXE, null,
                "hello-arm64-darwin");
        // Mach-O 64-bit magic 0xfeedfacf, little-endian.
        assertEquals(0xcf, b[0] & 0xff);
        assertEquals(0xfa, b[1] & 0xff);
        assertEquals(0xed, b[2] & 0xff);
        assertEquals(0xfe, b[3] & 0xff);
    }

    @Test
    public void compileX86_64DarwinHello() throws Exception {
        byte[] b = compileHello("x86_64-apple-darwin", Tinyc.OUTPUT_EXE, null,
                "hello-x86_64-darwin");
        assertEquals(0xcf, b[0] & 0xff);
        assertEquals(0xfa, b[1] & 0xff);
        assertEquals(0xed, b[2] & 0xff);
        assertEquals(0xfe, b[3] & 0xff);
    }

    @Test
    public void compileWasm32Hello() throws Exception {
        byte[] b = compileHello("wasm32-wasi", Tinyc.OUTPUT_EXE, "-nostdlib",
                "hello-wasm32.wasm");
        // WebAssembly magic: 0x00 'a' 's' 'm'.
        assertEquals(0x00, b[0] & 0xff);
        assertEquals('a', b[1]);
        assertEquals('s', b[2]);
        assertEquals('m', b[3]);
    }
}
