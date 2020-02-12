/*
 * This file is part of berk.
 *
 * Copyright (C) 2019 Kumar Kartikeya Dwivedi <memxor@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it would be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#pragma once

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
		fprintf(stderr, "\n");					\
	})

#define log_info(...) log_internal(LOG_INFO, __VA_ARGS__)
#define log_warn(...) log_internal(LOG_WARN, __VA_ARGS__)
#define log_error(...) do { log_internal(LOG_ERROR, __VA_ARGS__); fflush(stderr); } while (0)
