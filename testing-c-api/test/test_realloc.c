#include <errno.h>
#include <stdlib.h>

#include <check.h>
#include "checkutil-inl.h"

static const size_t SIZE_4K = (size_t)4*1024;
static const size_t SIZE_2M = (size_t)2*1024*1024;

static void assert_realloc(int err, int ret, void *ptr, size_t size)
{
	errno = EOK;
	void *const r = realloc(ptr, size);
	const int e = errno;
	ck_assert_int_eq(ret, !!r);
	ck_assert_int_eq(err, e);
	free(e == EOK ? r : ptr);
}

static void assert_realloc_success(void *ptr, size_t size)
{
	/* returns non-NULL */
	assert_realloc(EOK, !NULL, ptr, size);
}

static void assert_realloc_free(void *ptr)
{
	/* returns NULL but errno is not set */
	assert_realloc(EOK, !!NULL, ptr, 0);
}

static void assert_realloc_enomem(void *ptr)
{
	/* returns NULL and errno is ENOMEM */
	assert_realloc(ENOMEM, !!NULL, ptr, SIZE_MAX);
}

START_TEST(test_realloc_success)
{
	assert_realloc_success(NULL, 0);
	assert_realloc_success(NULL, 1);

	assert_realloc_success(malloc(SIZE_4K), SIZE_2M);
	assert_realloc_success(malloc(SIZE_2M), SIZE_4K);

	assert_realloc_success(calloc(SIZE_4K, 1), SIZE_2M);
	assert_realloc_success(calloc(SIZE_2M, 1), SIZE_4K);

	assert_realloc_success(realloc(NULL, SIZE_4K), SIZE_2M);
	assert_realloc_success(realloc(NULL, SIZE_2M), SIZE_4K);
}
END_TEST

START_TEST(test_realloc_free)
{
	assert_realloc_free(malloc(1));
	assert_realloc_free(calloc(1, 1));
	assert_realloc_free(realloc(NULL, 1));
}
END_TEST

START_TEST(test_realloc_enomem)
{
	assert_realloc_enomem(NULL);
	assert_realloc_enomem(malloc(1));
	assert_realloc_enomem(calloc(1, 1));
	assert_realloc_enomem(realloc(NULL, 1));
}
END_TEST

int main()
{
	TCase *const tcase = tcase_create("realloc");
	tcase_add_test(tcase, test_realloc_success);
	tcase_add_test(tcase, test_realloc_free);
	tcase_add_test(tcase, test_realloc_enomem);

	Suite *const suite = suite_create("realloc");
	suite_add_tcase(suite, tcase);

	SRunner *const srunner = srunner_create(suite);
	srunner_run_all(srunner, CK_NORMAL);
	const int failed = srunner_ntests_failed(srunner);
	srunner_free(srunner);

	return !!failed;
}
