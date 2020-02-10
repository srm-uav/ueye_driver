#include <stdio.h>
#include <string.h>

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

int capture_cam(Camera *c) {
	/* XXX: for now, the path is hardcoded to a file in the cwd of the process */

	int r;
	r = is_SetDisplayMode(c->hid, IS_SET_DM_DIB);
	if (r != IS_SUCCESS) {
		fprintf(stderr, "[%s] Failed at step: SetDisplayMode with error %d\n", __func__, r);
		return r;
	}

	r = is_FreezeVideo(c->hid, IS_WAIT);
	if (r != IS_SUCCESS) {
		fprintf(stderr, "[%s] Failed at step: FreezeVideo with error %d\n", __func__, r);
		return r;
	}

	IMAGE_FILE_PARAMS i;
	memset(&i, 0, sizeof(i));

	i.pwchFileName = L"./capture.png";
	i.nFileType = IS_IMG_PNG;
	i.ppcImageMem = c->img_mem;
	i.pnImageID = c->img_id;

	r = is_ImageFile(c->hid, IS_IMAGE_FILE_CMD_SAVE, &i, sizeof(i));
	if (r != IS_SUCCESS) {
		fprintf(stderr, "[%s] Failed at step: ImageFile with error %d\n", __func__, r);
	}

	return r;
}

int unref_cam(Camera *c) {
	if (!(--c->ref)) {
		fprintf(stderr, "Refcount dropped to zero, freeing object...\n")

		int r;
		r = is_FreeImageMem(c->hid, c->img_mem, c->img_id);
		if (r != IS_SUCESS) {
			fprintf(stderr, "[%s] Failed at step: FreeImageMem with error %d\n", __func__, r);
			return r;
		}

		r = is_ExitCamera(c->hid);
		if (r != IS_SUCCESS) {
			fprintf(stderr, "[%s] Failed at step: ExitCamera with error %d\n", __func__, r);
			return r;
		}
		fprintf ("Disconnected camera %s with HID %d\n", c->name, c->hid);
		free(c->name);
	}
	return 0;
}
