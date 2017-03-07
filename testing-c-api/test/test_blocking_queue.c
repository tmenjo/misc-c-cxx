#include <limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <unistd.h>

#include "blocking_queue.h"

#include <check.h>
#include "checkutil-inl.h"

#define A 13
#define B 11
#define C 19
#define D 17

/* testcase 1 */

START_TEST(test_capacity)
{
	/* capacity should be greater than 0 */
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
	ck_assert_int_eq(3, bq_capacity(queue));

	/* <= head [ B, C ] tail <= */

	p = bq_take(queue);
	ck_assert_ptr_eq(&b, p);
	ck_assert_int_eq(B, *p);
	ck_assert_int_eq(1, bq_size(queue));
	ck_assert_int_eq(3, bq_capacity(queue));

	/* <= head [ C ] tail <= */

	ck_assert(bq_put(queue, &d));
	ck_assert_int_eq(2, bq_size(queue));
	ck_assert_int_eq(3, bq_capacity(queue));

	/* <= head [ C, D ] tail <= */

	p = bq_take(queue);
	ck_assert_ptr_eq(&c, p);
	ck_assert_int_eq(C, *p);
	ck_assert_int_eq(1, bq_size(queue));
	ck_assert_int_eq(3, bq_capacity(queue));

	/* <= head [ D ] tail <= */

	p = bq_take(queue);
	ck_assert_ptr_eq(&d, p);
	ck_assert_int_eq(D, *p);
	ck_assert_int_eq(0, bq_size(queue));
	ck_assert_int_eq(3, bq_capacity(queue));

	/* <= head [] tail <= */

	bq_destroy(queue, NULL);
}
END_TEST

#define CAPACITY 10
#define NR_LOOPS 1000000
static void *run_producer(void *);
static void *run_consumer(void *);
START_TEST(test_multithread)
{
	struct bq *const queue = bq_new(CAPACITY);
	assert_not_nullptr(queue);

	void *ret = NULL;
	pthread_t p, c;

	/* run consumer first */
	assert_success(pthread_create(&c, NULL, run_consumer, queue));
	assert_success(pthread_create(&p, NULL, run_producer, queue));

	/* join producer first */
	assert_success(pthread_join(p, NULL));
	assert_success(pthread_join(c, &ret));

	assert_not_nullptr(ret);
	ck_assert(*(_Bool *)ret);
	free(ret);

	ck_assert_int_eq(0, bq_size(queue));
	bq_destroy(queue, free);
}
END_TEST

static void *run_producer(void *arg)
{
	struct bq *const queue = arg;

	for (int i = 0; i < NR_LOOPS; ++i) {
		int *const elem = malloc(sizeof(int));
		*elem = i;
		bq_put(queue, elem);
	}

	return NULL;
}

