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

/* ---- map: open addressing keyed by byte strings or integers (filled in with the lowered dict users) ---- */
typedef struct ispa_map { void *impl; ispa_arena *arena; } ispa_map;

/* ---- any: type-erased value, tag = a per-type address (filled in by the emitter) ---- */
typedef struct ispa_any { const void *tag; void *value; } ispa_any;

/* ---- callable: function pointer + closure environment (lambdas are hoisted by the emitter) ---- */
typedef struct ispa_fn { void *fn; void *env; } ispa_fn;

#ifdef __cplusplus
}
#endif
#endif
