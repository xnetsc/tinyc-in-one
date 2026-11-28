/* Cross-host differential probe: predefined macros. The
 * presence bitmask and the stringized values fingerprint the
 * compiler's predefined macro surface for the target, which
 * is fixed by the compiler's build configuration (tccdefs)
 * and target selection -- a place where a compiler built by
 * a different host toolchain can silently diverge.
 * __DATE__/__TIME__ are deliberately absent: they are defined
 * to expand to the compilation date and time, so no two
 * compilations can ever agree on them; __FILE__ is checked
 * separately by the harness because it expands to the input
 * path, an input to the compilation rather than a property
 * of the compiler. */
#define STR_(x) #x
#define STR(x) STR_(x)

#ifdef __STDC__
#define HAS___STDC__ 1
#else
#define HAS___STDC__ 0
#endif
#ifdef __STDC_HOSTED__
#define HAS___STDC_HOSTED__ 1
#else
#define HAS___STDC_HOSTED__ 0
#endif
#ifdef __TINYC__
#define HAS___TINYC__ 1
#else
#define HAS___TINYC__ 0
#endif
#ifdef __GNUC__
#define HAS___GNUC__ 1
#else
#define HAS___GNUC__ 0
#endif
#ifdef __x86_64__
#define HAS___x86_64__ 1
#else
#define HAS___x86_64__ 0
#endif
#ifdef __aarch64__
#define HAS___aarch64__ 1
#else
#define HAS___aarch64__ 0
#endif
#ifdef __riscv64
#define HAS___riscv64 1
#else
#define HAS___riscv64 0
#endif
#ifdef __linux__
#define HAS___linux__ 1
#else
#define HAS___linux__ 0
#endif
#ifdef __ELF__
#define HAS___ELF__ 1
#else
#define HAS___ELF__ 0
#endif
#ifdef _WIN32
#define HAS___WIN32 1
#else
#define HAS___WIN32 0
#endif
#ifdef __APPLE__
#define HAS___APPLE__ 1
#else
#define HAS___APPLE__ 0
#endif
#ifdef __MACH__
#define HAS___MACH__ 1
#else
#define HAS___MACH__ 0
#endif
#ifdef __ANDROID__
#define HAS___ANDROID__ 1
#else
#define HAS___ANDROID__ 0
#endif
#ifdef __BYTE_ORDER__
#define HAS___BYTE_ORDER__ 1
#else
#define HAS___BYTE_ORDER__ 0
#endif
#ifdef __SIZEOF_POINTER__
#define HAS___SIZEOF_POINTER__ 1
#else
#define HAS___SIZEOF_POINTER__ 0
#endif
#ifdef __wasm__
#define HAS___wasm__ 1
#else
#define HAS___wasm__ 0
#endif
#ifdef TCC_TARGET_WASM32
#define HAS_TCC_TARGET_WASM32 1
#else
#define HAS_TCC_TARGET_WASM32 0
#endif

static const unsigned long presence[] = {
    HAS___STDC__, HAS___STDC_HOSTED__, HAS___TINYC__, HAS___GNUC__,
    HAS___x86_64__, HAS___aarch64__, HAS___riscv64, HAS___linux__,
    HAS___ELF__, HAS___WIN32, HAS___APPLE__, HAS___MACH__,
    HAS___ANDROID__, HAS___BYTE_ORDER__, HAS___SIZEOF_POINTER__,
    HAS___wasm__, HAS_TCC_TARGET_WASM32
};

#ifdef __STDC_VERSION__
static const long stdc_version = __STDC_VERSION__;
#else
static const long stdc_version = -1;
#endif

#ifdef __BYTE_ORDER__
static const long byte_order_val = __BYTE_ORDER__;
#else
static const long byte_order_val = -1;
#endif

#ifdef __ORDER_LITTLE_ENDIAN__
static const long order_le = __ORDER_LITTLE_ENDIAN__;
#else
static const long order_le = -1;
#endif

#ifdef __ORDER_BIG_ENDIAN__
static const long order_be = __ORDER_BIG_ENDIAN__;
#else
static const long order_be = -1;
#endif

static const char version_str[] =
#ifdef __VERSION__
    STR(__VERSION__);
#else
    "<no __VERSION__>";
#endif

static const char tinyc_str[] =
#ifdef __TINYC__
    STR(__TINYC__);
#else
    "<no __TINYC__>";
#endif

unsigned long probe_macros_anchor(void)
{
    return presence[0] + (unsigned long)stdc_version +
           (unsigned long)byte_order_val + version_str[0] + tinyc_str[0];
}
