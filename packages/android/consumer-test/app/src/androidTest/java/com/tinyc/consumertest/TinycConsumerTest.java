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

    /**
     * The bundled header set's networking and threading surface,
     * compiled from the AAR's own sysroot and run in memory:
     * a pthread counter, a socket loopback exchange, and netdb
     * lookups (numeric and the localhost name). The program
     * returns the number of the first failing section, 0 when
     * all pass.
     */
    @Test
    public void networkingAndThreadingRun() throws Exception {
        TinycAssets.Paths paths = TinycAssets.init(ctx());
        String prog = String.join("\n",
            "#include <string.h>",
            "#include <pthread.h>",
            "#include <unistd.h>",
            "#include <sys/socket.h>",
            "#include <netinet/in.h>",
            "#include <arpa/inet.h>",
            "#include <netdb.h>",
            "static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;",
            "static int counter;",
            "static void *worker(void *arg) {",
            "    int i; (void)arg;",
            "    for (i = 0; i < 25000; i++) {",
            "        pthread_mutex_lock(&lock);",
            "        counter++;",
            "        pthread_mutex_unlock(&lock);",
            "    }",
            "    return 0;",
            "}",
            "int main(void) {",
            "    pthread_t th[4]; int i, made = 0;",
            "    for (i = 0; i < 4; i++)",
            "        if (pthread_create(&th[i], 0, worker, 0) == 0) made++;",
            "    for (i = 0; i < made; i++) pthread_join(th[i], 0);",
            "    if (made != 4 || counter != 100000) return 1;",
            "    {",
            "        int ls = socket(AF_INET, SOCK_STREAM, 0);",
            "        struct sockaddr_in a; socklen_t alen = sizeof a;",
            "        int ok = 0;",
            "        memset(&a, 0, sizeof a);",
            "        a.sin_family = AF_INET;",
            "        a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);",
            "        a.sin_port = 0;",
            "        if (ls >= 0 && bind(ls, (struct sockaddr *)&a, sizeof a) == 0",
            "            && listen(ls, 4) == 0",
            "            && getsockname(ls, (struct sockaddr *)&a, &alen) == 0) {",
            "            int cs = socket(AF_INET, SOCK_STREAM, 0);",
            "            if (cs >= 0 && connect(cs, (struct sockaddr *)&a, sizeof a) == 0) {",
            "                int as = accept(ls, 0, 0);",
            "                if (as >= 0) {",
            "                    char b[8] = {0};",
            "                    ok = send(cs, \"ping\", 4, 0) == 4",
            "                        && recv(as, b, 4, MSG_WAITALL) == 4",
            "                        && memcmp(b, \"ping\", 4) == 0",
            "                        && send(as, \"pong\", 4, 0) == 4",
            "                        && recv(cs, b, 4, MSG_WAITALL) == 4",
            "                        && memcmp(b, \"pong\", 4) == 0;",
            "                    close(as);",
            "                }",
            "                close(cs);",
            "            }",
            "            close(ls);",
            "        }",
            "        if (!ok) return 2;",
            "    }",
            "    {",
            "        struct addrinfo hints, *res = 0, *p;",
            "        int ok_num = 0, ok_name = 0;",
            "        memset(&hints, 0, sizeof hints);",
            "        hints.ai_family = AF_INET;",
            "        hints.ai_socktype = SOCK_STREAM;",
            "        if (getaddrinfo(\"127.0.0.1\", \"80\", &hints, &res) == 0 && res) {",
            "            struct sockaddr_in *sin = (struct sockaddr_in *)res->ai_addr;",
            "            ok_num = res->ai_family == AF_INET",
            "                && sin->sin_addr.s_addr == htonl(INADDR_LOOPBACK)",
            "                && sin->sin_port == htons(80);",
            "            freeaddrinfo(res); res = 0;",
            "        }",
            "        if (!ok_num) return 3;",
            "        if (getaddrinfo(\"localhost\", 0, &hints, &res) == 0 && res) {",
            "            for (p = res; p; p = p->ai_next) {",
            "                struct sockaddr_in *sin = (struct sockaddr_in *)p->ai_addr;",
            "                if (p->ai_family == AF_INET",
            "                    && sin->sin_addr.s_addr == htonl(INADDR_LOOPBACK))",
            "                    ok_name = 1;",
            "            }",
            "            freeaddrinfo(res); res = 0;",
            "        }",
            "        if (!ok_name) return 4;",
            "    }",
            "    return 0;",
            "}");
        try (Tinyc t = Tinyc.create(paths)) {
            t.setOutputType(Tinyc.OUTPUT_MEMORY);
            t.compileString(prog);
            assertEquals(0, t.run("prog"));
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
