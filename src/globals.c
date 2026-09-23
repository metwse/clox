#include "../include/globals.h"
#include "../include/value.h"

#include <stdint.h>


void globals_xinit(struct globals *g)
{
	fhmap_global_vals_xinit(&g->global_vals);
	fhmap_str_id_to_global_id_xinit(&g->m1);
	fhmap_global_id_to_str_id_xinit(&g->m2);
	fhmap_uninitialized_globals_xinit(&g->m3);

	fstack_recycle_global_ids_xinit(&g->recycle_global_ids);

	g->last_id = 0;
}

void globals_destroy(struct globals *g)
{
	fhmap_global_vals_destroy(&g->global_vals);
	fhmap_str_id_to_global_id_destroy(&g->m1);
	fhmap_global_id_to_str_id_destroy(&g->m2);
	fhmap_uninitialized_globals_destroy(&g->m3);

	fstack_recycle_global_ids_destroy(&g->recycle_global_ids);
}

void globals_define(struct globals *g, uint32_t global_id, struct val v)
{
	fhmap_global_vals_insert2(&g->global_vals, &global_id, &v);
	/* We've defined the global and initialized it, so we can remove it
	 * from unitialized globals list. It no longer is subject to garbage
	 * collection. */
	fhmap_uninitialized_globals_remove2(&g->m3, &global_id, NULL);
}

struct val *globals_get(struct globals *g, uint32_t global_id)
{
	return fhmap_global_vals_get2_mut(&g->global_vals, &global_id);
}

bool globals_remove(struct globals *g, uint32_t global_id, struct val *out_val)
{
	return fhmap_global_vals_remove2(&g->global_vals, &global_id, out_val);
}

/* TODO: gc */
/* Detach global ID from its string ID. */
/* static void globals_free_id(struct globals *g, uint32_t global_id); */

uint32_t globals_xget_global_id(struct globals *g, uint32_t str_id)
{
	const uint32_t *global_id =
		fhmap_str_id_to_global_id_get2(&g->m1, &str_id);

	if (global_id != NULL)
		return *global_id;
	else {
		uint32_t new_global_id;
		if (fstack_recycle_global_ids_len(&g->recycle_global_ids) >  0)
			new_global_id =
				*fstack_recycle_global_ids_pop(&g->recycle_global_ids);
		else
			new_global_id = g->last_id++;

		fhmap_str_id_to_global_id_insert2(&g->m1,
						  &str_id,
						  &new_global_id);
		fhmap_uninitialized_globals_insert2(&g->m3,
						    &new_global_id,
						    &(bool) { false });

		return new_global_id;
	}
}
