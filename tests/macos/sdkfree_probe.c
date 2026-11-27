/* ABI probe for the bundled Darwin headers (vendor-darwin/).
 *
 * Prints one key=value line per ABI fact: struct sizes and field
 * offsets, scalar type widths, and the value of every constant
 * whose Darwin value differs from (or might be confused with)
 * the Linux one.  run_macos_sdkfree.sh compiles this file twice -
 * with clang + the platform SDK headers, and with the packaged
 * tinyc + the bundled headers while the SDK is hidden - and diffs
 * the outputs.  An empty diff is the platform confirming the
 * bundled declarations.
 *
 * Lines starting with INFO are excluded from the diff: the
 * setjmp buffers are deliberately oversized in the bundled
 * headers, so their sizes are reported for the record and are
 * settled by the platform's own value in the reference output.
 */
#include <stdio.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <sys/ucontext.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <setjmp.h>
#include <time.h>
#include <wchar.h>
#include <sched.h>
#include <errno.h>
#include <fenv.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>

#define SZ(T) printf("sizeof_" #T "=%zu\n", sizeof(T))
#define OFF(T, f) printf("off_" #T "_" #f "=%zu\n", offsetof(T, f))
/* Offset of a register inside the thread-state struct the
 * mcontext's __ss member is declared with, without naming that
 * struct: the typedef names differ between the bundled header
 * and the platform headers (the platform SDK has no
 * __darwin_*_t typedef at all, so the reference probe must not
 * name one), so the type is taken from the __ss member itself.
 * The key is off___ss_<register> on both legs. */
#define OFF_SS(f) printf("off___ss_" #f "=%zu\n", \
	offsetof(__typeof__(((struct __darwin_mcontext64 *)0)->__ss), f))
#define K(name) printf(#name "=%lld\n", (long long)(name))