static void *run_consumer(void *arg)
{
	struct bq *const queue = arg;

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

#define NR_PRODUCERS 3
#define NR_CONSUMERS 5
static void *run_prod2(void *);
static void *run_cons2(void *);
START_TEST(test_multithread2)
{
	struct bq *const queue = bq_new(CAPACITY);
	assert_not_nullptr(queue);

	pthread_t p[NR_PRODUCERS], c[NR_CONSUMERS];

	/* run consumer first */
	for (size_t i = 0; i < NR_CONSUMERS; ++i)
		assert_success(pthread_create(&c[i], NULL, run_cons2, queue));
	for (size_t i = 0; i < NR_PRODUCERS; ++i)
		assert_success(pthread_create(&p[i], NULL, run_prod2, queue));

	sleep(1);

	/* cancel producer first */
	for (size_t i = 0; i < NR_PRODUCERS; ++i) {
		void *ret = NULL;
		assert_success(pthread_cancel(p[i]));
		assert_success(pthread_join(p[i], &ret));
		ck_assert_ptr_eq(PTHREAD_CANCELED, ret);
	}
	for (size_t i = 0; i < NR_CONSUMERS; ++i) {
		void *ret = NULL;
		assert_success(pthread_cancel(c[i]));
		assert_success(pthread_join(c[i], &ret));
		ck_assert_ptr_eq(PTHREAD_CANCELED, ret);
	}

	ck_assert_int_eq(0, bq_size(queue));
	bq_destroy(queue, free);
}
END_TEST

static void *run_prod2(void *arg)
{
	struct bq *const queue = arg;

	for (;;) {
		pthread_testcancel();

		int *const p = malloc(sizeof(int));
		pthread_cleanup_push(free, p);
		bq_put(queue, p);
		pthread_cleanup_pop(0); /* just remove */
	}

	return arg; /* should never be here */
}

static void *run_cons2(void *arg)
{
	struct bq *const queue = arg;

	for (;;) {
		int *const p = bq_take(queue);
		free(p);
	}

	return arg; /* should never be here */
}

/* testcase 2 */

static struct bq *queue_ = NULL;

static void setup(void)
{
	queue_ = bq_new(3);
	assert_not_nullptr(queue_);
	ck_assert_int_eq(3, bq_capacity(queue_));
	ck_assert_int_eq(0, bq_size(queue_));
}

static void teardown(void)
{
	bq_destroy(queue_, free);
}

#define assert_put_unsafe(queue, elem, size) do {	\
	int *const p = malloc(sizeof(int));		\
	assert_not_nullptr(p);				\
	*p = (elem);					\
	ck_assert(bq_put((queue), p));			\
	ck_assert_int_eq((size), bq_size((queue)));	\
} while(0)

#define assert_take_unsafe(queue, elem, size) do {	\
	int *const p = bq_take((queue));		\
	assert_not_nullptr(p);				\
	ck_assert_int_eq((elem), *p);			\
	ck_assert_int_eq((size), bq_size((queue)));	\
	free(p);					\
} while(0)

START_TEST(test_dtor)
{
	assert_put_unsafe(queue_, A, 1);
	assert_put_unsafe(queue_, B, 2);
	assert_put_unsafe(queue_, C, 3);
}
END_TEST

START_TEST(test_null_element)
{
	/* NULL cannot be put */
	ck_assert(!bq_put(queue_, NULL));
	ck_assert_int_eq(0, bq_size(queue_));
	ck_assert(!bq_offer(queue_, NULL));
	ck_assert_int_eq(0, bq_size(queue_));

	assert_put_unsafe(queue_, A, 1);
	assert_put_unsafe(queue_, B, 2);
	assert_put_unsafe(queue_, C, 3);

	/* non-blocking return */
	ck_assert(!bq_put(queue_, NULL));
	ck_assert_int_eq(3, bq_size(queue_));
	ck_assert(!bq_offer(queue_, NULL));
	ck_assert_int_eq(3, bq_size(queue_));
}
END_TEST

