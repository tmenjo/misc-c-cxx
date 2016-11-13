#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include <check.h>

#include "nsutil-inl.h"

#define EOK 0

static void assert_errno(int err)
{
	const int e = errno;
	ck_assert_int_eq(err, e);
}

static void assert_parse_decimal_long(int err, long ret, const char *str)
{
	errno = EOK;
	const long r = parse_decimal_long(str);
	assert_errno(err);
	ck_assert_int_eq(ret, r);
}

static void assert_parse_decimal_long_einval(const char *str)
{
	errno = EOK;
	parse_decimal_long(str);
	assert_errno(EINVAL);
}

static void assert_parse_decimal_long_border(
	int err, void (*altstr)(char *str), long border)
{
	char str[64];
	errno = 0;
	snprintf(str, sizeof(str), "%ld", border);
	assert_errno(EOK);

	if(altstr)
		altstr(str);

	assert_parse_decimal_long(err, border, str);
}

static void add_one(char *str)
{
	const size_t len = strlen(str);
	ck_assert_uint_lt(0, len);
	++str[len - 1];
}

START_TEST(test_parse_decimal_long)
{
	assert_parse_decimal_long(EOK,  0L,   "0");
	assert_parse_decimal_long(EOK,  1L,   "1");
	assert_parse_decimal_long(EOK, -1L,  "-1");
	assert_parse_decimal_long(EOK,  2L,  "+2");
	assert_parse_decimal_long(EOK,  3L,  " 3");
	assert_parse_decimal_long(EOK,  4L, " +4");
	assert_parse_decimal_long(EOK, -5L, " -5");
}
END_TEST

START_TEST(test_parse_decimal_long_einval)
{
	assert_parse_decimal_long_einval(" ");
	assert_parse_decimal_long_einval("+");
	assert_parse_decimal_long_einval("-");
	assert_parse_decimal_long_einval("a");
	assert_parse_decimal_long_einval("42 ");
	assert_parse_decimal_long_einval("+ 1");
	assert_parse_decimal_long_einval("- 2");
	assert_parse_decimal_long_einval("++3");
	assert_parse_decimal_long_einval("--4");
}
END_TEST

START_TEST(test_parse_decimal_long_border)
{
	assert_parse_decimal_long_border(EOK, NULL, LONG_MAX);
	assert_parse_decimal_long_border(EOK, NULL, LONG_MIN);
}
END_TEST

START_TEST(test_parse_decimal_long_erange)
{
	assert_parse_decimal_long_border(ERANGE, add_one, LONG_MAX);
	assert_parse_decimal_long_border(ERANGE, add_one, LONG_MIN);
}
END_TEST

START_TEST(test_get_second_part)
{
	ck_assert_int_eq(0L, get_second_part(         0L));
	ck_assert_int_eq(0L, get_second_part(         1L));
	ck_assert_int_eq(0L, get_second_part( 500000000L));
	ck_assert_int_eq(0L, get_second_part( 999999999L));
	ck_assert_int_eq(1L, get_second_part(1000000000L));
	ck_assert_int_eq(1L, get_second_part(1000000001L));
	ck_assert_int_eq(2L, get_second_part(2000000000L));
}
END_TEST

START_TEST(test_get_nanosecond_part)
{
	ck_assert_int_eq(        0L, get_nanosecond_part(         0L));
	ck_assert_int_eq(        1L, get_nanosecond_part(         1L));
	ck_assert_int_eq(500000000L, get_nanosecond_part( 500000000L));
	ck_assert_int_eq(999999999L, get_nanosecond_part( 999999999L));
	ck_assert_int_eq(        0L, get_nanosecond_part(1000000000L));
	ck_assert_int_eq(        1L, get_nanosecond_part(1000000001L));
	ck_assert_int_eq(        0L, get_nanosecond_part(2000000000L));
}
END_TEST

int main()
{
	TCase *const tcase1 = tcase_create("parse_decimal_long");
	tcase_add_test(tcase1, test_parse_decimal_long);
	tcase_add_test(tcase1, test_parse_decimal_long_einval);
	tcase_add_test(tcase1, test_parse_decimal_long_border);
	tcase_add_test(tcase1, test_parse_decimal_long_erange);
	TCase *const tcase2 = tcase_create("get_second_part");
	tcase_add_test(tcase2, test_get_second_part);
	TCase *const tcase3 = tcase_create("get_nanosecond_part");
	tcase_add_test(tcase3, test_get_nanosecond_part);

	Suite *const suite = suite_create("nsutil");
	suite_add_tcase(suite, tcase1);
	suite_add_tcase(suite, tcase2);
	suite_add_tcase(suite, tcase3);

	SRunner *const srunner = srunner_create(suite);
	srunner_run_all(srunner, CK_NORMAL);
	const int failed = srunner_ntests_failed(srunner);
	srunner_free(srunner);

	return !!failed;
}
