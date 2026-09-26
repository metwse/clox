#ifndef LW_CONFIG_H
#define LW_CONFIG_H


#include <stdint.h>
#include <inttypes.h>


/* Floating point type */
typedef double Lw_number_t;
#define Lw_str2number(str) strtod(str, NULL)
#define Lw_number_fmt "g"

/* Integer type. */
typedef int64_t Lw_integer_t;
#define Lw_str2integer(str) ((int64_t) strtoimax(str, NULL, 0))
#define Lw_integer_fmt PRIi64


#endif