static void *run_take(void *arg);
static void *run_put(void *arg);
START_TEST(test_block)
{
	int *p = NULL;

	void *ret = NULL;
	pthread_t t;

	/* a thread blocked by bq_take() can be canceled */
	assert_success(pthread_create(&t, NULL, run_take, queue_));
	assert_success(pthread_cancel(t));
	assert_success(pthread_join(t, &ret));
	ck_assert_ptr_eq(PTHREAD_CANCELED, ret);
	ck_assert_int_eq(0, bq_size(queue_));

	/* bq_take() is waiting for bq_put() */
	assert_success(pthread_create(&t, NULL, run_take, queue_));

	p = calloc(1, sizeof(int));
	assert_not_nullptr(p);
	ck_assert(bq_put(queue_, p));
	assert_success(pthread_join(t, &ret));
	ck_assert_ptr_eq(p, ret);
	free(p);
	ck_assert_int_eq(0, bq_size(queue_));

	assert_put_unsafe(queue_, A, 1);
	assert_put_unsafe(queue_, B, 2);
	assert_put_unsafe(queue_, C, 3);

	/* a thread blocked by bq_put() can be canceled */
	assert_success(pthread_create(&t, NULL, run_put, queue_));
	assert_success(pthread_cancel(t));
	assert_success(pthread_join(t, &ret));
	ck_assert_ptr_eq(PTHREAD_CANCELED, ret);
	ck_assert_int_eq(3, bq_size(queue_));

	/* bq_put() is waiting for bq_take() */
	assert_success(pthread_create(&t, NULL, run_put, queue_));

	p = bq_take(queue_);
	assert_not_nullptr(p);
	ck_assert_int_eq(A, *p);
	free(p);
	assert_success(pthread_join(t, &ret));
	assert_not_nullptr(ret);
	ck_assert_int_eq(D, *(int *)ret);
	ck_assert_int_eq(3, bq_size(queue_));

	assert_take_unsafe(queue_, B, 2);
	assert_take_unsafe(queue_, C, 1);
	assert_take_unsafe(queue_, D, 0);
}
END_TEST

static void *run_take(void *arg)
{
	struct bq *const queue = arg;
	int *const p = bq_take(queue);
	return p;
}

static void *run_put(void *arg)
{
	struct bq *const queue = arg;

	int *const p = malloc(sizeof(int));
	if (!p)
		return NULL;
	pthread_cleanup_push(free, p);

	*p = D;
	bq_put(queue, p);

	pthread_cleanup_pop(0); /* just remove */

	return p;
}

#define assert_offer_unsafe(queue, elem, size) do {	\
	int *const p = malloc(sizeof(int));		\
	assert_not_nullptr(p);				\
	*p = (elem);					\
	ck_assert(bq_offer((queue), p));		\
	ck_assert_int_eq((size), bq_size((queue)));	\
} while(0)

#define assert_poll_unsafe(queue, elem, size) do {	\
	int *const p = bq_poll((queue));		\
	assert_not_nullptr(p);				\
	ck_assert_int_eq((elem), *p);			\
	ck_assert_int_eq((size), bq_size((queue)));	\
	free(p);					\
} while(0)

START_TEST(test_nonblock)
{
	assert_nullptr(bq_poll(queue_));
	ck_assert_int_eq(0, bq_size(queue_));

	assert_offer_unsafe(queue_, A, 1);
	assert_offer_unsafe(queue_, B, 2);
	assert_offer_unsafe(queue_, C, 3);

	int *const p = calloc(1, sizeof(int));
	assert_not_nullptr(p);
	ck_assert(!bq_offer(queue_, p));
	ck_assert_int_eq(3, bq_size(queue_));
	free(p);

	assert_poll_unsafe(queue_, A, 2);
	assert_poll_unsafe(queue_, B, 1);
	assert_poll_unsafe(queue_, C, 0);
}
END_TEST

int main()
{
	TCase *const tcase1 = tcase_create("testcase1");
	tcase_add_test(tcase1, test_capacity);
	tcase_add_test(tcase1, test_fifo);
	tcase_add_test(tcase1, test_multithread);
	tcase_add_test(tcase1, test_multithread2);

	TCase *const tcase2 = tcase_create("testcase2");
	tcase_add_checked_fixture(tcase2, setup, teardown);
	tcase_add_test(tcase2, test_dtor);
	tcase_add_test(tcase2, test_null_element);
	tcase_add_test(tcase2, test_block);
	tcase_add_test(tcase2, test_nonblock);

	Suite *const suite = suite_create("blocking_queue");
	suite_add_tcase(suite, tcase1);
	suite_add_tcase(suite, tcase2);

	SRunner *const srunner = srunner_create(suite);
	srunner_run_all(srunner, CK_NORMAL);
	const int failed = srunner_ntests_failed(srunner);
	srunner_free(srunner);

	return !!failed;
}
