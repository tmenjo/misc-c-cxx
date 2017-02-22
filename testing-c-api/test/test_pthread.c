#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/eventfd.h>
#include <time.h>
#include <unistd.h>

#include <check.h>
#include "checkutil-inl.h"

struct v_int {
	size_t size; /* in count */
	int *ptr;
};

static pthread_key_t key;

static void dtor(void *arg)
{
	const struct v_int *fds = (const struct v_int *)arg;

	int *const ptr = fds->ptr;
	const size_t size = fds->size;

	for (size_t i = 0; i < size; ++i) {
		const int fd = ptr[i];
		printf("ptr=%p; fd=%d\n", ptr, fd); /* for debug */
		assert_success(close(fd));
	}
	free(ptr);
}

static void *run(void *arg)
{
	struct v_int *const fds =
		(struct v_int *)calloc(1, sizeof(struct v_int));
	assert_not_nullptr(fds);
	assert_success(pthread_setspecific(key, fds));

	const size_t size = *((size_t *)arg);
	int *const ptr = (int *)calloc(size, sizeof(int));
	assert_not_nullptr(ptr);
	for (size_t i = 0; i < size; ++i) {
		const int fd = eventfd(0, 0);
		assert_not_failure(fd);
		ptr[i] = fd;
	}
	fds->size = size;
	fds->ptr = ptr;

	free(arg);
	return NULL;
}

START_TEST(test_dtor)
{
	srandom((unsigned int)time(NULL));

	assert_success(pthread_key_create(&key, dtor));

	pthread_t thread[2];
	for (size_t i = 0; i < 2; ++i) {
		size_t *const arg = (size_t *)calloc(1, sizeof(size_t));
		assert_not_nullptr(arg);
		*arg = (size_t)((random() & 0x7) + 1);
		assert_success(pthread_create(&thread[i], NULL, run, arg));
	}
	for (size_t i = 0; i < 2; ++i) {
		assert_success(pthread_join(thread[i], NULL));
	}
}
END_TEST

int main()
{
	TCase *const tcase = tcase_create("all");
	tcase_add_test(tcase, test_dtor);

	Suite *const suite = suite_create("pthread");
	suite_add_tcase(suite, tcase);

	SRunner *const srunner = srunner_create(suite);
	srunner_run_all(srunner, CK_NORMAL);
	const int failed = srunner_ntests_failed(srunner);
	srunner_free(srunner);

	return !!failed;
}
