#include "../include/clox.h"
#include "../include/instructions.h"
#include "../include/vm.h"

#include "../vendor/libfun/include/stack.h"

#include <stdbool.h>
#include <stdio.h>


struct chunk_line_info {
	int line;
	int count;
};


void vm_xinit(struct vm *vm)
{
	fstack_xinit(&vm->chunk, 1);
	fstack_xinit(&vm->chunk_line_info, sizeof(struct chunk_line_info));

	fstack_xinit(&vm->constants, sizeof(struct clox_value));
	fstack_xinit(&vm->stack, sizeof(struct clox_value));

	vm->pc = 0;
}

void vm_destroy(struct vm *vm)
{
	fstack_destroy(&vm->chunk);
	fstack_destroy(&vm->chunk_line_info);

	fstack_destroy(&vm->constants);
	fstack_destroy(&vm->stack);
}

void vm_chunk_xwrite(struct vm *vm, int line, char *chunk, size_t len)
{
	if (!len)
		return;

	/* TODO: multipush */
	for (size_t i = 0; i < len; i++)
		fstack_xpush(&vm->chunk, &chunk[i]);

	bool count_incremented = false;

	if (fstack_len(&vm->chunk_line_info) > 0) {
		struct chunk_line_info *cli = fstack_top(&vm->chunk_line_info);

		if (cli->line == line) {
			cli->count += len;
			count_incremented = true;
		}
	}

	if (!count_incremented)
		fstack_xpush(&vm->chunk_line_info,
			     &(struct chunk_line_info) {
				.line = line,
				.count = len
			     });
}

void vm_chunk_xwrite_inst(struct vm *vm,
				 int line,
				 struct inst ins)
{
	vm_chunk_xwrite(vm, line, &(char) { ins.op }, 1);
	vm_chunk_xwrite(vm, line, ins.args, inst_arg_len(ins.op));
}

void vm_disassemble(struct vm *vm, FILE *out, size_t offset, size_t len)
{
	if (fstack_len(&vm->chunk) == 0)
		return;

	size_t cli_index = 0;
	size_t total_instructions = 0;
	int prev_line = -1;
	int line;

	for (size_t i = 0;
	     (len == 0 || i < len) && offset + i < fstack_len(&vm->chunk);) {
		for (; total_instructions <= offset + i; cli_index++) {
			struct chunk_line_info *cli =
				fstack_at(&vm->chunk_line_info, cli_index);

			total_instructions += cli->count;
			line = cli->line;
		}

		struct inst ins = vm_chunk_read_inst(vm, offset + i);
		i += 1 + inst_arg_len(ins.op);

		fprintf(out, "%04zu ", offset + i);
		inst_print(ins, out, prev_line == line ? -1 : line);

		prev_line = line;
	}
}

struct inst vm_chunk_read_inst(struct vm *vm, size_t offset)
{
	char *inst_bytes = fstack_at(&vm->chunk, offset);

	return (struct inst) {
		.op = (enum opcode) inst_bytes[0],
		.args = inst_bytes + 1
	};
}
