#include "ispa_c.h"

#define ALIGN16(n) (((n) + 15u) & ~(size_t)15u)
#define HDR ALIGN16(sizeof(ispa_block))

static void *raw_alloc(ispa_arena *a, size_t n) { return a->fns.alloc ? a->fns.alloc(a->fns.user, n) : malloc(n); }
static void raw_free(ispa_arena *a, void *p) { if (a->fns.release) a->fns.release(a->fns.user, p); else free(p); }

static ispa_block *new_block(ispa_arena *a, size_t min) {
    size_t cap = a->block_size > min ? a->block_size : min;
    ispa_block *b = (ispa_block *)raw_alloc(a, HDR + cap);
    if (!b) { fprintf(stderr, "ispa: out of memory\n"); abort(); }
    b->next = a->head; b->used = 0; b->cap = cap;
    a->head = b;
    return b;
}

void ispa_arena_init(ispa_arena *a, size_t block_size, const ispa_alloc_fns *fns) {
    memset(a, 0, sizeof *a);
    a->block_size = block_size ? block_size : 64 * 1024;
    if (fns) a->fns = *fns;
}
void *ispa_arena_alloc(ispa_arena *a, size_t size) {
    size = ALIGN16(size ? size : 1);
    ispa_block *b = a->head;
    if (!b || b->cap - b->used < size) b = new_block(a, size);
    void *p = (char *)b + HDR + b->used;
    b->used += size;
    return p;
}
void *ispa_arena_calloc(ispa_arena *a, size_t size) { void *p = ispa_arena_alloc(a, size); memset(p, 0, size); return p; }
void ispa_arena_reset(ispa_arena *a) {
    while (a->head && a->head->next) { ispa_block *n = a->head->next; raw_free(a, a->head); a->head = n; }
    if (a->head) a->head->used = 0;
}
void ispa_arena_free(ispa_arena *a) {
    while (a->head) { ispa_block *n = a->head->next; raw_free(a, a->head); a->head = n; }
}
ispa_arena_mark ispa_arena_mark_get(const ispa_arena *a) { ispa_arena_mark m = { a->head, a->head ? a->head->used : 0 }; return m; }
void ispa_arena_rewind(ispa_arena *a, ispa_arena_mark m) {
    while (a->head != m.block) { ispa_block *n = a->head->next; raw_free(a, a->head); a->head = n; }
    if (a->head) a->head->used = m.used;
}

ispa_str ispa_str_from(ispa_arena *a, const char *data, size_t len) {
    char *p = (char *)ispa_arena_alloc(a, len + 1);
    memcpy(p, data, len); p[len] = 0;
    return (ispa_str){ p, len };
}
ispa_str ispa_str_concat(ispa_arena *a, ispa_str x, ispa_str y) {
    char *p = (char *)ispa_arena_alloc(a, x.len + y.len + 1);
    memcpy(p, x.data, x.len); memcpy(p + x.len, y.data, y.len); p[x.len + y.len] = 0;
    return (ispa_str){ p, x.len + y.len };
}
bool ispa_str_eq(ispa_str x, ispa_str y) { return x.len == y.len && memcmp(x.data, y.data, x.len) == 0; }

void ispa_vec_init(ispa_vec *v, ispa_arena *a, size_t elem_size) { v->data = NULL; v->len = v->cap = 0; v->elem = elem_size; v->arena = a; }
void *ispa_vec_push(ispa_vec *v) {
    if (v->len == v->cap) {
        size_t nc = v->cap ? v->cap * 2 : 4;
        void *nd = ispa_arena_alloc(v->arena, nc * v->elem);
        if (v->len) memcpy(nd, v->data, v->len * v->elem);
        v->data = nd; v->cap = nc;
    }
    void *slot = (char *)v->data + v->len * v->elem;
    memset(slot, 0, v->elem);
    v->len++;
    return slot;
}
void *ispa_vec_at(const ispa_vec *v, size_t i) { return i < v->len ? (char *)v->data + i * v->elem : NULL; }
void ispa_vec_clear(ispa_vec *v) { v->len = 0; }

_Thread_local ispa_arena *ispa_cur;

static _Thread_local ispa_jmp *ispa_jmp_top;
static _Thread_local int64_t ispa_fail_kind;
static _Thread_local ispa_str ispa_fail_message;
_Noreturn void ispa_fail(int64_t kind, ispa_str message) {
    if (!ispa_jmp_top) { fprintf(stderr, "ispa: error %lld: %.*s\n", (long long)kind, (int)message.len, message.data); abort(); }
    ispa_fail_kind = kind; ispa_fail_message = message;
    longjmp(ispa_jmp_top->buf, 1);
}
int64_t ispa_attempt(void (*fn)(void *env), void *env) {
    ispa_jmp j; j.prev = ispa_jmp_top; ispa_jmp_top = &j;
    if (setjmp(j.buf) == 0) { fn(env); ispa_jmp_top = j.prev; return 0; }
    ispa_jmp_top = j.prev;
    return ispa_fail_kind;
}
ispa_str ispa_failure_message(void) { return ispa_fail_message; }
int64_t ispa_attempt_fn(ispa_fn f) { return ispa_attempt((void (*)(void *))f.fn, f.env); }
const char ispa_tag_int = 0, ispa_tag_ptr = 0, ispa_tag_str = 0;
ispa_any ispa_any_str(ispa_str s) { ispa_str *p = (ispa_str *)ispa_arena_alloc(ispa_cur, sizeof(ispa_str)); *p = s; return (ispa_any){ &ispa_tag_str, p }; }
void *ispa_clone_obj(const void *obj, size_t size) { void *p = ispa_arena_alloc(ispa_cur, size); memcpy(p, obj, size); return p; }
