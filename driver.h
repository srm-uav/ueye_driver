#ifndef __kkd_driver_h
#define __kkd_driver_h

#include <ueye.h>

struct Camera {
	unsigned int ref;
	HIDS hid;
	DWORD width;
	DWORD height;
	char *img_mem;
	INT img_id;
	char *name;
};

typedef struct Camera Camera;

int init_cam(Camera *c);

int capture_img(Camera *c);

int stream_loop(Camera *c, char *res, char *framerate);

void unref_cam(Camera *c);

#endif
