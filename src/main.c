#include "../include/common.h"
#include "../include/interpreter.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>


static char *read_file(const char *path)
{
	FILE *file = fopen(path, "rb");

	fseek(file, 0L, SEEK_END);
	size_t file_size = ftell(file);
	rewind(file);

	char *buffer = malloc(file_size + 1);
	clox_assert(buffer, "cannot allocate buffer for file content");
	size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
	buffer[bytes_read] = '\0';

	fclose(file);
	return buffer;
}

static void repl(struct interpreter *i) {
	char line[1024];

	while (true) {
		printf("> ");

		if (!fgets(line, sizeof(line), stdin)) {
			printf("\n");
			break;
		}

		interpreter_run(i, line);
	}
}


int main(int argc, char *argv[])
{
	if (argc > 2)
		clox_fatal("Usage %s [script]", argv[0]);

	interpreter_xstatic_init();

	struct interpreter i;

	interpreter_xinit(&i);

	if (argc == 2) {
		char * file_content = read_file(argv[1]);

		interpreter_run(&i, file_content);

		free(file_content);
	}

	repl(&i);

	interpreter_destroy(&i);

	interpreter_static_destroy();
}
