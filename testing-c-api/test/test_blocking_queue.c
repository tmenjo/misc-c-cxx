#include <limits.h>
#include <pthread.h>
#include <stdlib.h>

#include "blocking_queue.h"

#include <check.h>
#include "checkutil-inl.h"

#define A 13
#define B 11
#define C 19
#define D 17

#define assert_put_abc(queue) do {		\
	int *p = NULL;				\
	p = malloc(sizeof(int));		\
	assert_not_nullptr(p);			\
	*p = A;					\
	ck_assert(bq_offer((queue), p));	\
	p = malloc(sizeof(int));		\
	assert_not_nullptr(p);			\
	*p = B;					\
	ck_assert(bq_offer((queue), p));	\
	p = malloc(sizeof(int));		\
	assert_not_nullptr(p);			\
	*p = C;					\
	ck_assert(bq_offer((queue), p));	\
} while(0)

START_TEST(test_capacity)
{
	/* capacity should be positive */
	assert_nullptr(bq_new(0));
	assert_nullptr(bq_new(-1));
	assert_nullptr(bq_new(INT_MIN));

	struct bq *const queue = bq_new(INT_MAX);
	assert_not_nullptr(queue);
	ck_assert_int_eq(INT_MAX, bq_capacity(queue));
	ck_assert_int_eq(0, bq_size(queue));
	bq_destroy(queue, NULL);
}
END_TEST

START_TEST(test_null_element)
{
	struct bq *const queue = bq_new(1);
	assert_not_nullptr(queue);
	ck_assert_int_eq(1, bq_capacity(queue));
	ck_assert_int_eq(0, bq_size(queue));

	/* NULL cannot be put */
	ck_assert(!bq_put(queue, NULL));
	ck_assert_int_eq(0, bq_size(queue));
	ck_assert(!bq_offer(queue, NULL));
	ck_assert_int_eq(0, bq_size(queue));

	int a = A;
	ck_assert(bq_put(queue, &a));
	ck_assert_int_eq(1, bq_size(queue));

	/* non-blocking return */
	ck_assert(!bq_put(queue, NULL));
	ck_assert_int_eq(1, bq_size(queue));

	bq_destroy(queue, NULL);
}
END_TEST

START_TEST(test_fifo)
{
	struct bq *const queue = bq_new(3);
	assert_not_nullptr(queue);
	ck_assert_int_eq(3, bq_capacity(queue));
	ck_assert_int_eq(0, bq_size(queue));

	int a = A, b = B, c = C, d = D;
	int *p = NULL;

	/* <= head [] tail <= */

	ck_assert(bq_put(queue, &a));
	ck_assert_int_eq(1, bq_size(queue));
	ck_assert_int_eq(3, bq_capacity(queue));
	ck_assert(bq_put(queue, &b));
	ck_assert_int_eq(2, bq_size(queue));
	ck_assert_int_eq(3, bq_capacity(queue));
	ck_assert(bq_put(queue, &c));
	ck_assert_int_eq(3, bq_size(queue));
	ck_assert_int_eq(3, bq_capacity(queue));

	/* <= head [ A, B, C ] tail <= */

	p = bq_take(queue);
	ck_assert_ptr_eq(&a, p);
	ck_assert_int_eq(A, *p);
	ck_assert_int_eq(2, bq_size(queue));

	/* <= head [ B, C ] tail <= */

	p = bq_take(queue);
	ck_assert_ptr_eq(&b, p);
	ck_assert_int_eq(B, *p);
	ck_assert_int_eq(1, bq_size(queue));

	/* <= head [ C ] tail <= */

	ck_assert(bq_put(queue, &d));
	ck_assert_int_eq(2, bq_size(queue));

	/* <= head [ C, D ] tail <= */

	p = bq_take(queue);
	ck_assert_ptr_eq(&c, p);
	ck_assert_int_eq(C, *p);
	ck_assert_int_eq(1, bq_size(queue));

	/* <= head [ D ] tail <= */

	p = bq_take(queue);
	ck_assert_ptr_eq(&d, p);
	ck_assert_int_eq(D, *p);
	ck_assert_int_eq(0, bq_size(queue));

	/* <= head [] tail <= */

	bq_destroy(queue, NULL);
}
END_TEST

