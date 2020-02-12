#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include "../log.h"

typedef union sockaddr_union {
	struct sockaddr_in sa_in;
	struct sockaddr_in6 sa_in6;
} sockaddr_t;

typedef struct event_source {
	int fd;
	void (*callback)(void *userdata);
} source_t;

static int epfd;

int sock_create(in_port_t port) {
	int r = -1;

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		log_error("Listening socket could not be created: %m");
		goto end;
	}

	sockaddr_t local = {
		.sa_in.sin_family = AF_INET,
		.sa_in.sin_addr = INADDR_ANY,
		.sa_in.sin_port = htons(port),
	};

	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

	r = bind(fd, &local.sa_in, sizeof(local.sa_in));
	if (r < 0) {
		log_error("Binding socket to port failed: %m");
		goto end;
	}

	r = listen(fd, 5);
	if (r < 0) {
		log_error("Listening on socket failed: %m");
		goto end;
	}

end:
	return r < 0 ? r : fd;
}


void read_frame(void *ptr) {
	int fd = *((int *) ptr);
	/* TODO */
	return;
}

void accept_conn(void *ptr) {
	int r;

	int fd = *((int *) ptr);
	sockaddr_t client;
	int cfd = accept4(fd, &client.sa_in, &(socklen_t) { sizeof(client.sa_in) }, SOCK_NONBLOCK|SOCK_CLOEXEC);
	if (cfd < 0) {
		log_warn("Accept on listen socket failed, ignoring: %m");
		return;
	}
	source_t *c = malloc(sizeof(source_t));
	*c = (source_t) { .fd = cfd, .callback = &read_frame };
	struct epoll_event ev = { .events = EPOLLIN, .data.ptr = &c };

	r = epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
	if (r < 0) {
		log_warn("Failed to register client in the event loop: %m");
		close(cfd);
		return;
	}
	return;
}

int main(int argc, char *argv[]) {
	int r = -1;

	int sockfd = sock_create(6969);
	if (sockfd < 0)
		goto end;

	epfd = epoll_create1(EPOLL_CLOEXEC);

	source_t *a = malloc(sizeof(source_t));
	*a = (source_t) { .fd = sockfd, .callback = &accept_conn };
	struct epoll_event ev = { .events = EPOLLIN, .data.ptr = &a };

	r = epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);
	if (r < 0) {
		log_error("Registration of listen socket in event loop failed: %m");
		goto end;
	}

	for (;;) {
		int r;

		struct epoll_event events[10];
		r = epoll_wait(epfd, events, 10, -1);
		if (r < 0) {
			log_error("Failed in epoll_wait: %m");
			goto end;
		}

		for (int i = 0; i < r; ++i) {
			source_t *s = (source_t *) events[0].data.ptr;
			s->callback(&s->fd);
		}
	}

end:
	r < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
