#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "feature.h"

#ifdef HAS_BLOCKS
#  include <Block.h>
#endif

static int answer = -1;

typedef struct node node;

struct node {
  int value;
  node *left;
  node *right;
};

node *node_create(int value, node *left, node *right) {
  node *ret = calloc(1, sizeof(*ret));
  assert(ret != NULL);
  ret->value = value;
  ret->left = left;
  ret->right = right;
  return ret;
}

void node_destroy(node *n) {
  if (n == NULL) {
    return;
  }
  node_destroy(n->left);
  node_destroy(n->right);
  free(n);
}

/* recursive */

int recursive_sum(node *n) {
  if (n == NULL) {
    return 0;
  }
  const int suml = recursive_sum(n->left);
  const int sumr = recursive_sum(n->right);
  return suml + sumr + n->value;
}

#ifdef HAS_NESTED_FUNCTIONS
/* cps with gcc nested functions */

void nested_sum_impl(node *n, void k(int)) {
  if (n == NULL) {
    k(0);
  } else {
    void k1(int s0) {
      void k2(int s1) { k(s0 + s1 + n->value); }
      nested_sum_impl(n->right, k2);
    }
    nested_sum_impl(n->left, k1);
  }
}

void nested_sum(node *n) {
  void k3(int s) { answer = s; }
  nested_sum_impl(n, k3);
}
#endif

#ifdef HAS_BLOCKS
/* cps with clang blocks */

typedef void (^kontb)(int);

void blocks_sum_impl(node *n, kontb k) {
  if (n == NULL) {
    k(0);
  } else {
    kontb k1 = Block_copy(^(int s0) {
      kontb k2 = Block_copy(^(int s1) { k(s0 + s1 + n->value); });
      blocks_sum_impl(n->right, k2);
      Block_release(k2);
    });
    blocks_sum_impl(n->left, k1);
    Block_release(k1);
  }
}

void blocks_sum(node *n) {
  kontb k3 = ^(int s) { answer = s; };
  blocks_sum_impl(n, k3);
}
#endif

/* defunctionalization */

typedef enum kont_tag {
  K1,
  K2,
  K3,
} kont_tag;

typedef struct kont kont;

struct kont {
  kont_tag tag;
  union {
    struct {
      node *n;
      kont *k;
    } k1;
    struct {
      int s0;
      node *n;
      kont *k;
    } k2;
  } u;
};

#define KONT_ARENA_SIZE 128

typedef struct kont_allocator kont_allocator;

/* simple arena allocator for konts */
struct kont_allocator {
  size_t count;
  size_t capacity;
  kont konts[];
};

kont_allocator *kont_allocator_create(size_t capacity) {
  kont_allocator *ret = calloc(1, sizeof(*ret) + (capacity * sizeof(kont)));
  assert(ret != NULL);
  ret->count = 0;
  ret->capacity = capacity;
  return ret;
}

void kont_allocator_destroy(kont_allocator *alloc) {
  free(alloc);
}

kont *kont_allocate(kont_allocator *alloc) {
  assert(alloc->count < alloc->capacity);
  return &alloc->konts[alloc->count++];
}

kont *defunc_kont_k1(kont_allocator *alloc, node *n, kont *k) {
  kont *k1 = kont_allocate(alloc);
  {
    k1->tag = K1;
    k1->u.k1.n = n;
    k1->u.k1.k = k;
  }
  return k1;
}

kont *defunc_kont_k2(kont_allocator *alloc, int s0, node *n, kont *k) {
  kont *k2 = kont_allocate(alloc);
  {
    k2->tag = K2;
    k2->u.k2.s0 = s0;
    k2->u.k2.n = n;
    k2->u.k2.k = k;
  }
  return k2;
}

kont *defunc_kont_k3(kont_allocator *alloc) {
  kont *k3 = kont_allocate(alloc);
  assert(k3 != NULL);
  {
    k3->tag = K3;
  }
  return k3;
}

