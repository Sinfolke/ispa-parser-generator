/* ispa_c.h - runtime for the ISPA C target. C11, libc only.
 *
 * Memory model: arenas. Every allocation made by a parse comes from one of two arenas owned by the parser handle:
 *   - the RESULT arena holds AST nodes, tokens, diagnostics and the strings they point to; it can be detached with
 *     ispa_arena_detach so the AST outlives the parser;
 *   - the SCRATCH arena holds contexts, probes, recovery state and failed-alternative garbage; it is reset per parse.
 * A result-arena object must only point into the result arena (checked by ISPA_CHECK_ESCAPE in debug builds).
 * Nothing allocated here has a destructor: the lowered core owns no external resources.
 */
#ifndef ISPA_C_H
#define ISPA_C_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- allocator hooks (optional) ---- */
typedef struct ispa_alloc_fns {
    void *(*alloc)(void *user, size_t size);
    void (*release)(void *user, void *ptr);
    void *user;
} ispa_alloc_fns;

/* ---- arena ---- */
typedef struct ispa_block ispa_block;
struct ispa_block { ispa_block *next; size_t used, cap; /* data follows, 16-byte aligned */ };
typedef struct ispa_arena {
    ispa_block *head;
    size_t block_size;
    ispa_alloc_fns fns;
    bool result;                    /* tag used by the debug escape checker */
} ispa_arena;

void ispa_arena_init(ispa_arena *a, size_t block_size, const ispa_alloc_fns *fns /* NULL = malloc/free */);
void *ispa_arena_alloc(ispa_arena *a, size_t size);
void *ispa_arena_calloc(ispa_arena *a, size_t size);
void ispa_arena_reset(ispa_arena *a);     /* keeps the first block, frees the rest */
void ispa_arena_free(ispa_arena *a);
/* mark/rewind: only valid for allocations proven not to escape (see README, backtracking) */
typedef struct { ispa_block *block; size_t used; } ispa_arena_mark;
ispa_arena_mark ispa_arena_mark_get(const ispa_arena *a);
void ispa_arena_rewind(ispa_arena *a, ispa_arena_mark m);

/* ---- error state: replaces exceptions ---- */
typedef struct ispa_error { int kind; const char *message; } ispa_error;
#define ISPA_UNIMPLEMENTED(what) do { fprintf(stderr, "ispa: not implemented: %s\n", what); abort(); } while (0)

/* ---- strings: bytes (Python str semantics in the lowered core are byte strings) ---- */
typedef struct ispa_str { const char *data; size_t len; } ispa_str;
#define ISPA_STR(lit) ((ispa_str){ (lit), sizeof(lit) - 1 })
ispa_str ispa_str_from(ispa_arena *a, const char *data, size_t len);
ispa_str ispa_str_concat(ispa_arena *a, ispa_str x, ispa_str y);
bool ispa_str_eq(ispa_str x, ispa_str y);

/* ---- growable vector of fixed-size elements (doubling inside the arena: waste <= 2x) ---- */
typedef struct ispa_vec { void *data; size_t len, cap, elem; ispa_arena *arena; } ispa_vec;
void ispa_vec_init(ispa_vec *v, ispa_arena *a, size_t elem_size);
void *ispa_vec_push(ispa_vec *v);                 /* returns the new (zeroed) slot */
void *ispa_vec_at(const ispa_vec *v, size_t i);
void ispa_vec_clear(ispa_vec *v);                 /* just resets the length */

/* ---- callable: function pointer + closure environment (lambdas are hoisted by the emitter) ---- */
typedef struct ispa_fn { void *fn; void *env; } ispa_fn;

/* ---- errors: fail() jumps to the innermost attempt(); with none installed the process aborts. Arena memory needs no cleanup. ---- */
typedef struct ispa_jmp { jmp_buf buf; struct ispa_jmp *prev; } ispa_jmp;
_Noreturn void ispa_fail(int64_t kind, ispa_str message);
int64_t ispa_attempt(void (*fn)(void *env), void *env);   /* 0 on completion, else the kind passed to ispa_fail */
ispa_str ispa_failure_message(void);
int64_t ispa_attempt_fn(ispa_fn f);   /* f: void (*)(void *env) */

