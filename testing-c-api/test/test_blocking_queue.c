#include <pthread.h>
#include <stdlib.h>

#include "blocking_queue.h"

#include <check.h>
#include "checkutil-inl.h"

START_TEST(test_fifo)
{
	struct bq *queue = bq_new(3);
	assert_not_nullptr(queue);
	ck_assert_int_eq(3, bq_capacity(queue));
	ck_assert_int_eq(0, bq_size(queue));

	int a = 3, b = 2, c = 5;
	bq_put(queue, (void *)&a);
	ck_assert_int_eq(1, bq_size(queue));
	bq_put(queue, (void *)&b);
	ck_assert_int_eq(2, bq_size(queue));
	bq_put(queue, (void *)&c);
	ck_assert_int_eq(3, bq_size(queue));

	int *const ap = (int *)bq_take(queue);
	ck_assert_int_eq(2, bq_size(queue));
	ck_assert_ptr_eq(&a, ap);
	ck_assert_int_eq(3, *ap);

	int *const bp = (int *)bq_take(queue);
	ck_assert_int_eq(1, bq_size(queue));
	ck_assert_ptr_eq(&b, bp);
	ck_assert_int_eq(2, *bp);

	int d = 1;
	bq_put(queue, (void *)&d);
	ck_assert_int_eq(2, bq_size(queue));

	int *const cp = (int *)bq_take(queue);
	ck_assert_int_eq(1, bq_size(queue));
	ck_assert_ptr_eq(&c, cp);
	ck_assert_int_eq(5, *cp);

	int *const dp = (int *)bq_take(queue);
	ck_assert_int_eq(0, bq_size(queue));
	ck_assert_ptr_eq(&d, dp);
	ck_assert_int_eq(1, *dp);

	bq_destroy(queue, NULL);
}
END_TEST

START_TEST(test_nonblock)
{
	struct bq *queue = bq_new(1);
	assert_not_nullptr(queue);

	assert_nullptr(bq_poll(queue));

	int a = 3;
	ck_assert(bq_offer(queue, &a));

	int b = 2;
	ck_assert(!bq_offer(queue, &b));

	int *const ap = bq_poll(queue);
	ck_assert_ptr_eq(&a, ap);
}
END_TEST

START_TEST(test_dtor)
{
	struct bq *queue = bq_new(3);
	assert_not_nullptr(queue);

	int *const ap = calloc(1, sizeof(int));
	int *const bp = calloc(1, sizeof(int));
	int *const cp = calloc(1, sizeof(int));

	bq_put(queue, ap);
	bq_put(queue, bp);
	bq_put(queue, cp);
	ck_assert_int_eq(3, bq_size(queue));

	bq_destroy(queue, free);
}
END_TEST

static void *run_producer(void *);
static void *run_consumer(void *);
START_TEST(test_multithread)
{
	struct bq *const queue = bq_new(10);
	assert_not_nullptr(queue);

	void *ret = NULL;
	pthread_t p, c;
	assert_success(pthread_create(&c, NULL, run_consumer, (void *)queue));
	assert_success(pthread_create(&p, NULL, run_producer, (void *)queue));
	assert_success(pthread_join(p, NULL));
	assert_success(pthread_join(c, &ret));

	assert_not_nullptr(ret);
	assert_success(*((int *)ret));
	free(ret);

	bq_destroy(queue, free);
}
END_TEST

static void *run_producer(void *arg)
{
	struct bq *const queue = (struct bq *)arg;

	for (int i = 0; i < 10000; ++i) {
		int *const elem = malloc(sizeof(int));
		*elem = i;
		bq_put(queue, elem);
	}

	return NULL;
}

static void *run_consumer(void *arg)
{
	struct bq *const queue = (struct bq *)arg;

	int *const ret = malloc(sizeof(int));
	if (!ret)
		return NULL;
	pthread_cleanup_push(free, ret);

	*ret = 0;
	for (int i = 0; i < 10000; ++i) {
		int *const elem = bq_take(queue);
		if (i != *elem)
			*ret = 1; /* failure */
	}

	pthread_cleanup_pop(0); /* just remove */
	return (void *)ret;
}

int main()
{
	TCase *const tcase = tcase_create("all");
	tcase_add_test(tcase, test_fifo);
	tcase_add_test(tcase, test_nonblock);
	tcase_add_test(tcase, test_dtor);
	tcase_add_test(tcase, test_multithread);

	Suite *const suite = suite_create("blocking_queue");
	suite_add_tcase(suite, tcase);

	SRunner *const srunner = srunner_create(suite);
	srunner_run_all(srunner, CK_NORMAL);
	const int failed = srunner_ntests_failed(srunner);
	srunner_free(srunner);

	return !!failed;
}
