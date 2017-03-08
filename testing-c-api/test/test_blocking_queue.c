#include <limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <unistd.h>

#include "blocking_queue.h"

#include <check.h>
#include "checkutil-inl.h"
#include "checkutil-pthread-inl.h"

#define bq_put_or_free(q_, p_) do {		\
	pthread_cleanup_push_free((p_));	\
	bq_put((q_), (p_));			\
	pthread_cleanup_pop_noexec();		\
} while(0)

#define assert_put(q_, elem_) do {		\
	int *const p_ = malloc(sizeof(int));	\
	assert_not_nullptr(p_);			\
	*p_ = (elem_);				\
	ck_assert(bq_put((q_), p_));		\
} while(0)

#define assert_put_unsafe(q_, elem_, size_) do {	\
	assert_put((q_), (elem_));			\
	ck_assert_int_eq((size_), bq_size((q_)));	\
} while(0)

#define assert_take(q_, elem_) do {		\
	int *const p_ = bq_take((q_));		\
	assert_not_nullptr(p_);			\
	ck_assert_int_eq((elem_), *p_);		\
	free(p_);				\
} while(0)

#define assert_take_unsafe(q_, elem_, size_) do {	\
	assert_take((q_), (elem_));			\
	ck_assert_int_eq((size_), bq_size((q_)));	\
} while(0)

#define assert_offer_unsafe(q_, elem_, size_) do {	\
	int *const p_ = malloc(sizeof(int));		\
	assert_not_nullptr(p_);				\
	*p_ = (elem_);					\
	ck_assert(bq_offer((q_), p_));			\
	ck_assert_int_eq((size_), bq_size((q_)));	\
} while(0)

#define assert_poll_unsafe(q_, elem_, size_) do {	\
	int *const p_ = bq_poll((q_));			\
	assert_not_nullptr(p_);				\
	ck_assert_int_eq((elem_), *p_);			\
	ck_assert_int_eq((size_), bq_size((q_)));	\
	free(p_);					\
} while(0)

#define assert_offer_failed(q_, elem_) do {	\
	int *const p_ = malloc(sizeof(int));	\
	assert_not_nullptr(p_);			\
	*p_ = (elem_);				\
	ck_assert(!bq_offer((q_), p_));		\
	free(p_);				\
} while(0)

#define assert_poll_failed(q_) \
	assert_nullptr(bq_poll((q_)))

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

	int a = 'A', b = 'B', c = 'C', d = 'D';
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
	ck_assert_int_eq('A', *p);
	ck_assert_int_eq(2, bq_size(queue));
	ck_assert_int_eq(3, bq_capacity(queue));

	/* <= head [ B, C ] tail <= */

	p = bq_take(queue);
	ck_assert_ptr_eq(&b, p);
	ck_assert_int_eq('B', *p);
	ck_assert_int_eq(1, bq_size(queue));
	ck_assert_int_eq(3, bq_capacity(queue));

	/* <= head [ C ] tail <= */

	ck_assert(bq_put(queue, &d));
	ck_assert_int_eq(2, bq_size(queue));
	ck_assert_int_eq(3, bq_capacity(queue));

	/* <= head [ C, D ] tail <= */

	p = bq_take(queue);
	ck_assert_ptr_eq(&c, p);
	ck_assert_int_eq('C', *p);
	ck_assert_int_eq(1, bq_size(queue));
	ck_assert_int_eq(3, bq_capacity(queue));

	/* <= head [ D ] tail <= */

	p = bq_take(queue);
	ck_assert_ptr_eq(&d, p);
	ck_assert_int_eq('D', *p);
	ck_assert_int_eq(0, bq_size(queue));
	ck_assert_int_eq(3, bq_capacity(queue));

	/* <= head [] tail <= */

	bq_destroy(queue, NULL);
}
END_TEST

static void *run_producer1(void *);
static void *run_consumer1(void *);
START_TEST(test_multithread1)
{
	struct bq *const queue = bq_new(CAPACITY);
	assert_not_nullptr(queue);

	pthread_t p, c;

	/* run consumer first */
	assert_pthread_create(&c, run_consumer1, queue);
	assert_pthread_create(&p, run_producer1, queue);

	/* join producer first */
	assert_pthread_join(C_OK, p);
	assert_pthread_join(C_OK, c);

	ck_assert_int_eq(0, bq_size(queue));
	bq_destroy(queue, free);
}
END_TEST

static void *run_producer1(void *arg)
{
	struct bq *const queue = arg;

	int *const ret = malloc(sizeof(int));
	if (!ret) goto out0;
	pthread_cleanup_push_free(ret);

	*ret = C_ERR;

	for (int i = 0; i < NR_LOOPS; ++i) {
		int *const p = malloc(sizeof(int));
		if (!p) goto out1;

		*p = i;
		bq_put_or_free(queue, p);
	}

	*ret = C_OK;
out1:
	pthread_cleanup_pop_noexec();
out0:
	return ret;
}

