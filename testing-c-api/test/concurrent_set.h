#ifndef CONCURRENT_SET_H
#define CONCURRENT_SET_H

#include <stdbool.h>

struct cs;

struct cs *cs_new(
	int (*cmpr)(const void *, const void *),
	void (*dtor)(void *));

int cs_size(struct cs *set);

bool cs_contains(struct cs *set, const void *key);

int cs_add(struct cs *set, const void *key);

int cs_remove(struct cs *set, const void *key);

void cs_destroy(struct cs *set);

#endif /* CONCURRENT_SET_H */
