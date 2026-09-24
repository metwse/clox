#ifndef GC_H
#define GC_H

#include <stddef.h>

struct vm;  /* defined in vm.h */


struct gc_result {
	size_t strs;
	size_t objs;
	size_t global_ids;
};


/* Mark unreachable objects, strings, and global IDs. Returns reachable
 * object counts. */
struct gc_result vm_gc_mark(struct vm *);

/* Clear unreachable objects. */
void vm_gc_sweep(struct vm *);


#endif
