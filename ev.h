#ifndef __kkd_ev_h
#define __kkd_ev_h

#include <sys/epoll.h>

typedef struct event_source {
	int fd;
	void (*cb)(void *userdata);
} source_t;

#endif