static void *run_take(void *arg);
static void *run_put(void *arg);
START_TEST(test_block)
{
	struct bq *queue = bq_new(3);
	assert_not_nullptr(queue);
	ck_assert_int_eq(0, bq_size(queue));

	int *p = NULL;

	void *ret = NULL;
	pthread_t t;

	/* a thread blocked by bq_take() can be cancelled */
	assert_success(pthread_create(&t, NULL, run_take, queue));
	assert_success(pthread_cancel(t));
	assert_success(pthread_join(t, &ret));
	ck_assert_ptr_eq(PTHREAD_CANCELED, ret);
	ck_assert_int_eq(0, bq_size(queue));

	/* bq_take() waits for bq_put() */
	assert_success(pthread_create(&t, NULL, run_take, queue));

	p = calloc(1, sizeof(int));
	assert_not_nullptr(p);
	ck_assert(bq_put(queue, p));

	assert_success(pthread_join(t, &ret));
	ck_assert_ptr_eq(p, ret);
	ck_assert_int_eq(0, bq_size(queue));
	free(p);

	assert_put_abc(queue);
	ck_assert_int_eq(3, bq_size(queue));

	/* a thread blocked by bq_put() can be cancelled */
	assert_success(pthread_create(&t, NULL, run_put, queue));
	assert_success(pthread_cancel(t));
	assert_success(pthread_join(t, &ret));
	ck_assert_ptr_eq(PTHREAD_CANCELED, ret);

	/* bq_put() waits for bq_take() */
	assert_success(pthread_create(&t, NULL, run_put, queue));

	p = bq_take(queue);
	assert_not_nullptr(p);
	ck_assert_int_eq(A, *p);
	free(p);

	assert_success(pthread_join(t, &ret));
	ck_assert_ptr_eq(NULL, ret);
	ck_assert_int_eq(3, bq_size(queue));

	p = bq_take(queue);
	assert_not_nullptr(p);
	ck_assert_int_eq(B, *p);
	free(p);

	p = bq_take(queue);
	assert_not_nullptr(p);
	ck_assert_int_eq(C, *p);
	free(p);

	p = bq_take(queue);
	assert_not_nullptr(p);
	ck_assert_int_eq(42, *p);
	free(p);

	bq_destroy(queue, free);
}
END_TEST

static void *run_take(void *arg)
{
	struct bq *const queue = (struct bq *)arg;
	int *const p = bq_take(queue);
	return p;
}

static void *run_put(void *arg)
{
	struct bq *const queue = (struct bq *)arg;
	int *const p = malloc(sizeof(int));
	*p = 42;
	pthread_cleanup_push(free, p);
	bq_put(queue, p);
	pthread_cleanup_pop(0); /* just remove */
	return NULL;
}

START_TEST(test_nonblock)
{
	struct bq *queue = bq_new(1);
	assert_not_nullptr(queue);

	assert_nullptr(bq_poll(queue));

	int a = A, b = B;
	ck_assert(bq_offer(queue, &a));
	ck_assert(!bq_offer(queue, &b));

	int *const p = bq_poll(queue);
	ck_assert_ptr_eq(&a, p);
	ck_assert_int_eq(A, *p);

	bq_destroy(queue, NULL);
}
END_TEST

START_TEST(test_dtor)
{
	struct bq *const queue = bq_new(3);
	assert_not_nullptr(queue);

	assert_put_abc(queue);
	ck_assert_int_eq(3, bq_size(queue));

	bq_destroy(queue, free);
}
END_TEST

#define CAPACITY 10
#define NR_LOOPS 10000
static void *run_producer(void *);
static void *run_consumer(void *);
START_TEST(test_multithread)
{
	struct bq *const queue = bq_new(CAPACITY);
	assert_not_nullptr(queue);

	void *ret = NULL;
	pthread_t p, c;
	assert_success(pthread_create(&c, NULL, run_consumer, queue));
	assert_success(pthread_create(&p, NULL, run_producer, queue));
	assert_success(pthread_join(p, NULL));
	assert_success(pthread_join(c, &ret));

	assert_not_nullptr(ret);
	ck_assert(*(_Bool *)ret);
	free(ret);

	bq_destroy(queue, free);
}
END_TEST

static void *run_producer(void *arg)
{
	struct bq *const queue = (struct bq *)arg;

	for (int i = 0; i < NR_LOOPS; ++i) {
		int *const elem = malloc(sizeof(int));
		*elem = i;
		bq_put(queue, elem);
	}

	return NULL;
}

static void *run_consumer(void *arg)
{
	struct bq *const queue = (struct bq *)arg;

	_Bool *const ret = malloc(sizeof(_Bool));
	if (!ret)
		return NULL;
	pthread_cleanup_push(free, ret);

	*ret = 1; /* assume success */
	for (int i = 0; i < NR_LOOPS; ++i) {
		int *const elem = bq_take(queue);
		if (*elem != i)
			*ret = 0; /* failure */
		free(elem);
	}

	pthread_cleanup_pop(0); /* just remove */
	return ret;
}

int main()
{
	TCase *const tcase = tcase_create("all");
	tcase_add_test(tcase, test_capacity);
	tcase_add_test(tcase, test_null_element);
	tcase_add_test(tcase, test_fifo);
	tcase_add_test(tcase, test_block);
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
