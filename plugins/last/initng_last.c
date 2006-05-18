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

#include <initng_handler.h>
#include <initng_global.h>
#include <initng_plugin_hook.h>
#include <initng_common.h>
#include <initng_depend.h>
#include <initng_toolbox.h>
#include <initng_static_data_id.h>
#include <initng_static_states.h>
#include <initng_static_service_types.h>

INITNG_PLUGIN_MACRO;

s_entry LAST = { "last", SET, NULL,
	"If this option is set, you will be sure this service is started last."
};


/* returns TRUE if all use deps are started */
static int check_last(active_db_h * service)
{
	active_db_h *current = NULL;

	assert(service);
	assert(service->name);

	/* And LAST is set. */
	if (!is(&LAST, service))
		return (TRUE);

	/* ok check with all service */
	D_("LAST: walking through service db\n");
	while_active_db(current)
	{
		/* don't check ourself */
		if (current == service)
			continue;

		/* ignore runlevels */
		/* TODO 20051105 SaTaN0rX: i do not know why i need to check for this */
		/*if (current->type == &TYPE_RUNLEVEL)
		   continue; */

		/* if this service also should be started last, continue */
		if (is(&LAST, current))
			continue;

		/* TODO, use dep_on_deep */
		/* if current need service that should be last */
		if (initng_depend_deep(current, service) == TRUE)
		{
			/* don't wait, because this wait is circular */
			D_("Service %s depends on %s\n", service->name, current->name);
			continue;
		}

		if (IS_STARTING(current))
		{
			D_("Service %s is also starting, and %s should be started last\n",
			   current->name, service->name);
			return (FALSE);
		}
	}

	/* ok, finally last start this */
	return (TRUE);
}

int module_init(int api_version)
{
	D_("module_init();\n");
	if (api_version != API_VERSION)
	{
		F_("This module is compiled for api_version %i version and initng is compiled on %i version, won't load this module!\n", API_VERSION, api_version);
		return (FALSE);
	}

	initng_service_data_type_register(&LAST);
	initng_plugin_hook_register(&g.START_DEP_MET, 99, &check_last);
	return (TRUE);
}

void module_unload(void)
{
	D_("module_unload();\n");
	initng_service_data_type_unregister(&LAST);
	initng_plugin_hook_unregister(&g.START_DEP_MET, &check_last);
}
