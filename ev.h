#ifndef __kkd_ev_h
#define __kkd_ev_h

#include <sys/epoll.h>

#include "driver.h"

typedef struct event_source {
	Camera *c;
	int fd;
	void (*cb)(void *userdata);
} source_t;

#endif
