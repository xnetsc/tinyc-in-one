/* semaphore.h - POSIX semaphores for the win32 target.
 *
 * Part of the pthread surface: the implementation lives in
 * win32/lib/pthread.c and ships inside libtcc1.a.  Unnamed
 * semaphores are backed by Win32 semaphore objects.  Named
 * semaphores have no in-package backing: sem_open, sem_close
 * and sem_unlink fail with ENOSYS. */
#ifndef _SEMAPHORE_H
#define _SEMAPHORE_H

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void *_impl; /* implementation semaphore object */
} sem_t;

#define SEM_FAILED ((sem_t *)0)

int sem_init(sem_t *sem, int pshared, unsigned int value);
int sem_destroy(sem_t *sem);
sem_t *sem_open(const char *name, int oflag, ...);
int sem_close(sem_t *sem);
int sem_unlink(const char *name);
int sem_wait(sem_t *sem);
int sem_trywait(sem_t *sem);
int sem_timedwait(sem_t *sem, const struct timespec *abstime);
int sem_post(sem_t *sem);
int sem_getvalue(sem_t *sem, int *sval);

#ifdef __cplusplus
}
#endif

#endif /* _SEMAPHORE_H */
