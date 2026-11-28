/* PCRE2 configuration for the linux-gnu target family.
   Hand-written for the bundled build (see libs-config/README.md);
   the values are the upstream defaults for a Unicode, non-JIT,
   8-bit build against glibc. */

#define PACKAGE "pcre2"
#define PACKAGE_NAME "PCRE2"
#define PACKAGE_TARNAME "pcre2"
#define PACKAGE_STRING "PCRE2 10.45"
#define PACKAGE_VERSION "10.45"
#define VERSION "10.45"
#define PACKAGE_URL "https://www.pcre2project.org/"
#define PACKAGE_BUGREPORT ""

#define HAVE_ASSERT_H 1
#define HAVE_DLFCN_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_LIMITS_H 1
#define HAVE_MEMMOVE 1
#define HAVE_STDINT_H 1
#define HAVE_STDIO_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRERROR 1
#define HAVE_STRINGS_H 1
#define HAVE_STRING_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_UNISTD_H 1
#define HAVE_WCHAR_H 1

#define LINK_SIZE 2
#define PARENS_NEST_LIMIT 250
#define HEAP_LIMIT 20000000
#define MATCH_LIMIT 10000000
#define MATCH_LIMIT_DEPTH MATCH_LIMIT
#define MAX_NAME_SIZE 32
#define MAX_NAME_COUNT 10000
#define MAX_VARLOOKBEHIND 255
#define NEWLINE_DEFAULT 2

#define SUPPORT_UNICODE 1

/* Static build: no export decoration. */
#define PCRE2_EXPORT
