/* Windows feature smoke: the win32 target's pthread surface,
 * compiled with the packaged compiler and run on the Windows
 * CI runner. The program links with no -lpthread flag - the
 * implementation ships inside libtcc1.a - and the product
 * depends only on the Windows system DLLs. Each section
 * returns a distinct exit code on failure; success prints
 * FEATURE-PTHREAD-OK and returns 0. */
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static int counter;

static void *worker(void *arg)
{
    int i;
    (void)arg;
    for (i = 0; i < 25000; i++) {
        pthread_mutex_lock(&lock);
        counter++;
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

static pthread_once_t once = PTHREAD_ONCE_INIT;
static int once_ran;
static void run_once(void) { once_ran++; }

static pthread_key_t key;
static int dtor_ran;
static void dtor(void *v) { if (v) dtor_ran++; }
static void *key_worker(void *arg)
{
    (void)arg;
    pthread_setspecific(key, (void *)1);
    return NULL;
}

static pthread_mutex_t pc_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t pc_ready = PTHREAD_COND_INITIALIZER;
static int pc_value;
static void *consumer(void *arg)
{
    (void)arg;
    pthread_mutex_lock(&pc_lock);
    while (pc_value == 0)
        pthread_cond_wait(&pc_ready, &pc_lock);
    pthread_mutex_unlock(&pc_lock);
    return (void *)(long long)pc_value;
}

static sem_t sem;
static void *poster(void *arg)
{
    (void)arg;
    sem_post(&sem);
    return NULL;
}

static pthread_barrier_t barrier;
static void *barrier_worker(void *arg)
{
    (void)arg;
    pthread_barrier_wait(&barrier);
    return NULL;
}

int main(void)
{
    pthread_t th[4];
    int i, made = 0;
    void *ret = NULL;

    for (i = 0; i < 4; i++)
        if (pthread_create(&th[i], NULL, worker, NULL) == 0)
            made++;
    for (i = 0; i < made; i++)
        pthread_join(th[i], NULL);
    if (made != 4 || counter != 100000)
        return 1;

    pthread_once(&once, run_once);
    pthread_once(&once, run_once);
    if (once_ran != 1)
        return 2;

    if (pthread_key_create(&key, dtor) != 0)
        return 3;
    if (pthread_create(&th[0], NULL, key_worker, NULL) != 0)
        return 3;
    pthread_join(th[0], NULL);
    if (!dtor_ran)
        return 3;

    if (pthread_create(&th[0], NULL, consumer, NULL) != 0)
        return 4;
    pthread_mutex_lock(&pc_lock);
    pc_value = 7;
    pthread_cond_signal(&pc_ready);
    pthread_mutex_unlock(&pc_lock);
    pthread_join(th[0], &ret);
    if ((long long)ret != 7)
        return 4;

    if (sem_init(&sem, 0, 0) != 0)
        return 5;
    if (pthread_create(&th[0], NULL, poster, NULL) != 0)
        return 5;
    sem_wait(&sem);
    pthread_join(th[0], NULL);
    sem_destroy(&sem);

    if (pthread_barrier_init(&barrier, NULL, 4) != 0)
        return 6;
    for (i = 0; i < 3; i++)
        if (pthread_create(&th[i], NULL, barrier_worker, NULL) != 0)
            return 6;
    pthread_barrier_wait(&barrier);
    for (i = 0; i < 3; i++)
        pthread_join(th[i], NULL);
    pthread_barrier_destroy(&barrier);

    printf("FEATURE-PTHREAD-OK\n");
    return 0;
}
