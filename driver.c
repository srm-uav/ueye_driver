#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver.h"
#include "log.h"

int init_cam(Camera *c) {
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
	r = is_AllocImageMem(hid, width, height, 16, &img_mem, &img_id);
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

int capture_img(Camera *c) {
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
