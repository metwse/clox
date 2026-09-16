#include "../include/instructions.h"
#include "../include/vm.h"


int main(void)
{
	struct vm vm;

	vm_xinit(&vm);

	vm_disassemble(&vm, stderr, 0, 0);

	vm_chunk_xwrite_inst(&vm,
			     1,
			     (struct inst) {
				     .op = OP_RETURN
			     });

	vm_chunk_xwrite_inst(&vm,
			     1,
			     (struct inst) {
				    .op = OP_CONSTANT,
				    .args = &(char) { 1 }
			     });

	vm_chunk_xwrite_inst(&vm,
			     2,
			     (struct inst) {
				    .op = OP_CONSTANT_LONG,
				    .args = &(char[3]) { 1, 0, 0 }
			     });

	vm_chunk_xwrite_inst(&vm, 3, (struct inst) { .op = OP_RETURN, });
	vm_chunk_xwrite_inst(&vm, 3, (struct inst) { .op = OP_RETURN, });
	vm_chunk_xwrite_inst(&vm, 4, (struct inst) { .op = OP_RETURN, });
	vm_chunk_xwrite_inst(&vm, 6, (struct inst) { .op = OP_RETURN, });

	vm_disassemble(&vm, stderr, 0, 0);
	vm_disassemble(&vm, stderr, 1, 2);

	vm_destroy(&vm);
}
