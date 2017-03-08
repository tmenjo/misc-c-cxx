#define _GNU_SOURCE /* tdestroy from search.h */

#include <search.h>
#include <stdlib.h>

#include <check.h>
#include "checkutil-inl.h"

static void do_nothing(void *dummy)
{
	(void)dummy;
}

static int int_less(const void *lhs, const void *rhs)
{
	const int a = *(const int *)lhs;
	const int b = *(const int *)rhs;
	return (a < b) ? -1 : ((a > b) ? 1 : 0);
}

START_TEST(test_tsearch)
{
	void *root = NULL;

	const int a = 'A';
	const int *const *const app = tsearch(&a, &root, int_less);
	assert_not_nullptr(app);
	ck_assert_ptr_eq(&a, *app);
	ck_assert_int_eq('A', **app);

	tdestroy(root, do_nothing);
}
END_TEST

START_TEST(test_tfind)
{
	void *root = NULL;

	const int a = 'A';
	assert_nullptr(tfind(&a, &root, int_less));
	assert_not_nullptr(tsearch(&a, &root, int_less));

	const int b = 'B';
	assert_nullptr(tfind(&b, &root, int_less));

	const int k = 'A';
	const int *const *const app = tfind(&k, &root, int_less);
	assert_not_nullptr(app);
	ck_assert_ptr_eq(&a, *app);
	ck_assert_int_eq('A', **app);

	tdestroy(root, do_nothing);
}
END_TEST

START_TEST(test_tdelete)
{
	void *root = NULL;

	const int a = 'A';
	assert_nullptr(tdelete(&a, &root, int_less));
	assert_not_nullptr(tsearch(&a, &root, int_less));

	const int b = 'B';
	assert_nullptr(tdelete(&b, &root, int_less));

	const int k = 'A';
	assert_not_nullptr(tdelete(&k, &root, int_less));

	tdestroy(root, do_nothing);
}
END_TEST

START_TEST(test_tdestroy)
{
	void *root = NULL;

	int *const a = malloc(sizeof(int));
	assert_not_nullptr(a);
	*a = 'A';
	assert_not_nullptr(tsearch(a, &root, int_less));

	int *const b = malloc(sizeof(int));
	assert_not_nullptr(b);
	*b = 'B';
	assert_not_nullptr(tsearch(b, &root, int_less));

	int *const c = malloc(sizeof(int));
	assert_not_nullptr(c);
	*c = 'C';
	assert_not_nullptr(tsearch(c, &root, int_less));

	tdestroy(root, free);
}
END_TEST

START_TEST(test_dtor)
{
	void *root = NULL;

	/* allocate A and add it into the tree */
	int *const a = malloc(sizeof(int));
	*a = 'A';
	assert_not_nullptr(tsearch(a, &root, int_less));

	/* get a pointer to A; it is not removed yet */
	const int k = 'A';
	int **const app = tfind(&k, &root, int_less);
	assert_not_nullptr(app);
	int *const ap = *app;
	ck_assert_ptr_eq(a, ap);

	/* remove A from the tree then free it */
	assert_not_nullptr(tdelete(&k, &root, int_less));
	free(ap);

	tdestroy(root, free);
}
END_TEST

int main()
{
	TCase *const tcase = tcase_create("all");
	tcase_add_test(tcase, test_tsearch);
	tcase_add_test(tcase, test_tfind);
	tcase_add_test(tcase, test_tdelete);
	tcase_add_test(tcase, test_tdestroy);
	tcase_add_test(tcase, test_dtor);

	Suite *const suite = suite_create("tsearch");
	suite_add_tcase(suite, tcase);

	SRunner *const srunner = srunner_create(suite);
	srunner_run_all(srunner, CK_NORMAL);
	const int failed = srunner_ntests_failed(srunner);
	srunner_free(srunner);

	return !!failed;
}
