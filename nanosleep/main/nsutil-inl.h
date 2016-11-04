#ifndef NSUTIL_INL_H
#define NSUTIL_INL_H

#include <errno.h>
#include <stdlib.h>

#define SECOND_IN_NANOSECOND 1000000000L

inline long parse_decimal_long(const char *s)
{
	char *endptr = NULL;
	const long n = strtol(s, &endptr, 10);
	if (errno == 0 && *endptr)
		errno = EINVAL;
	return n;
}

inline long get_second_part(long nsec)
{
	return nsec / SECOND_IN_NANOSECOND;
}

inline long get_nanosecond_part(long nsec)
{
	return nsec % SECOND_IN_NANOSECOND;
}

#endif /* NSUTIL_INL_H */
