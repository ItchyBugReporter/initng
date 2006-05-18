/*
 * Initng, a next generation sysvinit replacement.
 * Copyright (C) 2006 Jimmy Wennlund <jimmy.wennlund@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
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

#include <stdio.h>
#include <stdlib.h>							/* free() exit() */
#include <string.h>
#include <assert.h>
#include <time.h>

#include <initng_handler.h>
#include <initng_global.h>
#include <initng_plugin_hook.h>
#include <initng_common.h>
#include <initng_toolbox.h>
#include <initng_static_data_id.h>
#include <initng_static_states.h>
#include <initng_static_service_types.h>
#include <initng_execute.h>
#include <initng_process_db.h>

INITNG_PLUGIN_MACRO;

const char *module_needs[] = {
	"daemon",
	NULL
};

ptype_h T_DAEMON_CLEAN = { "daemon_clean", NULL };

static int on_kill(active_db_h * service, process_h * process);

static int on_kill(active_db_h * s, process_h * p)
{
	if (strcmp(p->pt->name, "daemon") != 0)
		return (FALSE);

	/* start the T_DAEMON_CLEAN */
	W_("%s %s has been killed!, executing DAEMON_CLEAN!\n", p->pt->name,
	   s->name);
	initng_execute_launch(s, &T_DAEMON_CLEAN, NULL);

	/*
	 * if we return TRUE, other kill handlers wont get this signal, 
	 * it means that this signal have been handled.
	 */
	return (FALSE);
}

int module_init(int api_version)
{
	D_("module_init();\n");
	if (api_version != API_VERSION)
	{
		F_("This module is compiled for api_version %i version and initng is compiled on %i version, won't load this module!\n", API_VERSION, api_version);
		return (FALSE);
	}

	initng_process_db_ptype_register(&T_DAEMON_CLEAN);
	initng_plugin_hook_register(&g.HANDLE_KILLED, 5, &on_kill);
	return (TRUE);
}

void module_unload(void)
{
	D_("module_unload();\n");
	initng_plugin_hook_unregister(&g.HANDLE_KILLED, &on_kill);
	initng_process_db_ptype_unregister(&T_DAEMON_CLEAN);
}
