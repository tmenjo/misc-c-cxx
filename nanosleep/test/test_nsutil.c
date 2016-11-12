#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include <check.h>

#include "nsutil-inl.h"

#define EOK 0

static void setup(void)
{
	errno = EOK;
}

static void assert_parse_decimal_long(int err, long ret, const char *str)
{
	errno = EOK;
	const long r = parse_decimal_long(str);
	const int e = errno;
	ck_assert_int_eq(ret, r);
	ck_assert_int_eq(err, e);
}

static void assert_parse_decimal_long_einval(const char *str)
{
	errno = EOK;
	parse_decimal_long(str);
	const int e = errno;
	ck_assert_int_eq(EINVAL, e);
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

static void subtest_parse_decimal_long_border(long border)
{
	char str[64];
	snprintf(str, sizeof(str), "%ld", border);
	const int e = errno;
	ck_assert_int_eq(EOK, e);

	assert_parse_decimal_long(EOK, border, str);
}

START_TEST(test_parse_decimal_long_border)
{
	subtest_parse_decimal_long_border(LONG_MAX);
	setup();
	subtest_parse_decimal_long_border(LONG_MIN);
}
END_TEST

START_TEST(test_parse_decimal_long_einval)
{
	assert_parse_decimal_long_einval(" ");
	assert_parse_decimal_long_einval("+");
	assert_parse_decimal_long_einval("-");
	assert_parse_decimal_long_einval("a");
	assert_parse_decimal_long_einval("42 ");
}
END_TEST

static void subtest_parse_decimal_long_erange(long border)
{
	char str[64];
	snprintf(str, sizeof(str), "%ld", border);
	const int e = errno;
	ck_assert_int_eq(EOK, e);

	const size_t len = strlen(str);
	ck_assert_uint_lt(0, len);
	++str[len - 1];

	assert_parse_decimal_long(ERANGE, border, str);
}

START_TEST(test_parse_decimal_long_erange)
{
	subtest_parse_decimal_long_erange(LONG_MAX);
	setup();
	subtest_parse_decimal_long_erange(LONG_MIN);
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
	tcase_add_checked_fixture(tcase1, setup, NULL);
	tcase_add_test(tcase1, test_parse_decimal_long);
	tcase_add_test(tcase1, test_parse_decimal_long_border);
	tcase_add_test(tcase1, test_parse_decimal_long_einval);
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
