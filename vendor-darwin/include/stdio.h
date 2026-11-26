/* stdio.h - standard input/output, Darwin.
 *
 * Written for this package from the C99 <stdio.h> requirements
 * plus the POSIX stream functions libSystem provides.  Darwin
 * facts: FILE is fully opaque, the standard streams are the
 * exported variables __stdinp/__stdoutp/__stderrp behind the
 * stdin/stdout/stderr macros, fpos_t is a 64-bit file offset, and
 * BUFSIZ is 1024.
 */

#ifndef _TINYC_DARWIN_STDIO_H
#define _TINYC_DARWIN_STDIO_H

#include <stddef.h>
#include <stdarg.h>
#include <sys/types.h>

typedef struct __sFILE FILE;
typedef off_t fpos_t;

extern FILE *__stdinp;
extern FILE *__stdoutp;
extern FILE *__stderrp;
#define stdin	__stdinp
#define stdout	__stdoutp
#define stderr	__stderrp

#define EOF		(-1)
#define BUFSIZ		1024
#define FOPEN_MAX	20
#define FILENAME_MAX	1024
#define L_tmpnam	1024
#define TMP_MAX		308915776

#define _IOFBF		0
#define _IOLBF		1
#define _IONBF		2

#ifndef SEEK_SET
#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2
#endif

/* Operations on files */
int remove(const char *path);
int rename(const char *oldname, const char *newname);
FILE *tmpfile(void);
char *tmpnam(char *s);

/* File access */
FILE *fopen(const char *path, const char *mode);
FILE *freopen(const char *path, const char *mode, FILE *stream);
FILE *fdopen(int fd, const char *mode);
int fclose(FILE *stream);
int fflush(FILE *stream);
void setbuf(FILE *stream, char *buf);
int setvbuf(FILE *stream, char *buf, int mode, size_t size);

/* Formatted output */
int fprintf(FILE *stream, const char *format, ...);
int fscanf(FILE *stream, const char *format, ...);
int printf(const char *format, ...);
int scanf(const char *format, ...);
int snprintf(char *s, size_t n, const char *format, ...);
int sprintf(char *s, const char *format, ...);
int sscanf(const char *s, const char *format, ...);
int vfprintf(FILE *stream, const char *format, va_list ap);
int vfscanf(FILE *stream, const char *format, va_list ap);
int vprintf(const char *format, va_list ap);
int vscanf(const char *format, va_list ap);
int vsnprintf(char *s, size_t n, const char *format, va_list ap);
int vsprintf(char *s, const char *format, va_list ap);
int vsscanf(const char *s, const char *format, va_list ap);
int asprintf(char **strp, const char *format, ...);
int vasprintf(char **strp, const char *format, va_list ap);
int dprintf(int fd, const char *format, ...);
int vdprintf(int fd, const char *format, va_list ap);

/* Character and line input/output */
int fgetc(FILE *stream);
char *fgets(char *s, int n, FILE *stream);
int fputc(int c, FILE *stream);
int fputs(const char *s, FILE *stream);
int getc(FILE *stream);
int getchar(void);
char *gets(char *s);
int putc(int c, FILE *stream);
int putchar(int c);
int puts(const char *s);
int ungetc(int c, FILE *stream);
ssize_t getdelim(char **linep, size_t *linecapp, int delim, FILE *stream);
ssize_t getline(char **linep, size_t *linecapp, FILE *stream);

/* Direct input/output */
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);

/* File positioning */
int fseek(FILE *stream, long offset, int whence);
long ftell(FILE *stream);
void rewind(FILE *stream);
int fgetpos(FILE *stream, fpos_t *pos);
int fsetpos(FILE *stream, const fpos_t *pos);
off_t fseeko(FILE *stream, off_t offset, int whence);
off_t ftello(FILE *stream);

/* Error handling */
void clearerr(FILE *stream);
int feof(FILE *stream);
int ferror(FILE *stream);
void perror(const char *s);

/* Descriptors and pipes */
int fileno(FILE *stream);
FILE *popen(const char *command, const char *mode);
int pclose(FILE *stream);

/* Locking */
void flockfile(FILE *stream);
int ftrylockfile(FILE *stream);
void funlockfile(FILE *stream);
int getc_unlocked(FILE *stream);
int getchar_unlocked(void);
int putc_unlocked(int c, FILE *stream);
int putchar_unlocked(int c);

#endif
