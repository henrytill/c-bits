#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fnv.h"
#include "prelude.h"

struct entry {
	struct entry *next;
	const char *key;
	void *value;
};

struct table {
	size_t columns_len;
	struct entry *columns;
};

static struct table *table_create(const size_t columns_len)
{
	if ((columns_len & (columns_len - 1)) != 0) {
		(void)fprintf(stderr, "Error: columns_len must be a power of 2\n");
		return NULL;
	}

	struct table *ret = emalloc(sizeof(*ret));
	ret->columns = ecalloc(columns_len, sizeof(*ret->columns));
	ret->columns_len = columns_len;
	return ret;
}

static void table_destroy(struct table *t)
{
	struct entry *curr;
	struct entry *next;

	if (t == NULL) {
		return;
	}

	for (size_t i = 0; i < t->columns_len; ++i) {
		for (curr = t->columns[i].next; curr != NULL;) {
			next = curr->next;
			free(curr);
			curr = next;
		}
	}
	free(t->columns);
	free(t);
}

static int table_put(struct table *t, const char *key, void *value)
{
	const uint64_t hash = fnv_hash(strlen(key) + 1, (const unsigned char *)key);
	const ptrdiff_t index = (ptrdiff_t)(hash & (uint64_t)(t->columns_len - 1));
	struct entry *curr = t->columns + index;
	struct entry *prev = NULL;

	while (curr != NULL && curr->key != NULL && strcmp(key, curr->key) != 0) {
		prev = curr;
		curr = curr->next;
	}

	// first node
	if (curr != NULL && curr->key == NULL) {
		curr->key = key;
		curr->value = value;
		return 0;
	}

	// existing node
	if (curr != NULL && curr->key != NULL) {
		assert(strcmp(curr->key, key) == 0);
		curr->value = value;
		return 0;
	}

	// new node
	assert(curr == NULL);

	curr = ecalloc(1, sizeof(*curr));

	curr->next = NULL;
	curr->key = key;
	curr->value = value;
	prev->next = curr;

	return 0;
}

static void *table_get(struct table *t, const char *key)
{
	const uint64_t hash = fnv_hash(strlen(key) + 1, (const unsigned char *)key);
	const ptrdiff_t index = (ptrdiff_t)(hash & (uint64_t)(t->columns_len - 1));
	struct entry *curr = t->columns + index;

	while (curr != NULL && strcmp(key, curr->key) != 0) {
		curr = curr->next;
	}

	if (curr == NULL) {
		return NULL;
	}

	return curr->value;
}

int main(void)
{
	int ret = EXIT_FAILURE;

	if (fnv_hash_test() == false) {
		return EXIT_FAILURE;
	}

	const char *key = "foo";
	char *value = "bar";
	struct table *t = table_create(16);

	(void)table_put(t, key, value);
	char *ret_value = table_get(t, key);
	if (ret_value == NULL) {
		goto out_table_destroy;
	}

	(void)printf("%s: %s\n", key, ret_value);

	ret = EXIT_SUCCESS;
out_table_destroy:
	table_destroy(t);
	return ret;
}
