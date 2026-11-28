#if defined(__wasm__) || defined(__wasm32__)

#ifndef _STDIO_H
#define _STDIO_H

/* stdio.h - input and output for the tinyc-web freestanding
   sysroot.
 *
 * In the tinyc-web process model the functions below are
 * provided by the host shim layer as env imports.  There is no
 * stdio state in the guest: a FILE is an opaque handle value,
 * and the three standard streams are the fixed handle values
 * the macros below name — stdin reads the run's stdin bytes,
 * stdout writes to the run's output, stderr to its error
 * output.  (The process model has no data symbols a header
 * could declare stdin/stdout/stderr variables over, so they
 * are macros over the handle values.)  fclose on a standard
 * stream has no effect.
 *
 * fopen opens files in the process's file space: for a process
 * attached to a Tinyc instance that is the instance's virtual
 * FS — the same files the host stages with addFile and reads
 * back — and for a standalone run it is a private in-memory
 * space that lives for the run.  Paths are taken relative to
 * the file space's root.  Writes go through to the file space
 * as they happen, so every stream is effectively unbuffered:
 * setbuf and setvbuf record the requested mode and report
 * success, and output is visible to other readers immediately
 * in all modes.  The scanf family is provided.  Failed calls
 * record an error number, which <errno.h>'s errno reads back
 * and perror reports; errno there is a macro over the layer's
 * accessor function (a data symbol cannot be supplied by the
 * layer — the same reason the standard streams are macros
 * above). */

#include <stddef.h>
#include <stdarg.h>

typedef struct __tinyc_file FILE;

/* The position type of fgetpos/fsetpos: a byte offset. */
typedef long long fpos_t;

#ifndef EOF
#define EOF	(-1)
#endif

#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2

/* Streams are write-through (see the header comment); BUFSIZ
   sizes caller-provided arrays. */
#define BUFSIZ		1024
#define FILENAME_MAX	4096

/* Buffering modes for setvbuf.  All modes behave the same in
   this layer (writes go through immediately); the values are
   the conventional ones. */
#define _IOFBF		0
#define _IOLBF		1
#define _IONBF		2

/* tmpnam: the size a caller-provided buffer must have, and a
   lower bound on the number of distinct names it generates.
   Generated names live in the process's file space, in the
   same generated-name space tmpfile's backing entries use. */
#define L_tmpnam	64
#define TMP_MAX		10000

#define stdin	((FILE *)0)
#define stdout	((FILE *)1)
#define stderr	((FILE *)2)

/* tmpnam: the layer generates names under / that collide with
   no existing backing entry (the tmpfile naming discipline);
   L_tmpnam bounds the generated names, TMP_MAX is the count
   the layer guarantees unique names for. */
#define L_tmpnam	64
#define TMP_MAX		1000000

/* File operations */
FILE *fopen(const char *path, const char *mode);
FILE *freopen(const char *path, const char *mode, FILE *stream);
int fclose(FILE *stream);
int fflush(FILE *stream);
int remove(const char *path);
int rename(const char *oldpath, const char *newpath);
/* A FILE handle value is the descriptor in this layer, so
   fdopen validates the descriptor value and returns the same
   stream.  tmpfile's backing entry is unlinked at fclose. */
FILE *fdopen(int fd, const char *mode);
FILE *tmpfile(void);
char *tmpnam(char *s);

/* Buffering control and error reporting */
int setvbuf(FILE *stream, char *buf, int mode, size_t size);
void setbuf(FILE *stream, char *buf);
void perror(const char *s);

/* Direct input and output */
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
int fgetc(FILE *stream);
char *fgets(char *s, int size, FILE *stream);
int fputc(int c, FILE *stream);
int getc(FILE *stream);
int getchar(void);
int putc(int c, FILE *stream);
int putchar(int c);
int fputs(const char *s, FILE *stream);
int puts(const char *s);
int ungetc(int c, FILE *stream);

#define getc(stream)	fgetc(stream)
#define putc(c, stream)	fputc(c, stream)

/* Positioning */
int fseek(FILE *stream, long offset, int whence);
long ftell(FILE *stream);
void rewind(FILE *stream);
int fgetpos(FILE *stream, fpos_t *pos);
int fsetpos(FILE *stream, const fpos_t *pos);

/* Error handling */
int feof(FILE *stream);
int ferror(FILE *stream);
void clearerr(FILE *stream);

/* Formatted output */
int printf(const char *format, ...);
int fprintf(FILE *stream, const char *format, ...);
int sprintf(char *str, const char *format, ...);
int snprintf(char *str, size_t size, const char *format, ...);
int vprintf(const char *format, va_list ap);
int vfprintf(FILE *stream, const char *format, va_list ap);
int vsprintf(char *str, const char *format, va_list ap);
int vsnprintf(char *str, size_t size, const char *format, va_list ap);

/* Formatted input */
int scanf(const char *format, ...);
int fscanf(FILE *stream, const char *format, ...);
int sscanf(const char *str, const char *format, ...);
int vscanf(const char *format, va_list ap);
int vfscanf(FILE *stream, const char *format, va_list ap);
int vsscanf(const char *str, const char *format, va_list ap);

#endif /* _STDIO_H */

#else

/* This header ships in the wasm target's include set.  For any
   other target the system's own stdio.h is the right one. */
#include_next <stdio.h>

#endif