int main(void) {
    SZ(struct stat); SZ(struct timespec); SZ(struct timeval);
    SZ(struct dirent); SZ(struct flock); SZ(struct sigaction);
    SZ(stack_t); SZ(struct tm); SZ(struct rusage); SZ(struct rlimit);
    SZ(ucontext_t); SZ(struct __siginfo);
    SZ(struct __darwin_mcontext64);
    SZ(pthread_attr_t); SZ(pthread_mutex_t); SZ(pthread_mutexattr_t);
    SZ(pthread_cond_t); SZ(pthread_condattr_t); SZ(pthread_rwlock_t);
    SZ(pthread_rwlockattr_t); SZ(pthread_once_t); SZ(pthread_t);
    SZ(fd_set); SZ(mbstate_t); SZ(sigset_t); SZ(struct sched_param);
    SZ(off_t); SZ(time_t); SZ(dev_t); SZ(ino_t); SZ(mode_t);
    SZ(nlink_t); SZ(blksize_t); SZ(blkcnt_t); SZ(clock_t);
    SZ(suseconds_t); SZ(pid_t); SZ(uid_t); SZ(size_t); SZ(ptrdiff_t);
    SZ(fenv_t); SZ(fexcept_t);
    SZ(socklen_t); SZ(struct sockaddr); SZ(struct sockaddr_in);
    SZ(struct sockaddr_in6); SZ(struct sockaddr_storage);
    SZ(struct msghdr); SZ(struct cmsghdr); SZ(struct linger);
    SZ(struct addrinfo); SZ(struct hostent); SZ(struct servent);
    SZ(struct ip_mreq); SZ(struct in6_addr);
    OFF(struct sockaddr_in, sin_port); OFF(struct sockaddr_in, sin_addr);
    OFF(struct sockaddr_in6, sin6_port); OFF(struct sockaddr_in6, sin6_addr);
    OFF(struct sockaddr_in6, sin6_scope_id);
    OFF(struct addrinfo, ai_addrlen); OFF(struct addrinfo, ai_addr);
    OFF(struct addrinfo, ai_next);
    OFF(struct msghdr, msg_iov); OFF(struct msghdr, msg_control);
    OFF(struct msghdr, msg_flags);
    OFF(struct hostent, h_addr_list); OFF(struct servent, s_port);
    OFF(struct stat, st_size); OFF(struct stat, st_mtimespec);
    OFF(struct stat, st_birthtimespec); OFF(struct stat, st_blocks);
    OFF(struct dirent, d_name); OFF(struct dirent, d_reclen);
    OFF(struct tm, tm_gmtoff); OFF(struct rusage, ru_maxrss);
    OFF(struct flock, l_start); OFF(struct sigaction, sa_mask);
    OFF(ucontext_t, uc_sigmask); OFF(ucontext_t, uc_stack);
    OFF(ucontext_t, uc_link); OFF(ucontext_t, uc_mcsize);
    OFF(ucontext_t, uc_mcontext);
    OFF(struct __siginfo, si_code); OFF(struct __siginfo, si_addr);
/* The register offsets are taken through the type of the
 * mcontext's __ss member (OFF_SS, above), not through the bare
 * Mach struct names (struct x86_thread_state64 /
 * struct arm_thread_state64 live in <mach/.../thread_status.h>,
 * which <sys/ucontext.h> does not include) and not through the
 * __darwin_*_t typedefs (which only the bundled header provides;
 * the platform SDK has no such names, so the reference probe
 * would not compile). */
#if defined(__x86_64__)
    OFF(struct __darwin_mcontext64, __ss);
    OFF(struct __darwin_mcontext64, __fs);
    OFF_SS(__rip);
    OFF_SS(__rbp);
#elif defined(__aarch64__)
    OFF(struct __darwin_mcontext64, __ss);
    OFF(struct __darwin_mcontext64, __ns);
    OFF_SS(__pc);
    OFF_SS(__fp);
#endif
    printf("INFO sizeof_jmp_buf=%zu\n", sizeof(jmp_buf));
    printf("INFO sizeof_sigjmp_buf=%zu\n", sizeof(sigjmp_buf));
    K(O_CREAT); K(O_APPEND); K(O_CLOEXEC); K(O_RDONLY); K(O_RDWR);
    K(MAP_ANON); K(MAP_SHARED); K(PROT_READ); K(PROT_WRITE);
    K(CLOCK_REALTIME); K(CLOCK_MONOTONIC);
    K(SCHED_OTHER); K(SCHED_FIFO); K(SCHED_RR);
    K(SIGBUS); K(SIGUSR1); K(SIGSYS); K(SIGCHLD); K(SIGKILL);
    K(FPE_INTDIV); K(FPE_FLTDIV); K(SIG_HOLD);
    K(FP_NAN); K(FP_INFINITE); K(FP_ZERO); K(FP_NORMAL); K(FP_SUBNORMAL);
    K(FP_ILOGB0); K(FP_ILOGBNAN);
    K(FE_INVALID); K(FE_DIVBYZERO); K(FE_OVERFLOW); K(FE_UNDERFLOW);
    K(FE_INEXACT); K(FE_ALL_EXCEPT);
    K(FE_TONEAREST); K(FE_DOWNWARD); K(FE_UPWARD); K(FE_TOWARDZERO);
    K(EAGAIN); K(ENOTSUP); K(EDEADLK); K(EPERM); K(EINTR);
    K(PATH_MAX); K(RAND_MAX); K(BUFSIZ); K(FD_SETSIZE);
    K(SEEK_SET); K(F_GETFL); K(F_SETLK); K(AT_FDCWD);
    K(_SC_PAGESIZE); K(_SC_NPROCESSORS_ONLN); K(_SC_OPEN_MAX);
    K(AF_UNIX); K(AF_INET); K(AF_INET6); K(AF_ROUTE); K(AF_LINK);
    K(SOCK_STREAM); K(SOCK_DGRAM); K(SOCK_RAW); K(SOCK_RDM);
    K(SOCK_SEQPACKET);
    K(IPPROTO_IP); K(IPPROTO_TCP); K(IPPROTO_UDP); K(IPPROTO_IPV6);
    K(IPPROTO_RAW);
    K(SOL_SOCKET); K(SO_REUSEADDR); K(SO_KEEPALIVE); K(SO_BROADCAST);
    K(SO_LINGER); K(SO_SNDBUF); K(SO_RCVBUF); K(SO_SNDTIMEO);
    K(SO_RCVTIMEO); K(SO_ERROR); K(SO_TYPE); K(SO_NOSIGPIPE);
    K(SOMAXCONN);
    K(SHUT_RD); K(SHUT_WR); K(SHUT_RDWR);
    K(MSG_OOB); K(MSG_PEEK); K(MSG_DONTROUTE); K(MSG_WAITALL);
    K(MSG_DONTWAIT); K(MSG_NOSIGNAL);
    K(IP_TTL); K(IP_MULTICAST_IF); K(IP_ADD_MEMBERSHIP);
    K(IP_DROP_MEMBERSHIP); K(IPV6_V6ONLY);
    K(TCP_NODELAY); K(TCP_MAXSEG); K(TCP_NOPUSH); K(TCP_KEEPALIVE);
    K(TCP_KEEPINTVL); K(TCP_KEEPCNT); K(TCP_FASTOPEN);
    K(INADDR_LOOPBACK); K(INET_ADDRSTRLEN); K(INET6_ADDRSTRLEN);
    K(AI_PASSIVE); K(AI_CANONNAME); K(AI_NUMERICHOST);
    K(AI_NUMERICSERV); K(AI_ALL); K(AI_V4MAPPED_CFG);
    K(AI_ADDRCONFIG); K(AI_V4MAPPED);
    K(EAI_BADFLAGS); K(EAI_NONAME); K(EAI_AGAIN); K(EAI_FAIL);
    K(EAI_NODATA); K(EAI_FAMILY); K(EAI_SOCKTYPE); K(EAI_SERVICE);
    K(EAI_MEMORY); K(EAI_SYSTEM); K(EAI_OVERFLOW);
    K(NI_MAXHOST); K(NI_MAXSERV); K(NI_NOFQDN); K(NI_NUMERICHOST);
    K(NI_NAMEREQD); K(NI_NUMERICSERV); K(NI_DGRAM);
    printf("RLIM_INFINITY=%llu\n", (unsigned long long)RLIM_INFINITY);
    return 0;
}
