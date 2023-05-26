#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// https://datatracker.ietf.org/doc/html/draft-eastlake-fnv-03#page-15
static const struct test_vector {
    const char *input;
    uint64_t expected;
} TEST_VECTORS[] = {
    {"", 0xaf63bd4c8601b7df},
    {"a", 0x089be207b544f1e4},
    {"foobar", 0x34531ca7168b8f38},
    {NULL, 0},
};

static const uint64_t FNV_OFFSET_BASIS = 0xcbf29ce484222325;
static const uint64_t FNV_PRIME = 0x100000001b3;

// Imagine if this didn't hold...
_Static_assert(CHAR_BIT == 8, "CHAR_BIT != 8");

/// Return FNV-1a hash of input
static uint64_t fnv_hash(const size_t data_len,
                         const unsigned char data[data_len])
{
    extern const uint64_t FNV_OFFSET_BASIS;
    extern const uint64_t FNV_PRIME;

    uint64_t hash = FNV_OFFSET_BASIS;

    for (size_t i = 0; i < data_len; ++i) {
        hash ^= data[i];
        hash *= FNV_PRIME;
    }

    return hash;
}

static bool test_fnv_hash(void)
{
    extern const struct test_vector TEST_VECTORS[];

    printf("testing fnv_hash\n");

    const char *input = NULL;
    for (size_t i = 0; (input = TEST_VECTORS[i].input) != NULL; ++i) {
        const uint64_t expected = TEST_VECTORS[i].expected;
        const uint64_t actual = fnv_hash(strlen(input) + 1, (const unsigned char *)input);
        if (expected != actual) {
            (void)fprintf(stderr, "input: \"%s\", expected: %ld, actual: %ld\n",
                          input, expected, actual);
            return false;
        }
    }

    return true;
}

int main(void)
{
    assert(test_fnv_hash());

    return EXIT_SUCCESS;
}
