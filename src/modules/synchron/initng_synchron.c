/*
 * Initng, a next generation sysvinit replacement.
 * Copyright (C) 2006 Jimmy Wennlund <jimmy.wennlund@gmail.com>
 * Copyright (C) 2005 neuron <aagaande@gmail.com>
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

#include <stdio.h>
#include <string.h>		/* strstr() */
#include <stdlib.h>		/* free() exit() */
#include <assert.h>

static int module_init(void);
static void module_unload(void);

const struct initng_module initng_module = {
	.api_version = API_VERSION,
	.deps = { "service", NULL },
	.init = &module_init,
	.unload = &module_unload
};


s_entry SYNCRON = {
	.name = "syncron",
	.description = "All services with this same syncron string, "
	    "can't be started asynchronous.",
	.type = STRING,
	.ot = NULL,
};

a_state_h *SERVICE_START_RUN;
static int check;

static int resolv_SSR(void)
{
	SERVICE_START_RUN = initng_active_state_find("SERVICE_START_RUN");
	if (!SERVICE_START_RUN) {
		F_("Could not resolve SERVICE_START_RUN, is service type "
		   "loaded?");
		return FALSE;
	}

	return TRUE;
}

/* Make sure if service syncron=module_loading, only one of the services with
 * module_loading runs at once */
static void check_syncronicly_service(s_event * event)
{
	active_db_h *service;
	active_db_h *current, *q = NULL;
	const char *service_syncron;
	const char *current_syncron;

	assert(event->event_type == &EVENT_START_DEP_MET);
	assert(event->data);

	service = event->data;

	assert(service->name);

	/* we must have this state resolve, to compare it */
	if (!resolv_SSR())
		return;

	service_syncron = get_string(&SYNCRON, service);
	if (!service_syncron)
		return;

	while_active_db_safe(current, q) {
		/* don't check ourself */
		if (current == service)
			continue;

		/* If this service has the start process running */
		if (IS_MARK(current, SERVICE_START_RUN)) {
			current_syncron = get_string(&SYNCRON, current);
			if (current_syncron) {
				if (strcmp(service_syncron,
					   current_syncron) == 0) {
					D_("Service %s has to wait for %s\n",
					   service->name, current->name);
					/* refuse to change status */
					event->status = FAILED;
					return;
				}
			}
		}
	}
}

/* Make sure there is only one service starting */
static void check_syncronicly(s_event * event)
{
	active_db_h *service;
	active_db_h *current, *q = NULL;

	assert(event->event_type == &EVENT_START_DEP_MET);
	assert(event->data);

	service = event->data;

	/* we must have this state resolve, to compare it */
	if (!resolv_SSR())
		return;

	while_active_db_safe(current, q) {
		/* don't check ourself */
		if (current == service)
			continue;

		if (IS_MARK(service, SERVICE_START_RUN)) {
			/* no i cant set this status yet */
			event->status = FAILED;
			return;
		}
	}
}

int module_init(void)
{
	int i;

	SERVICE_START_RUN = NULL;

	initng_service_data_type_register(&SYNCRON);
	for (i = 0; g.Argv[i]; i++) {
		if (strstr(g.Argv[i], "synchronously")) {
			check = TRUE;
			initng_event_hook_register(&EVENT_START_DEP_MET,
						   &check_syncronicly);

			return TRUE;
		}
	}

	check = FALSE;
	/* Notice this is only added if we don't have --synchronously */
	D_("Adding synchron\n");
	initng_event_hook_register(&EVENT_START_DEP_MET,
				   &check_syncronicly_service);

	return TRUE;
}

void module_unload(void)
{
	if (check == TRUE) {
		initng_event_hook_unregister(&EVENT_START_DEP_MET,
					     &check_syncronicly);
	}

	initng_event_hook_unregister(&EVENT_START_DEP_MET,
				     &check_syncronicly_service);
	initng_service_data_type_unregister(&SYNCRON);
}
