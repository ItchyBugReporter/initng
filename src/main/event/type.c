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

#include <string.h>
#include <assert.h>
#include <initng.h>

/*
 * This function adds an event to event_db.
 */
void initng_event_type_register(s_event_type * ent)
{
	assert(ent);

	S_;

	if (ent->name)
		ent->name_len = strlen(ent->name);
	else
		ent->name_len = 0;

	/* initialize the event types list */
	initng_list_init(&ent->list);

	/* initialize the event hooks list */
	initng_list_init(&ent->hooks.list);

#ifdef CHECK_IF_CURRENTLY_ADDED
	{
		s_event_type *current = NULL;

		/* walk the event_db, and make sure its not added, or name taken */
		while_service_event_types(current) {
			if (current == ent) {
				if (ent->name) {
					F_("Option %s, already added!\n",
					   ent->name);
				} else {
					F_("Option, already added!\n");
				}

				return;
			}

			if (current->name && ent->name &&
			    strcmp(current->name, ent->name) == 0) {
				F_("option %s, name taken.\n");
				return;
			}
		}
	}
#endif

	/* add the event to the event_db list */
	initng_list_add(&ent->list, &g.event_db.list);
#ifdef DEBUG
	if (ent->name)
		D_(" \"%s\" added to option_db!\n", ent->name);
#endif
}

/*
 * This safely deletes an event from event_db.
 */
void initng_event_type_unregister(s_event_type * ent)
{
	initng_list_del(&ent->list);
}

/*
 * This walks the event_db removing ALL entries.
 */
void initng_event_type_unregister_all(void)
{
	s_event_type *current, *safe = NULL;

	/* walk the event db, remove all */
	while_event_types_safe(current, safe) {
		initng_event_type_unregister(current);
	}

	/* make sure the list is cleared! */
	initng_list_init(&g.event_db.list);
}

/*
 * Use this function to search a pointer in the
 * event_db
 */
s_event_type *initng_event_type_find(const char *string)
{
	s_event_type *current = NULL;

	S_;
	assert(string);
	D_("looking for %s.\n", string);
	while_event_types(current) {
		if (current->name && strcmp(current->name, string) == 0)
			return current;
	}
	return NULL;
}
