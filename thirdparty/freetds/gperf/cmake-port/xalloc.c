#include <config.h>
#include "xalloc.h"

#include <stdio.h>
#include <stdlib.h>

_Noreturn void xalloc_die(void)
{
	fputs("gperf: memory exhausted\n", stderr);
	exit(EXIT_FAILURE);
}
