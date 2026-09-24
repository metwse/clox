#include "../include/chunk.h"
#include "../include/gc.h"
#include "../include/globals.h"
#include "../include/object.h"
#include "../include/value.h"
#include "../include/vm.h"

#include <stddef.h>


static void mark_stack(struct vm *vm, struct fstack_vals *s, struct gc_result *);
static void mark_chunk(struct vm *vm, struct chunk *c, struct gc_result *);
static void mark_objs(struct vm *vm, struct obj *o, struct gc_result *);


static void mark_stack(struct vm *vm,
		       struct fstack_vals *s,
		       struct gc_result *res)
{
	for (size_t i = 0; i < fstack_vals_len(s); i++) {
		struct val v = *fstack_vals_at_mut(s, i);

		if (IS_OBJ(v))
			mark_objs(vm, AS_OBJ(v), res);
	}
}

static void mark_chunk(struct vm *vm, struct chunk *c, struct gc_result *res)
{
	mark_stack(vm, &c->constants, res);
}

static void mark_objs(struct vm *vm, struct obj *o, struct gc_result *res)
{
	if (o->is_marked)
		return;
	o->is_marked = true;

	res->objs++;

	switch (OBJ_TYPE(o)) {
	case OBJ_FUNCTION: {
		struct obj_function *function = AS_FUNCTION(o);
		mark_chunk(vm, &function->chunk, res);

		break;
	}

	case OBJ_CLOSURE: {
		struct obj_closure *closure = AS_CLOSURE(o);

		mark_objs(vm, (struct obj *) closure->function, res);
		for (size_t i = 0; i < closure->function->upvalue_count; i++)
			mark_objs(vm, (struct obj *) closure->upvalues[i], res);

		break;
	}

	case OBJ_UPVALUE: {
		struct obj_upvalue *upvalue = AS_UPVALUE(o);

		if (!upvalue->is_local && IS_OBJ(upvalue->location.captured))
			mark_objs(vm, AS_OBJ(upvalue->location.captured), res);

		break;
	}

	case OBJ_NATIVE_FUNCTION:
		break;

	case OBJ_STR_LITERAL:
		/* TODO: mark str_literal */
		break;
	}
}

struct gc_result vm_gc_mark(struct vm *vm)
{
	struct gc_result res = {
		.strs = 0,
		.objs = 0,
		.global_ids = 0
	};

	mark_stack(vm, &vm->stack, &res);

	struct fhmap_global_vals_it_mut it =
		fhmap_global_vals_iter_mut(&vm->globals->global_vals);
	struct fhmap_global_vals_entry_mut e;
	while (fhmap_global_vals_iter_next_mut(&it, &e)) {
		/* TODO: mark global_id, str_id */
		struct val v = *e.value;
		if (IS_OBJ(v))
			mark_objs(vm, AS_OBJ(v), &res);
	}

	for (size_t i = 0; i < fstack_call_frames_len(&vm->frames); i++) {
		struct call_frame *frame =
			fstack_call_frames_at_mut(&vm->frames, i);

		if (frame->closure)
			mark_objs(vm, (struct obj *) frame->closure, &res);
		else
			mark_chunk(vm, (struct chunk *) frame->c, &res);
	}

	if (vm->current.closure)
		mark_objs(vm, (struct obj *) vm->current.closure, &res);
	else
		mark_chunk(vm, (struct chunk *) vm->current.c, &res);

	return res;
}

void vm_gc_sweep(struct vm *vm)
{
	for (size_t i = 0; i < fstack_objects_len(&vm->objects); i++) {
		struct obj **o = fstack_objects_at_mut(&vm->objects, i);

		if ((*o)->is_marked) {
			(*o)->is_marked = false;
		} else {
			obj_free(*o);

			if (i < fstack_objects_len(&vm->objects) - 1) {
				*o = *fstack_objects_top(&vm->objects);
				i--;
			}

			fstack_objects_pop(&vm->objects);
		}
	}
}
