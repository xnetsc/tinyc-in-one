/* Functional battery for the bundled Darwin headers
 * (vendor-darwin/).  It is compiled twice by
 * run_macos_sdkfree.sh: once with clang and the platform SDK
 * headers (proves the battery itself is sound C), and once with
 * the packaged tinyc using only the bundled headers while the
 * SDK is hidden (proves the bundled set works end to end:
 * compile, link against the bundled .tbd stubs, run against the
 * system libSystem).
 *
 * Every section prints "ok <name>" or "FAIL <name>"; the program
 * exits with the number of failed sections and prints
 * "BATTERY OK" when all pass.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fenv.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <setjmp.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdatomic.h>
#include <pthread.h>

static int fails;

#define SECTION(name, cond) do { \
    if (cond) { printf("ok %s\n", name); } \
    else { printf("FAIL %s\n", name); fails++; } \
} while (0)

static int cmp_int(const void *a, const void *b) {
    int x = *(const int *)a, y = *(const int *)b;
    return (x > y) - (x < y);
}

static _Atomic int shared_atomic;
static int shared_plain;
static pthread_mutex_t plain_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t once_ctrl = PTHREAD_ONCE_INIT;
static int once_ran;

static void run_once(void) { once_ran++; }

static void *worker(void *arg) {
    (void)arg;
    int i;
    pthread_once(&once_ctrl, run_once);
    for (i = 0; i < 25000; i++) {
        atomic_fetch_add(&shared_atomic, 1);
        pthread_mutex_lock(&plain_lock);
        shared_plain++;
        pthread_mutex_unlock(&plain_lock);
    }
    return NULL;
}

static volatile sig_atomic_t got_usr1;
static void on_usr1(int sig) { (void)sig; got_usr1 = 1; }

static jmp_buf jb;
static void jump_away(void) { longjmp(jb, 7); }

int main(void) {
    /* stdio */
    {
        char buf[64];
        int n = snprintf(buf, sizeof buf, "%d-%s", 42, "tinyc");
        FILE *f;
        char back[16] = {0};
        SECTION("stdio.snprintf", n == 8 && strcmp(buf, "42-tinyc") == 0);
        f = tmpfile();
        if (f) {
            fwrite("hello", 1, 5, f);
            rewind(f);
            SECTION("stdio.tmpfile", fread(back, 1, 5, f) == 5 &&
                    strcmp(back, "hello") == 0);
            fclose(f);
        } else SECTION("stdio.tmpfile", 0);
    }
    /* stdlib */
    {
        int v[5] = {5, 3, 4, 1, 2};
        char *end = NULL;
        long lv;
        char tmpl[] = "/tmp/tinyc-sdkfree-XXXXXX";
        int fd;
        qsort(v, 5, sizeof(int), cmp_int);
        lv = strtol("12345", &end, 10);
        fd = mkstemp(tmpl);
        SECTION("stdlib.qsort", v[0] == 1 && v[4] == 5);
        SECTION("stdlib.strtol", lv == 12345 && end && *end == '\0');
        SECTION("stdlib.getenv", getenv("PATH") != NULL);
        SECTION("stdlib.mkstemp", fd >= 0);
        if (fd >= 0) {
            struct stat st;
            write(fd, "abc", 3);
            SECTION("stat.file", fstat(fd, &st) == 0 && S_ISREG(st.st_mode) &&
                    st.st_size == 3 && st.st_nlink >= 1 &&
                    stat(tmpl, &st) == 0 && st.st_size == 3);
            SECTION("stat.lstat", lstat(tmpl, &st) == 0 && S_ISREG(st.st_mode) &&
                    st.st_size == 3);
            SECTION("stat.fstatat", fstatat(AT_FDCWD, tmpl, &st, 0) == 0 &&
                    S_ISREG(st.st_mode) && st.st_size == 3);
            close(fd);
            unlink(tmpl);
        } else SECTION("stat.file", 0);
    }
    /* string */
    {
        char buf[32] = "abc";
        char *dup;
        strcat(buf, "def");
        dup = strdup(buf);
        SECTION("string.basics", strcmp(buf, "abcdef") == 0 &&
                strchr(buf, 'e') == buf + 4 &&
                strstr(buf, "cd") == buf + 2 && strlen(buf) == 6);
        SECTION("string.strdup", dup && strcmp(dup, "abcdef") == 0);
        free(dup);
        SECTION("string.strlcpy", strlcpy(buf, "xy", sizeof buf) == 2 &&
                strcmp(buf, "xy") == 0);
    }
    /* math */
    {
        double s = sin(0.0), c = cos(0.0), r = sqrt(144.0), p = pow(2.0, 10.0);
        double e = exp(1.0), l = log(e);
        volatile double zero = 0.0;
        SECTION("math.values", s == 0.0 && c == 1.0 && r == 12.0 &&
                p == 1024.0 && fabs(-3.5) == 3.5 &&
                floor(2.7) == 2.0 && ceil(2.1) == 3.0 &&
                fabs(l - 1.0) < 1e-9);
        SECTION("math.classify", isnan(zero / zero) && isinf(1.0 / zero) &&
                isfinite(1.5) && fpclassify(1.5) == FP_NORMAL);
    }
    /* fenv: the rounding mode is observable through rint on a
     * volatile operand (a plain constant could be folded at
     * the default rounding mode), and the exception flags
     * through raise/test. */
    {
        volatile double v = 1.5;
        int down_ok, up_ok, exc_ok;
        feclearexcept(FE_ALL_EXCEPT);
        feraiseexcept(FE_OVERFLOW);
        exc_ok = fetestexcept(FE_OVERFLOW) != 0;
        feclearexcept(FE_ALL_EXCEPT);
        fesetround(FE_DOWNWARD);
        down_ok = fegetround() == FE_DOWNWARD && rint(v) == 1.0;
        fesetround(FE_UPWARD);
        up_ok = fegetround() == FE_UPWARD && rint(v) == 2.0;
        fesetround(FE_TONEAREST);
        SECTION("fenv.round", down_ok && up_ok &&
                fegetround() == FE_TONEAREST && exc_ok);
    }
    /* pthread + stdatomic */
    {
        pthread_t th[4];
        int i, made = 0;
        for (i = 0; i < 4; i++)
            if (pthread_create(&th[i], NULL, worker, NULL) == 0) made++;
        for (i = 0; i < made; i++) pthread_join(th[i], NULL);
        SECTION("pthread.atomic", made == 4 &&
                atomic_load(&shared_atomic) == 100000 &&
                shared_plain == 100000 && once_ran == 1);
    }
    /* time */
    {
        time_t t0 = (time_t)0;
        struct tm tmv;
        struct timespec ts;
        SECTION("time.now", time(NULL) > 0);
        SECTION("time.gmtime", gmtime_r(&t0, &tmv) == &tmv &&
                tmv.tm_year == 70 && tmv.tm_mon == 0 && tmv.tm_mday == 1 &&
                tmv.tm_wday == 4);
        SECTION("time.clock", clock_gettime(CLOCK_MONOTONIC, &ts) == 0 &&
                ts.tv_sec >= 0 && ts.tv_nsec >= 0 && ts.tv_nsec < 1000000000L);
    }
    /* dirent */
    {
        const char *dir = "/tmp/tinyc-sdkfree-dir";
        char path[128];
        DIR *d;
        struct dirent *de;
        int found = 0, fd;
        mkdir(dir, 0755);
        snprintf(path, sizeof path, "%s/probe.txt", dir);
        fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (fd >= 0) close(fd);
        d = opendir(dir);
        if (d) {
            while ((de = readdir(d)) != NULL)
                if (strcmp(de->d_name, "probe.txt") == 0) found = 1;
            closedir(d);
        }
        SECTION("dirent.scan", found);
        /* The same directory through the other $INODE64 entry
         * points: a wrong suffix on any of these is a missing
         * libSystem symbol, so the battery dies at the call -
         * that is the tripwire for the marking. */
        {
            int dfd = open(dir, O_RDONLY);
            DIR *fdd = dfd >= 0 ? fdopendir(dfd) : NULL;
            struct dirent entry, *result = NULL;
            int ffound = 0;
            if (fdd) {
                while (readdir_r(fdd, &entry, &result) == 0 && result != NULL)
                    if (strcmp(result->d_name, "probe.txt") == 0) ffound = 1;
                closedir(fdd);
            }
            SECTION("dirent.fdopendir", ffound);
        }
        /* No dirent.getdirentries section: the function is not
         * callable under 64-bit inodes -- the SDK redirects it to
         * _getdirentries_is_not_available_when_64_bit_inodes_are_in_effect,
         * and there is no $INODE64 variant either.  It is not tested
         * here, on either leg. */
        {
            struct dirent **list = NULL;
            int n = scandir(dir, &list, NULL, alphasort);
            int sfound = 0, i;
            for (i = 0; i < n; i++) {
                if (strcmp(list[i]->d_name, "probe.txt") == 0) sfound = 1;
                free(list[i]);
            }
            free(list);
            SECTION("dirent.scandir", n >= 3 && sfound);
        }
        unlink(path);
        rmdir(dir);
    }
    /* mmap */
    {
        char *m = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANON, -1, 0);
        SECTION("mmap.anon", m != MAP_FAILED && (m[0] = 7, m[4095] = 9,
                m[0] + m[4095] == 16) && munmap(m, 4096) == 0);
    }
    /* signal + setjmp + errno */
    {
        int v;
        signal(SIGUSR1, on_usr1);
        raise(SIGUSR1);
        SECTION("signal.raise", got_usr1 == 1);
        v = setjmp(jb);
        if (v == 0) { jump_away(); SECTION("setjmp.longjmp", 0); }
        else SECTION("setjmp.longjmp", v == 7);
        errno = 0;
        SECTION("errno.value", open("/nonexistent-tinyc-probe", O_RDONLY) < 0 &&
                errno == ENOENT);
    }
    if (fails == 0) printf("BATTERY OK\n");
    return fails;
}