void defunc_apply(kont_allocator *alloc, kont *k, int s);

void defunc_sum_impl(kont_allocator *alloc, node *n, kont *k) {
  if (n == NULL) {
    defunc_apply(alloc, k, 0);
  } else {
    kont *k1 = defunc_kont_k1(alloc, n, k);
    defunc_sum_impl(alloc, n->left, k1);
  }
}

void defunc_apply(kont_allocator *alloc, kont *k, int s) {
  if (k->tag == K1) {
    kont *k2 = defunc_kont_k2(alloc, s, k->u.k1.n, k->u.k1.k);
    defunc_sum_impl(alloc, k->u.k1.n->right, k2);
  } else if (k->tag == K2) {
    defunc_apply(alloc, k->u.k2.k, k->u.k2.s0 + s + k->u.k2.n->value);
  } else if (k->tag == K3) {
    answer = s;
  }
}

void defunc_sum(node *n) {
  kont_allocator *alloc = kont_allocator_create(KONT_ARENA_SIZE);
  kont *k3 = defunc_kont_k3(alloc);
  defunc_sum_impl(alloc, n, k3);
  kont_allocator_destroy(alloc);
}

/* traditional stack iteration */

typedef struct stack_node stack_node;

struct stack_node {
  node *node;
  stack_node *next;
};

void push(stack_node **top, node *n) {
  stack_node *new = calloc(1, sizeof(*new));
  assert(new != NULL);
  new->node = n;
  new->next = *top;
  *top = new;
}

node *pop(stack_node **top) {
  if (*top == NULL) {
    return NULL;
  };
  stack_node *tmp = *top;
  node *ret = tmp->node;
  *top = (*top)->next;
  free(tmp);
  return ret;
}

void iterative_sum(node *root) {
  if (root == NULL) {
    answer = 0;
  }

  int sum = 0;
  stack_node *stack = NULL;
  push(&stack, root);

  node *curr = NULL;
  while (stack != NULL) {
    curr = pop(&stack);
    sum += curr->value;
    if (curr->right) {
      push(&stack, curr->right);
    }
    if (curr->left) {
      push(&stack, curr->left);
    }
  }

  answer = sum;
}

#define BRANCH(value, left, right) node_create(value, left, right)
#define LEAF(value)                node_create(value, NULL, NULL)

typedef struct algo algo;

struct algo {
  char *name;
  void (*f)(node *n);
};

int main(void) {
  algo algos[] = {
#ifdef HAS_NESTED_FUNCTIONS
    {"cps (w/nested functions)", &nested_sum},
#endif
#ifdef HAS_BLOCKS
    {"cps (w/blocks)", &blocks_sum},
#endif
    {"defunc", &defunc_sum},
    {"iterative", &iterative_sum},
    {NULL, NULL},
  };

  node *ns[] = {
    LEAF(123),
    BRANCH(1, LEAF(2), NULL),
    BRANCH(1, NULL, LEAF(2)),
    BRANCH(1, LEAF(200), LEAF(3)),
    BRANCH(1, LEAF(2), BRANCH(3, LEAF(4), LEAF(5))),
    BRANCH(1, BRANCH(3, LEAF(4), LEAF(5)), LEAF(2)),
    NULL,
  };

  algo a = {0};
  node *n = NULL;
  int ref = 0;

  for (size_t i = 0; (a = algos[i]).name != NULL; ++i) {
    printf("=== %s ===\n", a.name);
    for (size_t j = 0; (n = ns[j]) != NULL; ++j) {
      ref = recursive_sum(n);
      answer = -1;
      a.f(n);
      assert(ref == answer);
      printf("sum: %d\n", answer);
    }
  }

  for (size_t i = 0; (n = ns[i]) != NULL; ++i) {
    node_destroy(n);
  }

  return EXIT_SUCCESS;
}
