#ifndef __kkd_process_h
#define __kkd_process_h

#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/syscall.h>

#ifndef __NR_pidfd_open
#define __NR_pidfd_open 434   /* System call # on most architectures */
#endif

#ifndef __NR_pidfd_send_signal
#define __NR_pidfd_send_signal 424
#endif

int pidfd_open(pid_t pid, unsigned int flags);

int pidfd_send_signal(int pidfd, int sig, siginfo_t *info, unsigned int flags);

pid_t worker_create(int *fd, int stdinfd, char *res, char *framerate);

void pidfd_cb(void *ptr);

int send_sig(int fd, int sig);

#endif
