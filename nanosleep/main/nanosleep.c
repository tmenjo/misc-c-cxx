#include <errno.h>
#include <time.h>

#include "nsutil-inl.h"

int main(int argc, char **argv)
{
	if (argc < 2)
		return 1;

	const long nsec = parse_decimal_long(argv[1]);
	if (errno || nsec < 0)
		return 1;

	const struct timespec req = {
		.tv_sec  = (time_t)get_second_part(nsec),
		.tv_nsec = get_nanosecond_part(nsec),
	};
	struct timespec rem = { 0 };

	return !!nanosleep(&req, &rem);
}
