#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/wait.h>

#include "log.h"
#include "process.h"

/* Explore use of clone3 and possible sandboxing of children */

pid_t pidfd_create(int *fd, int stdinfd)
{
	int r;
	pid_t pid = fork();
	if (pid) {
		*fd = pidfd_open(pid, 0);
	}
	else {
		/* set stdin */
		close(0);
		r = dup2(stdinfd, stdin);
		if (r < 0) {
			log_error("Failed to dup read end of pipe to stdin: %m");
			return pid;
		}
		/* set up arguments for our ffmpeg worker */
		char *argv[] = { NULL, "-f", "image2pipe", "-r", "1", "-vcodec", "mjpeg", "-i", "-", \
				 "-vcodec", "libx264", "out.mp4", NULL };
		char *envp[] = { NULL };
		r = execve("/usr/bin/ffmpeg", argv, envp);
		if (r < 0)
			log_error("Failed to exec into ffmpeg: %m");
	}
	return pid;
}

void pidfd_cb(void *ptr)
{
	int fd = *((int *) ptr);
	int r;

	siginfo_t info;
	r = waitid(P_PIDFD, fd, &info, WEXITED|WNOHANG);
	if (r < 0) {
		log_error("Failed to wait on child: %m");
		return;
	}
	if (!r) {
		if (!infop.si_pid && !infop.si_signo)
			log_warn("Callback raised but no process can be waited upon, ignoring");
		else {
			fatal = true;
			/* add logging for child exit */
		}
	}
	return;
}

int send_sig(int fd, int sig)
{
	int r;
	siginfo_t info;

	memset(&info, 0, sizeof(info));
	info.si_code = SI_QUEUE;
	info.si_signo = sig;
	info.si_errno = 0;
	info.si_uid = getuid();
	info.si_pid = getpid();
	info.si_value.sival_int = 1234;

	r = pidfd_send_signal(fd, sig, &info, 0);
	if (r < 0) {
		log_error("Failed to send signal to child process: %m");
	}
	return r;
}