/* ---- current arena: allocation sites in lowered code take it from here (set by the parser facade per parse) ---- */
extern _Thread_local ispa_arena *ispa_cur;
#define ISPA_NEW(T) ((T *)ispa_arena_calloc(ispa_cur, sizeof(T)))
#define ISPA_CONCAT(a, b) ispa_str_concat(ispa_cur, (a), (b))
static inline int64_t ispa_idx(int64_t i, size_t len) {
    if (i < 0) i += (int64_t)len;
    if (i < 0 || (size_t)i >= len) { fprintf(stderr, "ispa: index out of range\n"); abort(); }
    return i;
}
static inline int ispa_str_cmp(ispa_str a, ispa_str b) {
    size_t n = a.len < b.len ? a.len : b.len; int c = memcmp(a.data, b.data, n);
    return c ? c : (a.len > b.len) - (a.len < b.len);
}
static inline ispa_str ispa_str_from_char(char c) { char *p = (char *)ispa_arena_alloc(ispa_cur, 2); p[0] = c; p[1] = 0; return (ispa_str){ p, 1 }; }
static inline char ispa_str_at(ispa_str s, int64_t i) { return s.data[ispa_idx(i, s.len)]; }

/* ---- typed vector (Python list: reference semantics, so always handled through NAME*) ----
 * ISPA_DEFINE_VEC(ispa_vec_int64_t, int64_t) defines the struct and NAME_new/_push/_pop/_get/_set/_extend/_clear. */
#define ISPA_DEFINE_VEC(NAME, T)                                                                              \
    typedef struct NAME { T *data; size_t len, cap; } NAME;                                                    \
    static inline NAME *NAME##_new(void) { return ISPA_NEW(NAME); }                                            \
    static inline void NAME##_reserve(NAME *v, size_t n) {                                                     \
        if (n <= v->cap) return;                                                                               \
        size_t nc = v->cap ? v->cap : 4; while (nc < n) nc *= 2;                                               \
        T *nd = (T *)ispa_arena_alloc(ispa_cur, nc * sizeof(T));                                               \
        if (v->len) memcpy(nd, v->data, v->len * sizeof(T));                                                   \
        v->data = nd; v->cap = nc;                                                                             \
    }                                                                                                          \
    static inline NAME *NAME##_from(T const *p, size_t n) { NAME *v = NAME##_new(); NAME##_reserve(v, n); if (n) memcpy(v->data, p, n * sizeof(T)); v->len = n; return v; } \
    static inline void NAME##_push(NAME *v, T x) { NAME##_reserve(v, v->len + 1); v->data[v->len++] = x; }     \
    static inline T NAME##_pop(NAME *v) { if (!v->len) { fprintf(stderr, "ispa: pop from empty\n"); abort(); } return v->data[--v->len]; } \
    static inline T NAME##_get(const NAME *v, int64_t i) { return v->data[ispa_idx(i, v->len)]; }              \
    static inline void NAME##_set(NAME *v, int64_t i, T x) { v->data[ispa_idx(i, v->len)] = x; }               \
    static inline void NAME##_extend(NAME *v, const NAME *o) { for (size_t k = 0; k < o->len; ++k) NAME##_push(v, o->data[k]); } \
    static inline void NAME##_insert(NAME *v, int64_t i, T x) {                                                \
        if (i < 0) { i += (int64_t)v->len; } if (i < 0) { i = 0; } if ((size_t)i > v->len) { i = (int64_t)v->len; }\
        NAME##_reserve(v, v->len + 1);                                                                         \
        memmove(v->data + i + 1, v->data + i, (v->len - (size_t)i) * sizeof(T)); v->data[i] = x; v->len++;     \
    }                                                                                                          \
    static inline void NAME##_clear(NAME *v) { v->len = 0; }

/* ---- map: open addressing keyed by byte strings or integers (filled in with the lowered dict users) ---- */
typedef struct ispa_map { void *impl; ispa_arena *arena; } ispa_map;

/* ---- any: type-erased value, tag = a per-type address (filled in by the emitter) ---- */
typedef struct ispa_any { const void *tag; void *value; } ispa_any;
extern const char ispa_tag_int, ispa_tag_ptr, ispa_tag_str;
#define ISPA_ANY_NONE ((ispa_any){ NULL, NULL })
#define ISPA_ANY_INT(x) ((ispa_any){ &ispa_tag_int, (void *)(intptr_t)(x) })
#define ISPA_ANY_PTR(p) ((ispa_any){ &ispa_tag_ptr, (void *)(p) })
ispa_any ispa_any_str(ispa_str s);


#ifdef __cplusplus
}
#endif
#endif
