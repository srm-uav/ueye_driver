#ifndef __kkd_driver_h
#define __kkd_driver_h

#include <ueye.h>

enum states {
	CAM_INACTIVE = 0,
	CAM_STARTING,
	CAM_RUNNING,
	CAM_STOPPING,
	CAM_FAILED,
	_CAM_STATE_MAX,
};

extern const char * const statestr[_CAM_STATE_MAX];

struct Camera {
	enum states state;
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
