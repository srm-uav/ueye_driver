#include <sys/types.h>
#include <sys/epoll.h>

typedef struct event_source {
	int fd;
	uint32_t events;
	void (*cb)(void *userdata);
} source_t;
