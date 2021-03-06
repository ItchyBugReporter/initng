/*
 * Initng, a next generation sysvinit replacement.
 * Copyright (C) 2006 Jimmy Wennlund <jimmy.wennlund@gmail.com>
 * Copyright (C) 2006 Ismael Luceno <ismael.luceno@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef INITNG_STATIC_EVENT_TYPES_H
#define INITNG_STATIC_EVENT_TYPES_H

#include <sys/select.h>

extern s_event_type EVENT_STATE_CHANGE;
extern s_event_type EVENT_SYSTEM_CHANGE;
extern s_event_type EVENT_IS_CHANGE;
extern s_event_type EVENT_UP_MET;
extern s_event_type EVENT_MAIN;
extern s_event_type EVENT_LAUNCH;
extern s_event_type EVENT_AFTER_FORK;
extern s_event_type EVENT_START_DEP_MET;
extern s_event_type EVENT_STOP_DEP_MET;
extern s_event_type EVENT_PIPE_WATCHER;
extern s_event_type EVENT_NEW_ACTIVE;
extern s_event_type EVENT_DEP_ON;
extern s_event_type EVENT_RELOAD_ACTIVE_DB;
extern s_event_type EVENT_DUMP_ACTIVE_DB;
extern s_event_type EVENT_ERROR_MESSAGE;
extern s_event_type EVENT_COMPENSATE_TIME;
extern s_event_type EVENT_HANDLE_KILLED;
extern s_event_type EVENT_SIGNAL;
extern s_event_type EVENT_BUFFER_WATCHER;
extern s_event_type EVENT_IO_WATCHER;
extern s_event_type EVENT_INTERRUPT;
extern s_event_type HALT;
extern s_event_type REBOOT;

void initng_register_static_event_types(void);


/* Event data types */

typedef struct {
	active_db_h *service;
	process_h *process;
	const char *exec_name;
} s_event_launch_data;

typedef struct {
	active_db_h *service;
	process_h *process;
} s_event_after_fork_data, s_event_handle_killed_data;

typedef struct {
	active_db_h *service;
	process_h *process;
	pipe_h *pipe;
} s_event_pipe_watcher_data;

typedef struct {
	active_db_h *service, *check;
} s_event_dep_on_data;

typedef struct {
	e_mt mt;
	const char *file, *func;
	int line;
	const char *format;
	va_list arg;
} s_event_error_message_data;

typedef struct {
	active_db_h *service;
	process_h *process;
	pipe_h *pipe;
	char *buffer_pos;
} s_event_buffer_watcher_data;

/* EVENT_IO_WATCHER actions */
#define IOW_ACTION_CLOSE	1
#define IOW_ACTION_CHECK	2
#define IOW_ACTION_CALL		3
#define IOW_ACTION_DEBUG	4

typedef struct {
	int action;
	int added;
	fd_set *readset, *writeset, *errset;
	char *debug_find_what;
	char **debug_out;
} s_event_io_watcher_data;

#endif /* INITNG_STATIC_EVENT_TYPES_H */
