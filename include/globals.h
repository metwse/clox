#ifndef GLOBALS_H
#define GLOBALS_H


#include "value.h"

#include <stdbool.h>
#include <stdint.h>


#define T uint32_t, struct val, global_vals
#include "../vendor/libfun/include/hmap.h"

#define T uint32_t, uint32_t, str_id_to_global_id
#include "../vendor/libfun/include/hmap.h"

#define T uint32_t, uint32_t, global_id_to_str_id
#include "../vendor/libfun/include/hmap.h"

#define T uint32_t, bool, uninitialized_globals
#include "../vendor/libfun/include/hmap.h"

#define T uint32_t, recycle_global_ids
#include "../vendor/libfun/include/stack.h"


/* Global variables. */
struct globals {
	/* global_id -> value */
	struct fhmap_global_vals global_vals;
	/* global_id -> str_id */
	struct fhmap_str_id_to_global_id m1;
	/* str_id -> global_id */
	struct fhmap_global_id_to_str_id m2;
	/* global_id -> is marked */
	struct fhmap_uninitialized_globals m3;

	/* Reassign previously released ID's instead of incrementing last_id. */
	struct fstack_recycle_global_ids recycle_global_ids;
	uint32_t last_id;
};

/* Noes on garbage collection:
 * str_id -> global_id is subject to garbage collection if the globas has been
 * deleted and there is no chunk refering to that global. Chunks should keep
 * track of referenced globals, and if a global is not initialized and no chunk
 * references it, its str_id - global_id mapping will be removed. */


/* Creates a new global variable table. */
void globals_xinit(struct globals *);

/* Releases resourced owned by the global table. */
void globals_destroy(struct globals *);

/* Defines a global variable. Note: The variable should not exist. */
void globals_define(struct globals *, uint32_t global_id, struct val v);

/* Gets a global variable. */
struct val *globals_get(struct globals *, uint32_t global_id);

/* Removes a global variable, returns true if a value is found and removed. */
bool globals_remove(struct globals *, uint32_t global_id, struct val *out_val);


#endif
