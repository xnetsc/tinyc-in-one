package com.tinyc.consumertest;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import android.content.Context;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import com.tinyc.Tinyc;
import com.tinyc.TinycAssets;
import com.tinyc.TinycIO;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.File;
import java.nio.file.Files;

/**
 * AAR consumer test: the AAR is a plain file dependency of this app;
 * every call goes through its public Java API. Covers loading the
 * bundled .so (any Tinyc call does), JNI compilation to an object
 * file, in-memory relocate + run, the CLI entry, and IO capture.
 */
@RunWith(AndroidJUnit4.class)
public class TinycConsumerTest {

    private Context ctx() {
        return InstrumentationRegistry.getInstrumentation().getTargetContext();
    }

    @Test
    public void compilesToObjectWithDeviceMachine() throws Exception {
        TinycAssets.Paths paths = TinycAssets.init(ctx());
        assertTrue(paths.toolchainDir.isDirectory());
        File obj = new File(ctx().getCacheDir(), "answer.o");
        try (Tinyc t = Tinyc.create(paths)) {
            t.setOutputType(Tinyc.OUTPUT_OBJ);
            t.compileString("int answer(void) { return 42; }");
            t.outputFile(obj.getAbsolutePath());
        }
        byte[] b = Files.readAllBytes(obj.toPath());
        assertTrue(b.length > 20);
        assertEquals(0x7f, b[0] & 0xff);
        assertEquals('E', b[1]);
        assertEquals('L', b[2]);
        assertEquals('F', b[3]);
        int machine = (b[18] & 0xff) | ((b[19] & 0xff) << 8);
        // 62 = x86_64 (emulator), 183 = aarch64 (arm64 device)
        assertTrue("unexpected e_machine " + machine, machine == 62 || machine == 183);
    }

    @Test
    public void runInMemoryReturnsProgramResult() throws Exception {
        TinycAssets.Paths paths = TinycAssets.init(ctx());
        try (Tinyc t = Tinyc.create(paths)) {
            t.setOutputType(Tinyc.OUTPUT_MEMORY);
            t.compileString("int main(void) { return 42; }");
            // No relocate() first: tcc_run loads runmain.o and
            // relocates itself, and a state already relocated by
            // tcc_relocate no longer has the sections that load
            // needs (the kernel now refuses the composition with a
            // clean error instead of crashing).
            assertEquals(42, t.run("prog"));
        }
    }

    @Test
    public void runCliVersionSucceeds() throws Exception {
        TinycAssets.init(ctx());
        assertEquals(0, Tinyc.runCli("--version"));
    }

    @Test
    public void ioCaptureReceivesPreprocessorOutput() throws Exception {
        TinycAssets.Paths paths = TinycAssets.init(ctx());
        java.io.ByteArrayOutputStream captured = new java.io.ByteArrayOutputStream();
        try (Tinyc t = Tinyc.create(paths)) {
            t.setIO(TinycIO.of(captured, captured, null));
            t.setOutputType(Tinyc.OUTPUT_PREPROCESS);
            t.compileString("int x = 1;");
        }
        String text = captured.toString(java.nio.charset.StandardCharsets.UTF_8.name());
        assertTrue(text.contains("int x"));
    }
}
