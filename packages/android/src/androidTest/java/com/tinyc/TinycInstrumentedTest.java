package com.tinyc;

import android.content.Context;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.nio.charset.StandardCharsets;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

/**
 * 设备侧测试：需要真机或模拟器（./gradlew connectedAndroidTest），
 * 且 jniLibs 里要有真实的 libtinyc-&lt;target&gt;.so。本机（CI 无设备）不运行。
 */
@RunWith(AndroidJUnit4.class)
public class TinycInstrumentedTest {

    private Context ctx() {
        return InstrumentationRegistry.getInstrumentation().getTargetContext();
    }

    @Test
    public void assetsExtractToFilesDir() throws Exception {
        TinycAssets.Paths paths = TinycAssets.init(ctx());
        assertTrue(paths.toolchainDir.isDirectory());
        assertTrue(paths.sysrootDir.isDirectory());
        // 第二次 init 走标记复用路径，结果一致
        TinycAssets.Paths again = TinycAssets.init(ctx());
        assertEquals(paths.rootDir, again.rootDir);
    }

    @Test
    public void compileStringToObject() throws Exception {
        TinycAssets.Paths paths = TinycAssets.init(ctx());
        File out = new File(ctx().getFilesDir(), "answer.o");
        try (Tinyc t = Tinyc.create(paths)) {
            t.setOutputType(Tinyc.OUTPUT_OBJ);
            t.compileString("int answer(void) { return 42; }");
            t.outputFile(out.getAbsolutePath());
        }
        assertTrue(out.isFile() && out.length() > 0);
    }

    @Test
    public void runCliVersion() throws Exception {
        TinycAssets.init(ctx());
        int rc = Tinyc.runCli("--version");
        assertEquals(0, rc);
    }

    @Test
    public void ioCaptureReceivesOutput() throws Exception {
        TinycAssets.Paths paths = TinycAssets.init(ctx());
        ByteArrayOutputStream captured = new ByteArrayOutputStream();
        TinycIO io = TinycIO.of(captured, captured, null);
        try (Tinyc t = Tinyc.create(paths)) {
            t.setIO(io);
            t.setOutputType(Tinyc.OUTPUT_PREPROCESS);
            // 预处理输出走 write_out（规格 6.2），应被 capture 收到
            t.compileString("int x = 1;");
        }
        String text = captured.toString(StandardCharsets.UTF_8.name());
        assertTrue(text.contains("int x"));
    }

    @Test
    public void compileErrorThrows() throws Exception {
        TinycAssets.Paths paths = TinycAssets.init(ctx());
        boolean threw = false;
        try (Tinyc t = Tinyc.create(paths)) {
            t.setOutputType(Tinyc.OUTPUT_OBJ);
            t.compileString("int broken( {");
        } catch (TinycException e) {
            threw = true;
        }
        assertTrue(threw);
    }
}
