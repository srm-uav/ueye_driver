#include <stdio.h>
#include <stdlib.h>

#include "driver.h"

int main(int argc, char *argv[]) {
	int r;
	if (argc > 1) {
		log_error("Program takes no arguments");
		return EXIT_FAILURE;
	}

	Camera c;
	r = init_cam(&c);
	if (r != IS_SUCCESS) {
		log_error("Error: Failed to initialize camera");
		goto end;
	}

	r = capture_img(&c);
	if (r != IS_SUCCESS) {
		log_error("Error: Failed to capture image");
		goto end;
	}

	r = unref_cam(&c);

end:
	return r < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