static void *run_consumer1(void *arg)
{
	struct bq *const queue = arg;

	int *const ret = malloc(sizeof(int));
	if (!ret) goto out0;
	pthread_cleanup_push_free(ret);

	*ret = C_ERR;

	for (int i = 0; i < NR_LOOPS; ++i) {
		int *const p = bq_take(queue);
		if (*p != i) goto out1;

		free(p);
	}

	*ret = C_OK;
out1:
	pthread_cleanup_pop_noexec();
out0:
	return ret;
}

static void *run_producer2(void *);
static void *run_consumer2(void *);
START_TEST(test_multithread2)
{
	struct bq *const queue = bq_new(CAPACITY);
	assert_not_nullptr(queue);

	pthread_t p[NR_PRODUCERS], c[NR_CONSUMERS];

	/* run consumer first */
	for (size_t i = 0; i < NR_CONSUMERS; ++i)
		assert_pthread_create(&c[i], run_consumer2, queue);
	for (size_t i = 0; i < NR_PRODUCERS; ++i)
		assert_pthread_create(&p[i], run_producer2, queue);

	sleep(1);

	/* cancel producer first */
	for (size_t i = 0; i < NR_PRODUCERS; ++i)
		assert_pthread_cancel(p[i]);
	for (size_t i = 0; i < NR_CONSUMERS; ++i)
		assert_pthread_cancel(c[i]);

	bq_destroy(queue, free);
}
END_TEST

static void *run_producer2(void *arg)
{
	struct bq *const queue = arg;

	for (;;) {
		pthread_testcancel();

		int *const p = malloc(sizeof(int));
		if (!p) continue;
		bq_put_or_free(queue, p);
	}

	return arg; /* should never be here */
}

static void *run_consumer2(void *arg)
{
	struct bq *const queue = arg;

	for (;;) {
		pthread_testcancel();

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

START_TEST(test_dtor)
{
	assert_put_unsafe(queue_, 'A', 1);
	assert_put_unsafe(queue_, 'B', 2);
	assert_put_unsafe(queue_, 'C', 3);
}
END_TEST

START_TEST(test_null_element)
{
	/* NULL cannot be put */
	ck_assert(!bq_put(queue_, NULL));
	ck_assert_int_eq(0, bq_size(queue_));
	ck_assert(!bq_offer(queue_, NULL));
	ck_assert_int_eq(0, bq_size(queue_));

	assert_put_unsafe(queue_, 'A', 1);
	assert_put_unsafe(queue_, 'B', 2);
	assert_put_unsafe(queue_, 'C', 3);

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
	pthread_t t;

	/* a thread blocked by bq_take() can be canceled */
	assert_pthread_create(&t, run_take, queue_);
	assert_pthread_cancel(t);
	ck_assert_int_eq(0, bq_size(queue_));

	/* bq_take() is waiting for bq_put() */
	assert_pthread_create(&t, run_take, queue_);
	assert_put(queue_, 'A');
	assert_pthread_join('A', t);
	ck_assert_int_eq(0, bq_size(queue_));

	assert_put_unsafe(queue_, 'B', 1);
	assert_put_unsafe(queue_, 'C', 2);
	assert_put_unsafe(queue_, 'D', 3);

	/* a thread blocked by bq_put() can be canceled */
	assert_pthread_create(&t, run_put, queue_);
	assert_pthread_cancel(t);
	ck_assert_int_eq(3, bq_size(queue_));

	/* bq_put() is waiting for bq_take() */
	assert_pthread_create(&t, run_put, queue_);
	assert_take(queue_, 'B');
	assert_pthread_join(C_OK, t);
	ck_assert_int_eq(3, bq_size(queue_));

	assert_take_unsafe(queue_, 'C', 2);
	assert_take_unsafe(queue_, 'D', 1);
	assert_take_unsafe(queue_, 'E', 0);
}
END_TEST

static void *run_take(void *arg)
{
	struct bq *const queue = arg;
	int *const p = bq_take(queue); /* cancellation point */
	return p;
}

static void *run_put(void *arg)
{
	struct bq *const queue = arg;

	int *const ret = malloc(sizeof(int));
	if (!ret) goto out0;
	pthread_cleanup_push_free(ret);

	*ret = C_ERR;

	int *const p = malloc(sizeof(int));
	if (!p) goto out1;

	*p = 'E';
	bq_put_or_free(queue, p); /* cancellation point */

	*ret = C_OK;
out1:
	pthread_cleanup_pop_noexec();
out0:
	return ret;
}

START_TEST(test_nonblock)
{
	assert_poll_failed(queue_);
	ck_assert_int_eq(0, bq_size(queue_));

	assert_offer_unsafe(queue_, 'A', 1);
	assert_offer_unsafe(queue_, 'B', 2);
	assert_offer_unsafe(queue_, 'C', 3);

	assert_offer_failed(queue_, 'D');
	ck_assert_int_eq(3, bq_size(queue_));

	assert_poll_unsafe(queue_, 'A', 2);
	assert_poll_unsafe(queue_, 'B', 1);
	assert_poll_unsafe(queue_, 'C', 0);
}
END_TEST

int main()
{
	TCase *const tcase1 = tcase_create("testcase1");
	tcase_add_test(tcase1, test_capacity);
	tcase_add_test(tcase1, test_fifo);
	tcase_add_test(tcase1, test_multithread1);
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
