/* libcurl configuration for the darwin target family.
   Hand-written for the bundled build (see libs-config/README.md).

   Same feature set as the linux-gnu family: HTTP, HTTPS, WS and
   WSS only, TLS via the bundled mbedTLS, NTLM disabled (mbedTLS
   has no MD4), no compression libraries, no IDN, no PSL. The
   HAVE_ set describes the bundled hand-written darwin header
   set, which is deliberately small: there is no poll.h (select
   is the fallback), no sys/un.h (no Unix sockets), no pwd.h, no
   ioctl declaration and no gethostbyname_r on macOS. MSG_NOSIGNAL
   does not exist on darwin; the socket code uses SO_NOSIGPIPE
   from the bundled sys/socket.h instead. */

#define CURL_OS "darwin"
#define PACKAGE "curl"
#define PACKAGE_NAME "curl"
#define PACKAGE_TARNAME "curl"
#define PACKAGE_STRING "curl 8.11.1"
#define PACKAGE_VERSION "8.11.1"
#define VERSION "8.11.1"
#define PACKAGE_URL "https://curl.se/"
#define PACKAGE_BUGREPORT "a suitable curl mailing list => https://curl.se/mail/"

#define STDC_HEADERS 1
#define OS "darwin"

/* Protocols: HTTP family only. */
#define CURL_DISABLE_DICT 1
#define CURL_DISABLE_FILE 1
#define CURL_DISABLE_FTP 1
#define CURL_DISABLE_GOPHER 1
#define CURL_DISABLE_IMAP 1
#define CURL_DISABLE_LDAP 1
#define CURL_DISABLE_LDAPS 1
#define CURL_DISABLE_MQTT 1
#define CURL_DISABLE_POP3 1
#define CURL_DISABLE_RTSP 1
#define CURL_DISABLE_SMB 1
#define CURL_DISABLE_SMTP 1
#define CURL_DISABLE_TELNET 1
#define CURL_DISABLE_TFTP 1
#define CURL_DISABLE_NTLM 1

/* TLS backend: bundled mbedTLS. */
#define USE_MBEDTLS 1

#define USE_IPV6 1
#define USE_THREADS_POSIX 1
#define CURL_SA_FAMILY_T sa_family_t

/* Type sizes: every darwin target here is LP64. */
#define SIZEOF_INT 4
#define SIZEOF_LONG 8
#define SIZEOF_OFF_T 8
#define SIZEOF_SIZE_T 8
#define SIZEOF_TIME_T 8
#define SIZEOF_CURL_OFF_T 8
#define SIZEOF_CURL_SOCKET_T 4

/* Headers (the bundled darwin set only). */
#define HAVE_ARPA_INET_H 1
#define HAVE_ATOMIC 1
#define HAVE_BOOL_T 1
#define HAVE_DIRENT_H 1
#define HAVE_DLFCN_H 1
#define HAVE_FCNTL_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_LIBGEN_H 1
#define HAVE_LOCALE_H 1
#define HAVE_NETDB_H 1
#define HAVE_NETINET_IN_H 1
#define HAVE_NETINET_TCP_H 1
#define HAVE_SETJMP_H 1
#define HAVE_STDATOMIC_H 1
#define HAVE_STDBOOL_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDIO_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRINGS_H 1
#define HAVE_STRING_H 1
#define HAVE_SYS_RESOURCE_H 1
#define HAVE_SYS_SOCKET_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_WAIT_H 1
#define HAVE_UNISTD_H 1
#define HAVE_UTIME_H 1

/* Functions and declarations. */
#define HAVE_ALARM 1
#define HAVE_ARC4RANDOM 1
#define HAVE_BASENAME 1
#define HAVE_CLOCK_GETTIME_MONOTONIC 1
#define HAVE_DECL_FSEEKO 1
#define HAVE_FCNTL 1
#define HAVE_FCNTL_O_NONBLOCK 1
#define HAVE_FREEADDRINFO 1
#define HAVE_FSEEKO 1
#define HAVE_FTRUNCATE 1
#define HAVE_GETADDRINFO 1
#define HAVE_GETADDRINFO_THREADSAFE 1
#define HAVE_GETEUID 1
#define HAVE_GETHOSTBYNAME 1
#define HAVE_GETHOSTNAME 1
#define HAVE_GETPEERNAME 1
#define HAVE_GETPPID 1
#define HAVE_GETRLIMIT 1
#define HAVE_GETSOCKNAME 1
#define HAVE_GETTIMEOFDAY 1
#define HAVE_GMTIME_R 1
#define HAVE_INET_NTOP 1
#define HAVE_INET_PTON 1
#define HAVE_LONGLONG 1
#define HAVE_OPENDIR 1
#define HAVE_PIPE 1
#define HAVE_POSIX_STRERROR_R 1
#define HAVE_PTHREAD_H 1
#define HAVE_RECV 1
#define HAVE_SCHED_YIELD 1
#define HAVE_SELECT 1
#define HAVE_SEND 1
#define HAVE_SENDMSG 1
#define HAVE_SETLOCALE 1
#define HAVE_SETRLIMIT 1
#define HAVE_SIGACTION 1
#define HAVE_SIGNAL 1
#define HAVE_SIGSETJMP 1
#define HAVE_SNPRINTF 1
#define HAVE_SOCKADDR_IN6_SIN6_SCOPE_ID 1
#define HAVE_SOCKET 1
#define HAVE_SOCKETPAIR 1
#define HAVE_STRCASECMP 1
#define HAVE_STRDUP 1
#define HAVE_STRERROR_R 1
#define HAVE_STRTOK_R 1
#define HAVE_STRTOLL 1
#define HAVE_STRUCT_SOCKADDR_STORAGE 1
#define HAVE_STRUCT_TIMEVAL 1
#define HAVE_SUSECONDS_T 1
#define HAVE_UTIME 1
#define HAVE_UTIMES 1

#define GETHOSTNAME_TYPE_ARG2 size_t
#define LT_OBJDIR ".libs/"
