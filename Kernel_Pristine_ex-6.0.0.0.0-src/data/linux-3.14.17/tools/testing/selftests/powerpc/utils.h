/*
 * Copyright 2013, Michael Ellerman, IBM Corp.
 * Licensed under GPLv2.
 */

#ifndef _SELFTESTS_POWERPC_UTILS_H
#define _SELFTESTS_POWERPC_UTILS_H

#include <stdint.h>
#include <stdbool.h>

/* Avoid headaches with PRI?64 - just use %ll? always */
typedef unsigned long long u64;
typedef   signed long long s64;

/* Just for familiarity */
typedef uint32_t u32;
typedef uint8_t u8;


int test_harness(int (test_function)(void), char *name);


/* Yes, this is evil */
#define FAIL_IF(x)						\
do {								\
	if ((x)) {						\
		fprintf(stderr,					\
		"[FAIL] Test FAILED on line %d\n", __LINE__);	\
		return 1;					\
	}							\
} while (0)

#endif /* _SELFTESTS_POWERPC_UTILS_H */
