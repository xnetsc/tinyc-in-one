/* libcurl configuration for the windows target family.
   Hand-written for the bundled build (see libs-config/README.md),
   modeled on upstream's hand-crafted lib/config-win32.h and
   restricted to what the bundled win32 header set declares.

   Same feature set as the other families: HTTP, HTTPS, WS and
   WSS only, TLS via the bundled mbedTLS, NTLM disabled (mbedTLS
   has no MD4), no compression libraries, no IDN, no PSL.
   Family notes: the resolver is the Windows threaded resolver
   (USE_THREADS_WIN32, upstream's Windows default); there is no
   USE_UNIX_SOCKETS because the bundled winsock headers have no
   sockaddr_un; inet_ntop/inet_pton are not declared by the
   bundled ws2tcpip.h, so libcurl compiles its own; gettimeofday
   is not claimed (the bundled header maps it to a MinGW runtime
   helper the bundled import set does not provide, and libcurl
   has a native Windows implementation); USE_WIN32_CRYPTO is
   off because the bundled headers have no wincrypt.h (TLS
   entropy comes from mbedTLS's BCrypt source, not from
   libcurl's random). */

#define CURL_OS "x86_64-pc-win32"
#define PACKAGE "curl"
#define PACKAGE_NAME "curl"
#define PACKAGE_TARNAME "curl"
#define PACKAGE_STRING "curl 8.11.1"
#define PACKAGE_VERSION "8.11.1"
#define VERSION "8.11.1"
#define PACKAGE_URL "https://curl.se/"
#define PACKAGE_BUGREPORT "a suitable curl mailing list => https://curl.se/mail/"

#define STDC_HEADERS 1
#define OS "win32"

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
#define USE_THREADS_WIN32 1
#define NEED_MALLOC_H 1

/* Type sizes: Windows is LLP64; the bundled headers typedef
   off_t to long. */
#define SIZEOF_INT 4
#define SIZEOF_LONG 4
#define SIZEOF_OFF_T 4
#define SIZEOF_SIZE_T 8
#define SIZEOF_TIME_T 8
#define SIZEOF_CURL_OFF_T 8
#define SIZEOF_CURL_SOCKET_T 8

/* Headers (the bundled win32 set only). */
#define HAVE_BOOL_T 1
#define HAVE_FCNTL_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_IO_H 1
#define HAVE_LOCALE_H 1
#define HAVE_STDBOOL_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDIO_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_UTIME_H 1
#define HAVE_UNISTD_H 1

/* Functions and declarations. */
#define HAVE_CLOSESOCKET 1
#define HAVE_FREEADDRINFO 1
#define HAVE_GETADDRINFO 1
#define HAVE_GETADDRINFO_THREADSAFE 1
#define HAVE_GETHOSTNAME 1
#define HAVE_GETPEERNAME 1
#define HAVE_GETSOCKNAME 1
#define HAVE_IOCTLSOCKET 1
#define HAVE_IOCTLSOCKET_FIONBIO 1
#define HAVE_LONGLONG 1
#define HAVE_RECV 1
#define HAVE_SELECT 1
#define HAVE_SEND 1
#define HAVE_SETLOCALE 1
#define HAVE_SETMODE 1
#define HAVE__SETMODE 1
#define HAVE_SIGNAL 1
#define HAVE_SNPRINTF 1
#define HAVE_SOCKET 1
#define HAVE_SOCKADDR_IN6_SIN6_SCOPE_ID 1
#define HAVE_STRCASECMP 1
#define HAVE_STRDUP 1
#define HAVE_STRICMP 1
#define HAVE_STRTOLL 1
#define HAVE_STRUCT_SOCKADDR_STORAGE 1
#define HAVE_STRUCT_TIMEVAL 1
#define HAVE_UTIME 1

/* recv/send signatures on winsock. */
#define RECV_TYPE_ARG1 SOCKET
#define RECV_TYPE_ARG2 char *
#define RECV_TYPE_ARG3 int
#define RECV_TYPE_ARG4 int
#define RECV_TYPE_RETV int
#define SEND_TYPE_ARG1 SOCKET
#define SEND_QUAL_ARG2 const
#define SEND_TYPE_ARG2 char *
#define SEND_TYPE_ARG3 int
#define SEND_TYPE_ARG4 int
#define SEND_TYPE_RETV int

/* Typedef replacements the bundled headers do not provide. */
#define in_addr_t unsigned long
#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
#define ssize_t __int64
#endif

#define LT_OBJDIR ".libs/"
