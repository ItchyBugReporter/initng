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

/* common standard first define */
#include "initng.h"

/* system headers */
#include <sys/time.h>
#include <time.h>							/* time() */
#include <fcntl.h>							/* fcntl() */
#include <sys/un.h>							/* memmove() strcmp() */
#include <sys/wait.h>						/* waitpid() sa */
#include <linux/kd.h>						/* KDSIGACCEPT */
#include <sys/ioctl.h>						/* ioctl() */
#include <stdio.h>							/* printf() */
#include <stdlib.h>							/* free() exit() */
#include <sys/reboot.h>						/* reboot() RB_DISABLE_CAD */
#include <assert.h>

#include "initng_common.h"

/* local headers include */
#include "initng_main.h"
#include "initng_active_db.h"
#include "initng_load_module.h"
#include "initng_plugin_callers.h"
#include "initng_toolbox.h"
#include "initng_string_tools.h"
#include "initng_global.h"
#include "initng_static_states.h"
#include "initng_depend.h"
#include "initng_handler.h"

/*
 * this function walks through the g.Argv, if it founds service name,
 * with an starting -service, this function will return FALSE, and
 * the service wont be able to load, means that is it blacklisted.
 * return TRUE if service is okay to load.
 */
int initng_common_service_blacklisted(const char *name)
{
	int i;

	assert(name);
	assert(g.Argv);

	/* walk through arguments looking for this dep to be blacklisted */
	for (i = 1; (g.Argv)[i]; i++)
	{
		/* if we got a match */
		if ((g.Argv)[i][0] == '-')
		{
			if (strcmp(name, (g.Argv)[i] + 1) == 0
				|| service_match(name, (g.Argv)[i] + 1))
				return (TRUE);
		}
	}
	return (FALSE);
}

/* creates a active service */
active_db_h *initng_common_load_to_active(const char *service_name)
{
	active_db_h *a_new = NULL;
	active_db_h *current = NULL;

	assert(service_name);
	D_("load_to_active(%s); \n", service_name);

	/* check the blacklist db */
	if (initng_common_service_blacklisted(service_name))
	{
		F_("load_to_active(%s): Service BLACKLISTED.\n", service_name);
		return (NULL);
	}

	/* This shud be done by the function calling this service */
#ifdef EXTRA_CHECK
	/* Make sure there isn't any process with this name running */
	if ((a_new = initng_active_db_find_by_name(service_name)))
	{
		F_("load_to_active(%s): Service with same name %s exists! \n",
		   service_name);
		return (NULL);
	}
#endif

	/* if not create a new active entry, out of memory? */
	if (!(a_new = initng_active_db_new(service_name)))
	{
		D_("load_to_active(%s): Unable to allocate process, out of memory?\n",
		   service_name);
		return (NULL);
	}

	/* this actually loads service from disk */
	if (!initng_common_get_service(a_new) || !(a_new->from_service))
	{
		D_("load_to_active(%s): Can't get service!\n", service_name);
		if (a_new)
			initng_active_db_free(a_new);
		return (NULL);
	}

	/* Mark this service as state LOADING, plug-ins may hook here */
	if (!initng_common_mark_service(a_new, &LOADING))
	{
		W_("Failed to mark service LOADING.\n");
		if (a_new)
			initng_active_db_free(a_new);
		return (NULL);
	}

	/*
	   if (a_new->from_service->type != TYPE_SERVICE &&
	   a_new->from_service->type != TYPE_DAEMON &&
	   a_new->from_service->type != TYPE_RUNLEVEL)
	   {
	   F_("load_to_active(%s): service is not a service type!\n",
	   service_name);
	   if (a_new)
	   active_db_free(a_new);
	   return (NULL);
	   } */

	/* circular dependencies are BAD, so check this one before adding it to the db */
	/* dep_on_deep will not crash as long as the offending service is not loaded into the db */

	/* dep_on_deep will loop over the active_db, but as the a_new service is not added to 
	 * the service db up to now, this will not cause an endless loop, cause it will not
	 * loop over a_new */

	/* dep_on_deep() requires dep_on(), which uses string based comparison, and will completely
	 * ignore the active_db. This is at least true for the dep_on() check that's implemented by
	 * the depend plugin(). In future plug-ins, do NOT implement dep_on() code that will require
	 * any of the services being added to the active_db */
	if (g.no_circular == 0)
	{
		while_active_db(current)
		{
			if (initng_depend_deep(a_new, current)
				&& initng_depend_deep(current, a_new))
			{
				F_("load_to_active(%s): not loading service %s, because it has a circular dependency on %s\n", service_name, a_new->name, current->name);
				initng_active_db_free(a_new);
				return NULL;
			}
		}
	}

	/* return with pointer to service */
	if (initng_active_db_register(a_new) == TRUE)
		return (a_new);

	/* fail by default */
	F_("active_db_add() FAILED!\n");
	if (a_new)
		initng_active_db_free(a_new);
	return (NULL);
}



/* this function tries to find the from_service pointer from a service name.
   this actually loads the config from disk, parser by parser */
