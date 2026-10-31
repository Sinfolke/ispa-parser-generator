#include "../ispa_c.h"
#include <assert.h>
int main(void) {
    ispa_arena a; ispa_arena_init(&a, 256, NULL);
    ispa_vec v; ispa_vec_init(&v, &a, sizeof(int64_t));
    for (int64_t i = 0; i < 1000; ++i) *(int64_t *)ispa_vec_push(&v) = i * 3;
    assert(v.len == 1000 && *(int64_t *)ispa_vec_at(&v, 999) == 2997);
    ispa_str s = ispa_str_concat(&a, ISPA_STR("foo"), ISPA_STR("bar"));
    assert(ispa_str_eq(s, ISPA_STR("foobar")));
    ispa_arena_mark m = ispa_arena_mark_get(&a);
    for (int i = 0; i < 100; ++i) ispa_arena_alloc(&a, 1000);
    ispa_arena_rewind(&a, m);
    assert(ispa_str_eq(s, ISPA_STR("foobar")));  /* data before the mark survives a rewind */
    ispa_arena_free(&a);
    puts("runtime ok");
    return 0;
}
