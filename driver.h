#ifndef __kkd_driver_h
#define __kkd_driver_h

#include <assert.h>
#include <ueye.h>

#include "log.h"

enum state {
	CAM_INACTIVE,
	CAM_STARTING,
	CAM_RUNNING,
	CAM_STOPPING,
	CAM_FAILED,
	_CAM_STATE_MAX,
};

typedef enum state state_t;

extern const char * const statestr[_CAM_STATE_MAX];

struct Camera {
	state_t state;
	unsigned int ref;
	HIDS hid;
	DWORD width;
	DWORD height;
	char *img_mem;
	UINT img_id;
	char *name;
};

typedef struct Camera Camera;

int init_cam(Camera *c);

int capture_img(Camera *c);

int stream_loop(Camera *c, char *res, char *framerate);

void unref_cam(Camera *c);

static inline void chstate(Camera *c, state_t a)
{
	assert(c);
	assert(a < _CAM_STATE_MAX);
	log_info("Changing state for Camera (ID:%d): [%s] -> [%s]", c->hid, statestr[c->state], statestr[a]);
	c->state = a;
}

#endif
