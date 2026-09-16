#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>  // IWYU pragma: export


#define __stringify_detail(a) #a
#define __stringify(a) __stringify_detail(a)

#define clox_report(...) do { \
		fprintf(stderr, "["  __FILE__ ":" \
			__stringify(__LINE__) "] " \
			__VA_ARGS__); \
		fputc('\n', stderr); \
	} while(0)

#define clox_assert(c, ...) do { \
		if (!(c)) { \
			clox_fatal("Assertion failed for: " \
				   __stringify(c) "\n> " __VA_ARGS__); \
		} \
	} while(0)

#if __unix__

#include <signal.h>  // IWYU pragma: export

#define clox_fatal(...) do { \
		clox_report(__VA_ARGS__); \
		raise(SIGINT); /* SIGINT for debugging */ \
	} while(0)

#else

#include <stdlib.h>  // IWYU pragma: export

#define clox_fatal(...) do { \
		clox_report(__VA_ARGS__); \
		exit(EXIT_FAILURE); \
	} while(0)

#endif

/* warn unused result */
#if defined(__GNUC__) || defined(__clang__)
#define _wur __attribute__((warn_unused_result))
#else
#define _wur
#endif


#endif
