#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

int stream_loop(Camera *c)
{
	int r;

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		log_error("Failed to create client socket: %m");
		goto end;
	}
	/* TODO: switch to generic funcs */
	struct sockaddr_in server;
	struct hostent *serv_addr;

	server.sin_family = AF_INET;
	server.sin_port = htons(6969);
	serv_addr = gethostbyname("localhost");

	memcpy(&server.sin_addr.s_addr, serv_addr->h_addr, serv_addr->h_length);
	r = connect(fd, (struct sockaddr *) &server, sizeof(server));
	if (r < 0) {
		log_error("Failed to establish connection to server: %m");
		goto end_close;
	}

	r = is_SetImageMem(c->hid, c->img_mem, c->img_id);
	if (r != IS_SUCCESS) {
		log_error("Failed at step SetImageMem with error %d", r);
		goto end_close;
	}

	INT lineinc;
	r = is_GetImageMemPitch(c->hid, &lineinc);
	if (r != IS_SUCCESS) {
		log_error("Failed at step GetImageMemPitch with error %d", r);
		goto end_close;
	}

	size_t size = lineinc * c->height;

	for (;;) {
		r = is_CaptureVideo(c->hid, IS_WAIT);
		if (r != IS_SUCCESS) {
			log_error("Failed at step CaptureVideo with error %d", r);
			goto end_close;
		}
		r = write(fd, c->img_mem, size);
		if (r < 0) {
			if (errno == EAGAIN)
				continue;
			log_error("Failed to transmit buffer: %m");
			goto end_close;
		}
	}

end_close:
	close(fd);
end:
	return r < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}

static int setup_worker(Camera *c, int *writefd, int *pidfd)
{
	int r;

	int pipefd2[2];
	r = pipe2(pipefd, O_CLOEXEC);
	if (r < 0) {
		log_error("Failed to create pipe");
		goto end:
	}
	int fd;
	r = pidfd_create(&fd, pipefd[0]);
	close(pipefd[0]);
	if (r < 0) {
		log_error("Failed to create worker process: %m");
		goto end_free:
	}
	*pidfd = fd;
	*writefd = pipefd[1];
end_free:
	close(pipefd[1]);
end:
	return r;
}

int stream_loop_exp(Camera *c)
{
	int r, writefd, pidfd;
	r = setup_worker(c, &writefd, &pidfd);
	if (r < 0) {
		log_error("Failed to setup worker, fatal");
		return r;
	}
	int fd = *pidfd;
	for (;;) {
		/* generate image, write to pipe, rinse and repeat */
	}
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
