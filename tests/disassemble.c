#include "../include/instructions.h"
#include "../include/chunk.h"


int main(void)
{
	struct chunk c;

	chunk_xinit(&c);

	chunk_disassemble(&c, stderr, 0, 0);

	chunk_xwrite_inst(&c, 1, (struct inst) { .op = OP_RETURN });

	chunk_xwrite_inst(&c,
			  1,
			  (struct inst) {
				    .op = OP_CONSTANT,
				    .args = &(char) { 1 }
			  });

	chunk_xwrite_inst(&c,
			  2,
			  (struct inst) {
				  .op = OP_CONSTANT_LONG,
				  .args = &(char[3]) { 1, 0, 0 }
			  });

	chunk_xwrite_inst(&c, 3, (struct inst) { .op = OP_RETURN, });
	chunk_xwrite_inst(&c, 3, (struct inst) { .op = OP_RETURN, });
	chunk_xwrite_inst(&c, 4, (struct inst) { .op = OP_RETURN, });
	chunk_xwrite_inst(&c, 6, (struct inst) { .op = OP_RETURN, });

	chunk_disassemble(&c, stderr, 0, 0);
	chunk_disassemble(&c, stderr, 1, 2);

	chunk_destroy(&c);
}
