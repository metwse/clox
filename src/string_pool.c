#include "../include/string_pool.h"

#include <stdint.h>


void str_pool_xinit(struct str_pool *p)
{
	p->last_id = 0;
	fhmap_str_ids_xinit(&p->ids);
	fhmap_str_chars_xinit(&p->chars);
}

void str_pool_destroy(struct str_pool *p)
{
	fhmap_str_ids_destroy(&p->ids);
	fhmap_str_chars_destroy(&p->chars);
}

uint32_t str_pool_xget_id(struct str_pool *p, const char *chars, size_t len)
{
	if (len == 0)
		return UINT32_MAX;

	const uint32_t *id = fhmap_str_ids_get(&p->ids, chars, len);

	if (id == NULL) {
		uint32_t new_id = p->last_id++;

		struct fhmap_str_ids_entry_mut e;
		fhmap_str_ids_xinserte(&p->ids, chars, len, &new_id, &e);
		fhmap_str_chars_xinsert2(&p->chars, &new_id, &e);

		return new_id;
	} else {
		return *id;
	}
}

bool str_pool_get_chars(const struct str_pool *p,
			uint32_t str_id,
			const char **out_chars,
			size_t *out_len)
{
	if (str_id == UINT32_MAX) {
		*out_len = 0;
		return true;
	}

	const struct fhmap_str_ids_entry_mut *e =
		fhmap_str_chars_get2(&p->chars, &str_id);

	if (e == NULL) {
		return false;
	} else {
		*out_chars = e->key;
		*out_len = e->key_len;

		return true;
	}
}
