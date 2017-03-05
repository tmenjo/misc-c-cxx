#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/queue.h>

#include "blocking_queue.h"

struct bq_elem {
	void *elem_;
	STAILQ_ENTRY(bq_elem) next_;
};
STAILQ_HEAD(bq_head, bq_elem);

struct bq {
	int size_;
	int capacity_;
	struct bq_head head_;
	pthread_mutex_t mutex_;
	pthread_cond_t cond_can_put_;
	pthread_cond_t cond_can_take_;
};

struct bq *bq_new(int cap)
{
	if (cap <= 0) {
		errno = EINVAL;
		return NULL;
	}

	struct bq *const queue = malloc(sizeof(struct bq));
	if (!queue)
		return NULL;

	queue->size_ = 0;
	queue->capacity_ = cap;
	STAILQ_INIT(&(queue->head_));
	pthread_mutex_init(&(queue->mutex_), NULL);
	pthread_cond_init(&(queue->cond_can_put_), NULL);
	pthread_cond_init(&(queue->cond_can_take_), NULL);

	return queue;
}

int bq_capacity(const struct bq *queue)
{
	return queue->capacity_;
}

int bq_size(struct bq *queue)
{
	/* TODO lock-free read */
	pthread_mutex_lock(&(queue->mutex_));
	const int ret = queue->size_;
	pthread_mutex_unlock(&(queue->mutex_));

	return ret;
}

void bq_put(struct bq *queue, void *raw)
{
	struct bq_elem *const wrap = malloc(sizeof(struct bq_elem));
	wrap->elem_ = raw;

	pthread_mutex_t *const mutex = &(queue->mutex_);
	pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)mutex);
	pthread_mutex_lock(mutex);
	/* critical section >>> */
	while (queue->size_ >= queue->capacity_)
		pthread_cond_wait(&(queue->cond_can_put_), mutex);

	STAILQ_INSERT_TAIL(&(queue->head_), wrap, next_);
	++(queue->size_);

	pthread_cond_broadcast(&(queue->cond_can_take_));
	/* <<< critical section */
	pthread_cleanup_pop(1); /* execute before remove */
}

void *bq_take(struct bq *queue)
{
	/*
	 * "wrap" should be defined here because
	 * pthread_cleanup_{push,pop} make a code block
	 */
	struct bq_elem *wrap = NULL;

	pthread_mutex_t *const mutex = &(queue->mutex_);
	pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)mutex);
	pthread_mutex_lock(mutex);
	/* critical section >>> */
	while (queue->size_ <= 0)
		pthread_cond_wait(&(queue->cond_can_take_), mutex);

	struct bq_head *const head = &(queue->head_);
	wrap = STAILQ_FIRST(head);
	STAILQ_REMOVE_HEAD(head, next_);
	--(queue->size_);

	pthread_cond_broadcast(&(queue->cond_can_put_));
	/* <<< critical section */
	pthread_cleanup_pop(1); /* execute before remove */

	void *const raw = wrap->elem_;
	free(wrap);

	return raw;
}

void bq_destroy(struct bq *queue, void (*dtor)(void *))
{
	struct bq_elem *i = STAILQ_FIRST(&(queue->head_));
	while (i) {
		struct bq_elem *n = STAILQ_NEXT(i, next_);
		if (dtor)
			dtor(i->elem_);
		free(i);
		i = n;
	}

	pthread_mutex_destroy(&(queue->mutex_));
	pthread_cond_destroy(&(queue->cond_can_put_));
	pthread_cond_destroy(&(queue->cond_can_take_));

	free(queue);
}
