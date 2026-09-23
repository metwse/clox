#ifndef STR_POOL_H
#define STR_POOL_H

#include <stdint.h>

#define T char, uint32_t, str_ids
#include "../vendor/libfun/include/hmap.h"

#define T uint32_t, struct fhmap_str_ids_entry_mut, str_chars
#include "../vendor/libfun/include/hmap.h"


/* String deduplication pool. */
struct str_pool {
	struct fhmap_str_ids ids;
	struct fhmap_str_chars chars;

	uint32_t last_id;
};


/* Initializes a new string pool. */
void str_pool_xinit(struct str_pool *);

/* Release resources owned by the string pool. */
void str_pool_destroy(struct str_pool *);

/* Returns id of the string or allocates a new id for it.
 * Note: String should not be null-terminated. */
uint32_t str_pool_xget_id(struct str_pool *, const char *chars, size_t len);

/* Returns true if the string is found.
 * Note: Returned str will not be null-terminated. */
bool str_pool_get_chars(const struct str_pool *,
			uint32_t str_id,
			const char **out_chars,
			size_t *out_len);


#endif
