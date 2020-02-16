#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include "process.h"
#include "driver.h"
#include "log.h"
#include "ev.h"

static int epfd;

int init_cam(Camera *c)
{
	c->ref = 0;

	int r;
	log_info("Initiating connection...");

	HIDS hid = 0;
	r = is_InitCamera(&hid, NULL);
	if (r != IS_SUCCESS) {
		log_error("Failed at step: InitCamera with error %d", r);
		return r;
	}
	log_info("Connected to camera %d", hid);

	SENSORINFO s;
	r = is_GetSensorInfo(hid, &s);
	if (r != IS_SUCCESS) {
		log_error("Failed at step: GetSensorInfo with error %d", r);
		return r;
	}

	r = is_SetColorMode(hid, IS_CM_BGR8_PACKED);
	if (r != IS_SUCCESS) {
		log_error("Failed at step: SetColorMode with error %d", r);
		return r;
	}

	char *img_mem;
	INT img_id;
	DWORD width = s.nMaxWidth;
	DWORD height = s.nMaxHeight;
	r = is_AllocImageMem(hid, width, height, 24, &img_mem, &img_id);
	if (r != IS_SUCCESS) {
		log_error("Failed at step: AllocImageMem with error %d", r);
		return r;
	}

	r = is_SetImageMem(hid, img_mem, img_id);
	if (r != IS_SUCCESS) {
		log_error("Failed at step: SetImageMem with error %d", r);
		return r;
	}

	c->ref++;
	c->name = strdup(s.strSensorName);
	c->hid = hid;
	c->width = width;
	c->height = height;
	c->img_mem = img_mem;
	c->img_id = img_id;

	log_info("Sensor %s has been set initialized", c->name);
	return r = IS_SUCCESS;
}

int capture_img(Camera *c)
{
	/* XXX: for now, the path is hardcoded to a file in the cwd of the process */

	int r;
	r = is_SetDisplayMode(c->hid, IS_SET_DM_DIB);
	if (r != IS_SUCCESS) {
		log_error("Failed at step: SetDisplayMode with error %d", r);
		return r;
	}

	r = is_FreezeVideo(c->hid, IS_WAIT);
	if (r != IS_SUCCESS) {
		log_error("Failed at step: FreezeVideo with error %d", r);
		return r;
	}

	IMAGE_FILE_PARAMS i;
	memset(&i, 0, sizeof(i));

	i.pwchFileName = L"./capture.png";
	i.nFileType = IS_IMG_PNG;
	i.ppcImageMem = &c->img_mem;
	i.pnImageID = &c->img_id;

	r = is_ImageFile(c->hid, IS_IMAGE_FILE_CMD_SAVE, &i, sizeof(i));
	if (r != IS_SUCCESS) {
		log_error("Failed at step: ImageFile with error %d", r);
		return r;
	}

	return r;
}

static int setup_worker(Camera *c, int *writefd, int *pidfd)
{
	int r;

	int pipefd[2];
	r = pipe2(pipefd, O_CLOEXEC);
	if (r < 0) {
		log_error("Failed to create pipe");
		goto end;
	}
	int fd;
	r = worker_create(&fd, pipefd[0]);
	close(pipefd[0]);
	if (r < 0) {
		log_error("Failed to create worker process: %m");
		goto end;
	}
	*pidfd = fd;
	*writefd = pipefd[1];
end:
	return r;
}

int stream_loop(Camera *c)
{
	int r, writefd, pidfd;
	r = setup_worker(c, &writefd, &pidfd);
	if (r < 0) {
		log_error("Failed to setup worker, fatal");
		return r;
	}
	close(1);
	dup2(writefd, 1);
	close(writefd);
	writefd = 1;

	epfd = epoll_create1(EPOLL_CLOEXEC);
	struct epoll_event ev, events[10];

	source_t worker = { .fd = pidfd, .cb = &pidfd_cb };
	ev.events = EPOLLIN;
	ev.data.ptr = &worker;
	r = epoll_ctl(epfd, EPOLL_CTL_ADD, worker.fd, &ev);
	if (r < 0) {
		log_error("Registration of worker's pidfd failed: %m");
		goto end;
	}

	for (;;) {
		if (__builtin_expect(fatal, 0)) {
			r = -1;
			goto end;
		}
		/* ratelimit me proportional to framerate and explore use of
		 * timerfd for frame generation and integration into event loop
		 */
		r = is_CaptureVideo(c->hid, IS_WAIT);
		if (r != IS_SUCCESS) {
			log_error("Failed at step CaptureVideo with error %d", r);
			goto end;
		}
		IMAGE_FILE_PARAMS i;
		memset(&i, 0, sizeof(i));
		/* we might have to use mkfifo to create the pipe so that is_ImageFile
		 * is able to open it, currently open would fail. The idea however is
		 * to stream jpegs to ffmpeg over its stdin and generate the mp4 stream
		 */
		i.pwchFileName = L"/proc/self/fd/1";
		i.nFileType = IS_IMG_JPG;
		i.ppcImageMem = &c->img_mem;
		i.pnImageID = &c->img_id;

		r = is_ImageFile(c->hid, IS_IMAGE_FILE_CMD_SAVE, &i, sizeof(i));
		if (r != IS_SUCCESS) {
			log_error("Failed at step: ImageFile with error %d", r);
			goto end;
		}
		/* dispatch routines must be non blocking */
		int n = epoll_wait(epfd, events, 10, 500000);
		for (int i = 0; i < n; ++i) {
			source_t *s = events[i].data.ptr;
			s->cb(&s);
		}
	}
end:
	close(pidfd);
	close(writefd);
	return r;
}

void unref_cam(Camera *c) {
	if (!(--c->ref)) {
		log_info("Refcount dropped to zero, freeing object...");

		int r;
		r = is_FreeImageMem(c->hid, c->img_mem, c->img_id);
		if (r != IS_SUCCESS) {
			log_error("Failed at step: FreeImageMem with error %d", r);
		}

		r = is_ExitCamera(c->hid);
		if (r != IS_SUCCESS) {
			log_error("Failed at step: ExitCamera with error %d", r);
		}
		log_info("Disconnected camera %s with HID %d", c->name, c->hid);
		free(c->name);
	}
	return;
}
