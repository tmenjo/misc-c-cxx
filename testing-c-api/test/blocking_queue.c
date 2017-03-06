#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/queue.h>

#include "blocking_queue.h"

#define unlikely(x) __builtin_expect(!!(x), 0)

#define pthread_cleanup_push_mutex_unlock(m) \
	pthread_cleanup_push((void *)pthread_mutex_unlock, (m))

/* prototype */
struct bq_elem;
static void do_nothing(void *);
static _Bool bq_empty_unsafe(const struct bq *);
static _Bool bq_full_unsafe(const struct bq *);
static void bq_insert_unsafe(struct bq *, struct bq_elem *);
static struct bq_elem *bq_remove_unsafe(struct bq *);

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

struct bq *bq_new(int capacity)
{
	if (unlikely(capacity <= 0)) {
		errno = EINVAL;
		return NULL;
	}

	struct bq *const queue = malloc(sizeof(struct bq));
	if (unlikely(!queue))
		return NULL;

	queue->size_ = 0;
	queue->capacity_ = capacity;
	STAILQ_INIT(&queue->head_);
	pthread_mutex_init(&queue->mutex_, NULL);
	pthread_cond_init(&queue->cond_can_put_, NULL);
	pthread_cond_init(&queue->cond_can_take_, NULL);

	return queue;
}

int bq_capacity(const struct bq *queue)
{
	return queue->capacity_;
}

int bq_size(struct bq *queue)
{
	pthread_mutex_lock(&queue->mutex_);
	const int ret = queue->size_; /* TODO lock-free read */
	pthread_mutex_unlock(&queue->mutex_);

	return ret;
}

_Bool bq_put(struct bq *queue, void *raw)
{
	struct bq_elem *const wrap = malloc(sizeof(struct bq_elem));
	wrap->elem_ = raw;

	pthread_mutex_t *const mutex = &queue->mutex_;
	pthread_cleanup_push_mutex_unlock(mutex);
	pthread_mutex_lock(mutex);
	/* critical section >>> */
	while (bq_full_unsafe(queue))
		pthread_cond_wait(&queue->cond_can_put_, mutex);

	bq_insert_unsafe(queue, wrap);

	pthread_cond_broadcast(&queue->cond_can_take_);
	/* <<< critical section */
	pthread_cleanup_pop(1); /* execute before remove */

	return 1;
}

_Bool bq_offer(struct bq *queue, void *raw)
{
	_Bool ret = 0; /* assume failure */
	struct bq_elem *const wrap = malloc(sizeof(struct bq_elem));
	wrap->elem_ = raw;

	if (pthread_mutex_trylock(&queue->mutex_) != 0)
		goto out;

	/* critical section >>> */

	if (!bq_full_unsafe(queue)) {
		bq_insert_unsafe(queue, wrap);
		ret = 1; /* success */
		pthread_cond_broadcast(&queue->cond_can_take_);
	}

	/* <<< critical section */
	pthread_mutex_unlock(&queue->mutex_);
out:
	if (!ret)
		free(wrap);
	return ret;
}

void *bq_take(struct bq *queue)
{
	/*
	 * "wrap" should be defined here because
	 * pthread_cleanup_{push,pop} make a code block
	 */
	struct bq_elem *wrap = NULL;

	pthread_mutex_t *const mutex = &queue->mutex_;
	pthread_cleanup_push_mutex_unlock(mutex);
	pthread_mutex_lock(mutex);
	/* critical section >>> */
	while (bq_empty_unsafe(queue))
		pthread_cond_wait(&queue->cond_can_take_, mutex);

	wrap = bq_remove_unsafe(queue);

	pthread_cond_broadcast(&queue->cond_can_put_);
	/* <<< critical section */
	pthread_cleanup_pop(1); /* execute before remove */

	void *const raw = wrap->elem_;
	free(wrap);

	return raw;
}

void *bq_poll(struct bq *queue)
{
	void *raw = NULL; /* assume failure */
	struct bq_elem *wrap = NULL; /* assume failure */

	if (pthread_mutex_trylock(&queue->mutex_) != 0)
		goto out;
	/* critical section >>> */

	if (!bq_empty_unsafe(queue)) {
		wrap = bq_remove_unsafe(queue); /* success */
		pthread_cond_broadcast(&queue->cond_can_put_);
	}

	/* <<< critical section */
	pthread_mutex_unlock(&queue->mutex_);

	if (wrap) {
		raw = wrap->elem_;
		free(wrap);
	}
out:
	return raw;
}

void bq_destroy(struct bq *queue, void (*dtor)(void *))
{
	dtor = dtor ? dtor : do_nothing;

	struct bq_elem *i = STAILQ_FIRST(&queue->head_);
	while (i) {
		struct bq_elem *n = STAILQ_NEXT(i, next_);
		dtor(i->elem_);
		free(i);
		i = n;
	}

	pthread_mutex_destroy(&queue->mutex_);
	pthread_cond_destroy(&queue->cond_can_put_);
	pthread_cond_destroy(&queue->cond_can_take_);

	free(queue);
}

static void do_nothing(void *dummy)
{
	(void)dummy;
}

static inline _Bool bq_empty_unsafe(const struct bq *queue)
{
	return (queue->size_ <= 0);
}

static inline _Bool bq_full_unsafe(const struct bq *queue)
{
	return (queue->size_ >= queue->capacity_);
}

static inline void bq_insert_unsafe(struct bq *queue, struct bq_elem *wrap)
{
	STAILQ_INSERT_TAIL(&queue->head_, wrap, next_);
	++queue->size_;
}

static inline struct bq_elem *bq_remove_unsafe(struct bq *queue)
{
	struct bq_elem *const wrap = STAILQ_FIRST(&queue->head_);
	STAILQ_REMOVE_HEAD(&queue->head_, next_);
	--queue->size_;

	return wrap;
}
