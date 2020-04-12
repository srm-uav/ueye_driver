#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "process.h"
#include "driver.h"
#include "log.h"
#include "ev.h"

/* Explore use of clone3 and possible sandboxing of children */

int pidfd_open(pid_t pid, unsigned int flags)
{
	return syscall(__NR_pidfd_open, pid, flags);
}

int pidfd_send_signal(int pidfd, int sig, siginfo_t *info, unsigned int flags)
{
	return syscall(__NR_pidfd_send_signal, pidfd, sig, info, flags);
}

pid_t worker_create(int *fd, int stdinfd, char *res, char *framerate)
{
	int r;
	pid_t pid = fork();
	if (pid) {
		*fd = pidfd_open(pid, 0);
	}
	else {
		/* set stdin */
		close(0);
		r = dup2(stdinfd, 0);
		if (r < 0) {
			log_error("Failed to dup read end of pipe to stdin: %m");
			return pid;
		}
		/* set up arguments for our ffmpeg worker */
		r = mkdir("hls", 0755);
		if (r < 0) {
			log_error("Failed to create directory hls, ignoring: %m");
		}
		char *argv[] = { "ffmpeg", "-f", "image2pipe", "-framerate", framerate, "-i", "/dev/stdin", "-an", "-s", res, "-c:v", \
				 "libx264", "-pix_fmt", "yuv420p", "-hls_time", "1", "-hls_list_size", "10", "-hls_segment_filename", \
				 "hls/capture%05d.ts", "-hls_flags", "delete_segments", "hls/index.m3u8", NULL };
		char *envp[] = { NULL };
		r = execve("/usr/bin/ffmpeg", argv, envp);
		if (r < 0)
			log_error("Failed to exec into ffmpeg: %m");

		exit(EXIT_FAILURE);
	}
	return pid;
}

void pidfd_cb(void *ptr)
{
	source_t *p = (source_t *) ptr;
	int fd = p->fd;
	int r;

	siginfo_t info;

#ifndef P_PIDFD
#define P_PIDFD 3
#endif

	r = waitid(P_PIDFD, fd, &info, WEXITED|WNOHANG);
	if (r < 0) {
		log_error("Failed to wait on child: %m");
		return;
	}
	if (!r) {
		if (!info.si_pid && !info.si_signo)
			log_warn("Callback raised but no process can be waited upon, ignoring");
		else {
			log_error("Worker process died/interrupted, fatal");
			log_info("Changing state of Camera: %s -> %s", statestr[p->c->state], statestr[CAM_FAILED]);
			p->c->state = CAM_FAILED;
		}
	}
	return;
}

int send_sig(int fd, int sig)
{
	int r;

	r = pidfd_send_signal(fd, sig, NULL, 0);
	if (r < 0) {
		log_error("Failed to send signal to child process: %m");
	}
	return r;
}
