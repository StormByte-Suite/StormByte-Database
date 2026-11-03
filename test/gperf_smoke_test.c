#include "gperf_smoke_generated.h"

int main(void)
{
	const struct test_word *alpha = in_word_set("alpha", 5);
	const struct test_word *omega = in_word_set("omega", 5);

	if (!alpha || alpha->value != 1 || !omega || omega->value != 2)
		return 1;
	if (in_word_set("alph", 4) || in_word_set("other", 5) || in_word_set("", 0))
		return 1;
	return 0;
}
