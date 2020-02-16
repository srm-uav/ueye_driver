#pragma once

#include <sys/epoll.h>

typedef struct event_source {
	int fd;
	void (*cb)(void *userdata);
} source_t;
