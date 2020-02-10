#include <stdio.h>
#include <string.h>
#include <uEye.h>

#include "driver.h"

int init_cam(Camera *c) {
	c->ref = 0;

	int r;
	fprintf(stderr, "Initiating connection...\n");

	HIDS hid = 0;
	r = is_InitCamera(&hid, NULL);
	if (r != IS_SUCCESS) {
		fprintf(stderr, "[%s] Failed at step: InitCamera with error %d\n", __func__, r);
		return r;
	}
	fprintf(stderr, "Connected to camera %d\n", hid);

	SENSORINFO s;
	r = is_GetSensorInfo(&hid, &s);
	if (r != IS_SUCCESS) {
		fprintf(stderr, "[%s] Failed at step: GetSensorInfo with error %d\n", __func__, r);
		return r;
	}

	r = is_SetColorMode(hid, IS_CM_BGR8_PACKED);
	if (r != IS_SUCCESS) {
		fprintf(stderr, "[%s] Failed at step: SetColorMode with error %d\n", __func__, r);
		return r;
	}

	char *img_mem;
	INT img_id;
	DWORD width = s->nMaxWidth;
	DWORD height = s->nMaxHeight;
	r = is_AllocImageMem(hid, width, height, 16, &img_mem, &img_id);
	if (r != IS_SUCCESS) {
		fprintf(stderr, "[%s] Failed at step: AllocImageMem with error %d\n", __func__, r);
		return r;
	}

	r = is_SetImageMem(hid, img_mem, img_id);
	if (r != IS_SUCCESS) {
		fprintf(stderr, "[%s] Failed at step: SetImageMem with error %d\n", __func__, r);
		return r;
	}

	c->ref++;
	c->name = strdup(s->strSensorName);
	c->hid = hid;
	c->width = width;
	c->height = height;
	c->img_mem = img_mem;
	c->img_id = img_id;

	fprintf(stderr, "Sensor %s has been set initialized\n", c->name);
	return r = IS_SUCCESS;
}

int capture_cam(Camera *c);

int unref_cam(Camera *c);
