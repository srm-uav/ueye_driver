#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "driver.h"
#include "log.h"

extern char log_buf[];

int main(int argc, char *argv[]) {
	int r;

	r = mlockall(MCL_FUTURE);
	if (r < 0) {
		log_warn("Failed to lock pages in memory, ignoring: %m");
	}

	r = setvbuf(stderr, log_buf, _IOFBF, LOG_BUF_SIZE);
	if (!r) {
		log_warn("Failed to set full buffering, ignoring: %m");
	}

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
