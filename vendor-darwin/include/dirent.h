/* dirent.h - directory entries, Darwin.
 *
 * Written for this package from the POSIX <dirent.h>
 * requirements with the Darwin struct dirent layout: 64-bit
 * inode, the 64-bit seek offset, the two 16-bit lengths, the
 * type byte, then the name buffer of 256 bytes (280 bytes in
 * total).  The macOS CI probe compares the size and the d_name
 * offset against the platform headers.
 */

#ifndef _TINYC_DARWIN_DIRENT_H
#define _TINYC_DARWIN_DIRENT_H

#include <stddef.h>
#include <sys/types.h>

#define MAXNAMLEN	255

struct dirent {
	ino_t		d_ino;
	uint64_t	d_seekoff;
	uint16_t	d_reclen;
	uint16_t	d_namlen;
	uint8_t		d_type;
	char		d_name[256];
};

typedef struct __dirdesc DIR;

#define DT_UNKNOWN	0
#define DT_FIFO		1
#define DT_CHR		2
#define DT_DIR		4
#define DT_BLK		6
#define DT_REG		8
#define DT_LNK		10
#define DT_SOCK		12
#define DT_WHT		14

DIR *opendir(const char *dirname);
DIR *fdopendir(int fd);
struct dirent *readdir(DIR *dirp);
int readdir_r(DIR *dirp, struct dirent *entry, struct dirent **result);
void rewinddir(DIR *dirp);
void seekdir(DIR *dirp, long loc);
long telldir(DIR *dirp);
int closedir(DIR *dirp);
int dirfd(DIR *dirp);
int scandir(const char *dirname, struct dirent ***namelist,
	    int (*select)(const struct dirent *),
	    int (*compar)(const struct dirent **, const struct dirent **));
int alphasort(const struct dirent **d1, const struct dirent **d2);
int getdirentries(int fd, char *buf, int nbytes, long *basep);

#endif
