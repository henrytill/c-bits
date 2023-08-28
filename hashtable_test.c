#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fnv.h"
#include "hashtable.h"

static const struct test_vector {
  const char *key;
  char *value;
} TEST_VECTORS[] = {
#define X(prefix) {#prefix "_key", #prefix "_value"},
#include "hashtable_vectors.def"
#undef X
  {NULL, NULL},
};

int main(void) {
  extern const struct test_vector TEST_VECTORS[];

  if (fnv_hash_test() == false) {
    return EXIT_FAILURE;
  }

  int ret = EXIT_FAILURE;
  const char *key = NULL;
  struct table *t = table_create(8);

  for (size_t i = 0; (key = TEST_VECTORS[i].key) != NULL; ++i) {
    table_put(t, key, TEST_VECTORS[i].value);
  }

  for (size_t i = 0; (key = TEST_VECTORS[i].key) != NULL; ++i) {
    char *value = table_get(t, key);
    if (strcmp(TEST_VECTORS[i].value, value) != 0) {
      goto out_table_destroy;
    }
  }

  ret = EXIT_SUCCESS;
out_table_destroy:
  table_destroy(t);
  return ret;
}
