#define _GNU_SOURCE /* tdestroy from search.h */

#include <errno.h>
#include <pthread.h>
#include <search.h>
#include <stdbool.h>
#include <stdlib.h>

#include "concurrent_set.h"

#define unlikely(x) __builtin_expect(!!(x), 0)

static void do_nothing(void *);
static int cs_add_unsafe(struct cs *set, const void *key);
static int cs_remove_unsafe(struct cs *set, const void *key);
static void *cs_find_unsafe(const struct cs *set, const void *key);
static void *cs_search_unsafe(struct cs *set, const void *key);
static void *cs_delete_unsafe(struct cs *set, const void *key);

struct cs {
	int size_;
	void *root_;
	int (*cmpr_)(const void *, const void *);
	void (*dtor_)(void *);
	pthread_mutex_t mutex_;
};

struct cs *cs_new(
	int (*cmpr)(const void *, const void *),
	void (*dtor)(void *))
{
	if (unlikely(!cmpr)) {
		errno = EINVAL;
		return NULL;
	}

	struct cs *const set = malloc(sizeof(struct cs));
	if (unlikely(!set))
		return NULL;

	set->size_ = 0;
	set->root_ = NULL;
	set->cmpr_ = cmpr;
	set->dtor_ = dtor ? dtor : do_nothing;
	pthread_mutex_init(&set->mutex_, NULL);
	return set;
}

int cs_size(struct cs *set)
{
	pthread_mutex_lock(&set->mutex_);
	const int ret = set->size_; /* TODO lock-free read */
	pthread_mutex_unlock(&set->mutex_);
	return ret;
}

int cs_add(struct cs *set, const void *key)
{
	if (unlikely(!key))
		return -EINVAL;

	pthread_mutex_lock(&set->mutex_);
	const int ret = cs_add_unsafe(set, key);
	pthread_mutex_unlock(&set->mutex_);
	return ret;
}

int cs_remove(struct cs *set, const void *key)
{
	if (unlikely(!key))
		return -EINVAL;

	pthread_mutex_lock(&set->mutex_);
	const int ret = cs_remove_unsafe(set, key);
	pthread_mutex_unlock(&set->mutex_);
	return ret;
}

bool cs_contains(struct cs *set, const void *key)
{
	if (unlikely(!key))
		return false; /* inval */

	pthread_mutex_lock(&set->mutex_);
	const bool ret = cs_find_unsafe(set, key);
	pthread_mutex_unlock(&set->mutex_);
	return ret;
}

void cs_destroy(struct cs *set)
{
	tdestroy(set->root_, set->dtor_);
	free(set);
}

static void do_nothing(void *dummy)
{
	(void)dummy;
}

static inline int cs_add_unsafe(struct cs *set, const void *key)
{
	if (cs_find_unsafe(set, key))
		return EEXIST;

	if (unlikely(!cs_search_unsafe(set, key)))
		return -ENOMEM;

	++set->size_;
	return 0;
}

static inline int cs_remove_unsafe(struct cs *set, const void *key)
{
	void **const found = cs_find_unsafe(set, key);
	if (!found)
		return ENOENT;

	/*
	 * dereference "found" here
	 * because it will be destroyed by cs_delete_unsafe()
	 */
	void *const tobefree = *found;

	if (unlikely(!cs_delete_unsafe(set, key)))
		return -ENOENT; /* should never be here */

	set->dtor_(tobefree);
	--set->size_;
	return 0;
}

static inline void *cs_find_unsafe(const struct cs *set, const void *key)
{
	return tfind(key, &set->root_, set->cmpr_);
}

static inline void *cs_search_unsafe(struct cs *set, const void *key)
{
	return tsearch(key, &set->root_, set->cmpr_);
}

static inline void *cs_delete_unsafe(struct cs *set, const void *key)
{
	return tdelete(key, &set->root_, set->cmpr_);
}
