#ifndef BLOCKING_QUEUE_H
#define BLOCKING_QUEUE_H

/**
 * A blocking queue (opaque type).
 */
struct bq;

/**
 * Create a new blocking queue.
 *
 * @param capacity should be greater than 0.
 * @return a pointer to a new blocking queue if success; NULL otherwise.
 */
struct bq *bq_new(int capacity);

/**
 * Get the capacity of a blocking queue.
 */
int bq_capacity(const struct bq *queue);

/**
 * Get the number of elements in a blocking queue.
 *
 * @param queue is conceptually const.
 */
int bq_size(struct bq *queue);

/**
 * Insert a new element into tail of a blocking queue.
 *
 * If a blocking queue is full (that is, its size is equal to its capacity),
 * bq_put() blocks a calling thread until bq_take() removes an element.
 *
 * A thread can be cancelled while it is blocked by bq_put().
 *
 * @return 1 if "element" is not NULL; 0 otherwise.
 */
_Bool bq_put(struct bq *queue, void *element);

/**
 * Non-blocking version of bq_put().
 *
 * @return 1 if "element" is not NULL and success; 0 otherwise.
 */
_Bool bq_offer(struct bq *queue, void *element);

/**
 * Get and remove an element from head of a blocking queue.
 *
 * If a blocking queue is empty (that is, its size is equal to 0),
 * bq_take() blocks a calling thread until bq_put() inserts a new element.
 *
 * A thread can be cancelled while it is blocked by bq_take().
 *
 * @return non-NULL pointer to a taken element.
 */
void *bq_take(struct bq *queue);

/**
 * Non-blocking version of bq_take().
 *
 * @return non-NULL pointer to a taken element if success; NULL otherwise.
 */
void *bq_poll(struct bq *queue);

/**
 * Destroy a blocking queue.
 *
 * A destoryed queue never be reusable.
 *
 * @param dtor destroys each element in a blocking queue.
 *        If you want to do nothing for each, give it NULL.
 */
void bq_destroy(struct bq *queue, void (*dtor)(void *));

#endif /* BLOCKING_QUEUE_H */
