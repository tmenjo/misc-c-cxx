#include <pthread.h>
#include <errno.h>
#include <stdlib.h>

#include "concurrent_set.h"

#include <check.h>
#include "checkutil-inl.h"

static int int_comparator(const void *lhs, const void *rhs)
{
	const int a = *(const int *)lhs;
	const int b = *(const int *)rhs;
	return (a < b) ? -1 : ((a > b) ? 1 : 0);
}

static int *int_new(int value)
{
	int *const ret = malloc(sizeof(int));
	if (ret)
		*ret = value;
	return ret;
}

START_TEST(test_unique_bag)
{
	struct cs *const set = cs_new(int_comparator, NULL);
	assert_not_nullptr(set);
	ck_assert_int_eq(0, cs_size(set));

	/* NULL cannot be either added or removed */
	ck_assert_int_eq(-EINVAL, cs_add(set, NULL));
	ck_assert(!cs_contains(set, NULL));
	ck_assert_int_eq(0, cs_size(set));
	ck_assert_int_eq(-EINVAL, cs_remove(set, NULL));

	const int a = 3;
	assert_success(cs_add(set, &a));
	ck_assert_int_eq(1, cs_size(set));

	const int b = 3;
	ck_assert(cs_contains(set, &b));
	ck_assert_int_eq(EEXIST, cs_add(set, &b));
	ck_assert_int_eq(1, cs_size(set));

	const int c = 5;
	ck_assert(!cs_contains(set, &c));
	ck_assert_int_eq(ENOENT, cs_remove(set, &c));
	ck_assert_int_eq(1, cs_size(set));

	assert_success(cs_remove(set, &b));
	ck_assert_int_eq(0, cs_size(set));

	cs_destroy(set);
}
END_TEST

START_TEST(test_dtor)
{
	struct cs *const set = cs_new(int_comparator, free);
	assert_not_nullptr(set);

	int *const a = int_new(3);
	assert_not_nullptr(a);
	int *const b = int_new(2);
	assert_not_nullptr(b);
	int *const c = int_new(5);
	assert_not_nullptr(c);
	int *const d = int_new(7);
	assert_not_nullptr(d);

	assert_success(cs_add(set, a));
	assert_success(cs_add(set, b));
	assert_success(cs_add(set, c));
	assert_success(cs_add(set, d));
	ck_assert_int_eq(4, cs_size(set));

	assert_success(cs_remove(set, a));
	assert_success(cs_remove(set, b));
	ck_assert_int_eq(2, cs_size(set));

	cs_destroy(set);
}
END_TEST

#define NR_THREADS 8
static void *run(void *);
START_TEST(test_multithread)
{
	struct cs *const set = cs_new(int_comparator, free);
	ck_assert_ptr_ne(NULL, set);

	pthread_t threads[NR_THREADS];
	for (size_t i = 0; i < NR_THREADS; ++i) {
		assert_success(pthread_create(
			&threads[i], NULL, run, (void *)set));
	}

	void *ret = NULL;
	for (size_t i = 0; i < NR_THREADS; ++i) {
		assert_success(pthread_join(threads[i], &ret));
		assert_not_nullptr(ret);
		assert_success(*((int *)ret));
		free(ret);
	}

	cs_destroy(set);
}
END_TEST

static void *run(void *arg)
{
	struct cs *const set = (struct cs *)arg;

	int *const ret = int_new(0); /* assume success */
	if (!ret)
		goto out;

	for (int i = 0; i < 10000; ++i) {
		int *const p = int_new(i & 0xFF);
		if (!p) {
			*ret = -ENOMEM;
			goto out;
		}
		const int a = (*p == 0xFF) ? 0 : *p + 1;

		switch (cs_add(set, p)) {
		case EEXIST:
			free(p);
			break;
		case -ENOMEM:
			free(p);
			*ret = -ENOMEM;
			goto out;
		}

		if (cs_remove(set, &a) < 0) {
			*ret = -ENOENT;
			goto out;
		}
	}

out:
	return ret;
}

int main()
{
	TCase *const tcase = tcase_create("all");
	tcase_add_test(tcase, test_unique_bag);
	tcase_add_test(tcase, test_dtor);
	tcase_add_test(tcase, test_multithread);

	Suite *const suite = suite_create("concurrent_set");
	suite_add_tcase(suite, tcase);

	SRunner *const srunner = srunner_create(suite);
	srunner_run_all(srunner, CK_NORMAL);
	const int failed = srunner_ntests_failed(srunner);
	srunner_free(srunner);

	return !!failed;
}
