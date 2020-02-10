#include <stdio.h>
#include <unistd.h>

#include "driver.h"

int main(int argc, char *argv[]) {
	int r;
	if (argc > 1) {
		fprintf(stderr, "Program takes no arguments\n");
		return EXIT_FAILURE;
	}

	Camera c;
	r = init_cam(&c);
	if (r != IS_SUCCESS) {
		fprintf(stderr, "Error: Failed to initialize camera\n");
		goto end;
	}

	r = capture_cam(&c);
	if (r != IS_SUCCESS) {
		fprintf(stderr, "Error: Failed to capture image\n");
		goto end;
	}

	r = unref_cam(&c);

end:
	return r < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
