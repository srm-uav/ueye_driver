#ifndef __kkd_log_h
#define __kkd_log_h

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

enum log_level {
	LOG_INFO,
	LOG_WARN,
	LOG_ERROR,
	_LOG_LEVEL_MAX,
};

#define LOG_DEBUG 1
#define LOG_BUF_SIZE 4096U

char log_buf[LOG_BUF_SIZE];

static char* log_level_to_str[_LOG_LEVEL_MAX] = {
	[LOG_INFO] = "info",
	[LOG_WARN] = "warn",
	[LOG_ERROR] = "error",
};

#define log_internal(level, ...)					\
	({								\
		int _lev = level;					\
		char *_levs = log_level_to_str[_lev];			\
		if (LOG_DEBUG)						\
			fprintf(stderr, "(%s) [%s:%s:%d] ",		\
				_levs, __FILE__, __func__, __LINE__);	\
		fprintf(stderr, __VA_ARGS__);				\
	})

#define log_info(...) log_internal(LOG_INFO, __VA_ARGS__)
#define log_warn(...) log_internal(LOG_WARN, __VA_ARGS__)
#define log_error(...) do { log_internal(LOG_ERROR, __VA_ARGS__); fflush(stderr); } while (0)

#endif
