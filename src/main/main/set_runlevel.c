/*
 * Initng, a next generation sysvinit replacement.
 * Copyright (C) 2006 Jimmy Wennlund <jimmy.wennlund@gmail.com>
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

#include <initng.h>

#include <time.h>		/* time() */
#include <fcntl.h>		/* fcntl() */
#include <string.h>		/* memmove() strcmp() */
#include <sys/wait.h>		/* waitpid() sa */
#include <sys/ioctl.h>		/* ioctl() */
#include <stdlib.h>		/* free() exit() */
#include <termios.h>
#include <stdio.h>
#include <errno.h>
#ifdef SELINUX
#include <selinux/selinux.h>
#include <selinux/get_context_list.h>
#endif
#ifdef HAVE_COREDUMPER
#include <google/coredumper.h>
#endif

void initng_main_set_runlevel(const char *runlevel)
{
	/* first free the old_runlevel */
	if (g.old_runlevel) {
		free(g.old_runlevel);
		g.old_runlevel = NULL;
	}

	/* then move the last new, to the old */
	if (g.runlevel) {
		g.old_runlevel = g.runlevel;
		g.runlevel = NULL;
	}

	/* and set the new */
	g.runlevel = initng_toolbox_strdup(runlevel);

	/* call system state modules on a change like this */
	initng_module_callers_system_changed(g.sys_state);
}