int initng_common_get_service(active_db_h * service)
{
	int i = 0;

	assert(service);
	assert(service->name);
	D_("get_service(%s);\n", service->name);

	/* check if service is set already */
	if (service->from_service)
	{
		D_("get_service(%s): already have a service!\n", service->name);
		return (TRUE);
	}

	/* try parse with dynamic loaded parsers */
	if (!(service->from_service = initng_common_parse_service(service->name)))
	{
		/* or fail */
		D_("get_service(%s): Can't get source.. \n", service->name);
		return (FALSE);
	}

	/* set some standard pointers */
	service->type = service->from_service->type;
	service->data.res = &service->from_service->data;

	D_(" get_service(%s): got service from initng dynamic parsers.\n",
	   service->name);

	/* will continue here only, if initng_modules_parse did succeed */

	/* if the name of service is starting on a / chars, its a full filename
	 * and the service name cant be that name
	 */
	if (service->name[0] == '/')
	{
		/* before renaming, we must check so there dont exist a service with this name */
		if (initng_active_db_find_by_exact_name(service->from_service->name))
		{
			/* bail out */
			initng_service_cache_free(service->from_service);
			service->from_service = NULL;
			return (FALSE);
		}

		/*
		 * <jimmy.wennlund@gmail.com>
		 * TODO: we have to check all tese possible senarios!
		 *
		 * example 1, An i file containing a service.
		 * service->name == "/etc/initng/test.i" and service->from_service->name == "test"
		 *
		 * example 2, An i file containing multible services.
		 * service->name == "/etc/initng/system/coldplug.i" and service->from_service->name == "system/coldplug/pci"
		 *
		 * example 3, An i file containing a variable service. 
		 * service->name == "/etc/initng/net.i" and servce->from_service->name == "net/ *"
		 */

		/* Right now youst rip the from_service name right off */
		free(service->name);
		service->name = strdup(service->from_service->name);
	}

	/*
	 * if service->name is "samba", and service->from_service->name is "daemon/samba" 
	 * we have to update service->name.
	 * also notice that service->name should not exactly match service->from_service->name, 
	 * service->name might be "eth0" and service->from_service->name might be "net / *".
	 */

	/* cont the tokens to the first '/' */
	while (service->from_service->name[i]
		   && service->from_service->name[i] != '/')
		i++;

	/* if there was no '/' don't complain */
	if (service->from_service->name[i] != '/')
		return (TRUE);

	/* compare the first i chars */
	if (strncmp(service->from_service->name, service->name, i) != 0)
	{
		D_("Name %s ..\n", service->name);
		char *new_name = (char *) i_calloc(strlen(service->name) + i + 3,
										   sizeof(char));

		strncpy(new_name, service->from_service->name, i + 1);
		strcat(new_name, service->name);

		/* before renaming, we must check so there dont exist a service with this name */
		if (initng_active_db_find_by_exact_name(new_name))
		{
			/* bail out */
			initng_service_cache_free(service->from_service);
			service->from_service = NULL;
			free(new_name);
			return (FALSE);
		}

		/* ok update new name */
		free(service->name);
		service->name = new_name;
		D_(" Is now: %s \n", service->name);
	}

	/* return happily */
	return (TRUE);

}

service_cache_h *initng_common_parse_service(const char *name)
{
	s_call *current, *safe = NULL;
	service_cache_h *service = NULL;

	D_("initng_common_parse_service(%s);\n", name);

	/* first check service cache */
	service = initng_service_cache_find_by_name(name);
	if (service)
	{
		/* May be there is plug-ins that will parse extra data */
		current = NULL;
		safe = NULL;
		while_list_safe(current, &g.ADDITIONAL_PARSE, safe)
		{
			if (((*current->c.additional_parse) (service)) != TRUE)
			{
				initng_service_cache_free(service);
				return (NULL);
			}
		}

		return (service);
	}

	/* try parse with dynamic loaded parsers */
	while_list_safe(current, &g.PARSERS, safe)
	{
		service = ((*current->c.parser) (name));

		/* break if we found the service */
		if (service)
			break;

	}

	/* look so we actually got one */
	if (!service)
		return (NULL);

	/* May be there is plug-ins that will parse extra data */
	current = NULL;
	safe = NULL;
	while_list_safe(current, &g.ADDITIONAL_PARSE, safe)
	{
		if (((*current->c.additional_parse) (service)) != TRUE)
		{
			initng_service_cache_free(service);
			return (NULL);
		}
	}

	/* return the service */
	return (service);
}

/*
 * Use this function to change the status of an service, this
 * function might refuse to change to that state, and if that
 * it will return FALSE, please always check the return value
 * when calling this function.
 */
int initng_common_mark_service(active_db_h * service, a_state_h * state)
{
	assert(service);
	assert(service->name);
	assert(service->current_state);
	assert(state);

	D_("going to mark_service(%s) from %s to %s\n", service->name,
	   service->current_state->state_name, state->state_name);

	/* 1: Test if already set */
	if (service->current_state == state)
	{
		D_("warning, this state %s is already set on %s!\n",
		   state->state_name, service->name);
		/* it is actually already set to requested state */
		return (TRUE);
	}

	D_(" %-20s : %10s -> %-10s\n", service->name,
	   service->current_state->state_name, state->state_name);

	/* Fill last entries */
	service->last_state = service->current_state;
	memcpy(&service->time_last_state, &service->time_current_state,
		   sizeof(struct timeval));

	/* update rough last to */
	if (service->last_rought_state != service->current_state->is)
	{
		service->last_rought_state = service->current_state->is;
		memcpy(&service->last_rought_time, &service->time_current_state,
			   sizeof(memcpy));
	}

	/* reset alarm, set state and time */
	service->alarm = 0;
	service->current_state = state;
	gettimeofday(&service->time_current_state, NULL);

	/* Set INTERRUPT, the interrupt is set only when a service
	 * changes state, and all state handlers will be called
	 */
	list_add(&service->interrupt, &g.active_database.interrupt);


	/* return success */
	return (TRUE);
}
