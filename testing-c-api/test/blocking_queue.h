#ifndef BLOCKING_QUEUE_H
#define BLOCKING_QUEUE_H

/**
 * Blocking queue (opaque type).
 */
struct bq;

/**
 * Create a new blocking queue.
 *
 * @return a new blocking queue if success; NULL otherwise
 * @except EINVAL capacity <= 0
 * @except ENOMEM failed to allocate memory
 */
struct bq *bq_new(int capacity);

/**
 * Get the capacity of a blocking queue.
 */
int bq_capacity(const struct bq *queue);

/**
 * Get the number of element in a blocking queue.
 *
 * Though "queue" is not const,
 * this function never insert or remove new elements.
 */
int bq_size(struct bq *queue);

/**
 * Insert a new element into tail of a blocking queue.
 *
 * If the queue is full (that is, its size is equal to its capacity),
 * bq_put() blocks a calling thread until bq_take() removes an element.
 *
 * @return 1 always.
 */
_Bool bq_put(struct bq *queue, void *element);

/**
 * Non-blocking version of bq_put().
 *
 * @return 1 if success; 0 otherwise.
 */
_Bool bq_offer(struct bq *queue, void *element);

/**
 * Get and remove an element from head of a blocking queue.
 *
 * If the queue is empty (that is, its size is equal to 0),
 * bq_take() blocks a calling thread until bq_put() inserts a new element.
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
 * Destoryed queue never be reusable.
 */
void bq_destroy(struct bq *queue, void (*dtor)(void *));

#endif /* BLOCKING_QUEUE_H */
