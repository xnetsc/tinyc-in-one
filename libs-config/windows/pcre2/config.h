/* PCRE2 configuration for the windows target family.
   Hand-written for the bundled build (see libs-config/README.md);
   the values are the upstream defaults for a Unicode, non-JIT,
   8-bit build. The staged pcre2.h for this family predefines
   PCRE2_STATIC (the bundled library is static-only). */

#define PACKAGE "pcre2"
#define PACKAGE_NAME "PCRE2"
#define PACKAGE_TARNAME "pcre2"
#define PACKAGE_STRING "PCRE2 10.45"
#define PACKAGE_VERSION "10.45"
#define VERSION "10.45"
#define PACKAGE_URL "https://www.pcre2project.org/"
#define PACKAGE_BUGREPORT ""

#define PCRE2_EXPORT

#define SUPPORT_UNICODE 1
#define SUPPORT_PCRE2_8 1
#define LINK_SIZE 2
#define PARENS_NEST_LIMIT 250
#define MATCH_LIMIT 10000000
#define MATCH_LIMIT_DEPTH 10000000
#define HEAP_LIMIT 20000000
#define NEWLINE_DEFAULT 2
#define MAX_NAME_SIZE 128
#define MAX_NAME_COUNT 10000
#define MAX_VARLOOKBEHIND 255

#define HAVE_INTTYPES_H 1
#define HAVE_LIMITS_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDIO_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_UNISTD_H 1
#define HAVE_WINDOWS_H 1
#define HAVE_MEMMOVE 1
#define HAVE_STRERROR 1
#define STDC_HEADERS 1
