#include <check.h>

int foo(int, int, int);

START_TEST(test_foo)
{
    ck_assert_int_eq(11, foo(2, 3, 5));
}
END_TEST

int main()
{
    TCase *const tcase = tcase_create("foo");
    tcase_add_test(tcase, test_foo);
    Suite *const suite = suite_create("sub");
    suite_add_tcase(suite, tcase);

    SRunner *const srunner = srunner_create(suite);
    srunner_run_all(srunner, CK_NORMAL);
    const int failed = srunner_ntests_failed(srunner);
    srunner_free(srunner);

    return !!failed;
}
