#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include <check.h>

#include "nsutil-inl.h"

void setup(void)
{
	errno = 0;
}

START_TEST(test_parse_decimal_long)
{
	ck_assert_int_eq(0L, parse_decimal_long("0"));
	ck_assert_int_eq(0, errno);

	ck_assert_int_eq(1L, parse_decimal_long("1"));
	ck_assert_int_eq(0, errno);

	ck_assert_int_eq(-1L, parse_decimal_long("-1"));
	ck_assert_int_eq(0, errno);

	ck_assert_int_eq(2L, parse_decimal_long("+2"));
	ck_assert_int_eq(0, errno);

	ck_assert_int_eq(3L, parse_decimal_long(" 3"));
	ck_assert_int_eq(0, errno);

	ck_assert_int_eq(4L, parse_decimal_long(" +4"));
	ck_assert_int_eq(0, errno);

	ck_assert_int_eq(-5L, parse_decimal_long(" -5"));
	ck_assert_int_eq(0, errno);
}
END_TEST

static void subtest_parse_decimal_long_border(long border)
{
	char str[64];
	snprintf(str, sizeof(str), "%ld", border);
	ck_assert_int_eq(0, errno);

	ck_assert_int_eq(border, parse_decimal_long(str));
	ck_assert_int_eq(0, errno);
}

START_TEST(test_parse_decimal_long_border_positive)
{
	subtest_parse_decimal_long_border(LONG_MAX);
}
END_TEST

START_TEST(test_parse_decimal_long_border_negative)
{
	subtest_parse_decimal_long_border(LONG_MIN);
}
END_TEST

START_TEST(test_parse_decimal_long_einval)
{
	parse_decimal_long(" ");
	ck_assert_int_eq(EINVAL, errno);

	errno = 0;

	parse_decimal_long("+");
	ck_assert_int_eq(EINVAL, errno);

	errno = 0;

	parse_decimal_long("-");
	ck_assert_int_eq(EINVAL, errno);

	errno = 0;

	parse_decimal_long("a");
	ck_assert_int_eq(EINVAL, errno);

	errno = 0;

	parse_decimal_long("42 ");
	ck_assert_int_eq(EINVAL, errno);
}
END_TEST

static void subtest_parse_decimal_long_erange(long border)
{
	char str[64];
	snprintf(str, sizeof(str), "%ld", border);
	ck_assert_int_eq(0, errno);

	const size_t len = strlen(str);
	ck_assert(len != 0);
	++str[len - 1];

	ck_assert_int_eq(border, parse_decimal_long(str));
	ck_assert_int_eq(ERANGE, errno);
}

START_TEST(test_parse_decimal_long_erange_positive)
{
	subtest_parse_decimal_long_erange(LONG_MAX);
}
END_TEST

START_TEST(test_parse_decimal_long_erange_negative)
{
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
	tcase_add_test(tcase1, test_parse_decimal_long_border_positive);
	tcase_add_test(tcase1, test_parse_decimal_long_border_negative);
	tcase_add_test(tcase1, test_parse_decimal_long_einval);
	tcase_add_test(tcase1, test_parse_decimal_long_erange_positive);
	tcase_add_test(tcase1, test_parse_decimal_long_erange_negative);
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
