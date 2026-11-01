/* The lowered Python runtime executes in C: strings, sorted maps, the DFA scanner.
 *   cc -std=c11 -I. -Ilowered tests/lowered.c lowered/ispa_std.c ispa_c.c */
#include "../lowered/ispa_std.h"
#include <assert.h>

#define NS(x) python_ispa_std_##x
static int64_t accepted_len;

/* semantic callback: Callable[[int, Captures, int, int, int, Cursor], Pair[int, RawToken]] */
static NS(utility_Pair__int64_t_python_ispa_std_raw_token_RawTokenP) *semantic(void *env, int64_t index, NS(dfa_api_Captures) *captures,
                                                                              int64_t start, int64_t length, int64_t line, NS(refs_Cursor) *cursor) {
    (void)index; (void)captures; (void)start; (void)line; (void)cursor;
    accepted_len = length;
    return NS(utility_Pair__int64_t_python_ispa_std_raw_token_RawTokenP_new)(NS(dfa_api_NULL_STATE), (NS(raw_token_RawToken) *)env);
}

static NS(dfa_run_DfaTables) *tables(void) {   /* DFA for [a-z]+ */
    ispa_vec_int64_t *classes = ispa_vec_int64_t_new();
    for (int i = 0; i < 256; ++i) ispa_vec_int64_t_push(classes, (i >= 'a' && i <= 'z') ? 1 : 0);
    ispa_vec_int64_t *r0 = ispa_vec_int64_t_new(), *r1 = ispa_vec_int64_t_new();
    ispa_vec_int64_t_push(r0, NS(dfa_api_NULL_STATE)); ispa_vec_int64_t_push(r0, 1);
    ispa_vec_int64_t_push(r1, 2);                      ispa_vec_int64_t_push(r1, 1);
    ispa_vec_ispa_vec_int64_tP *table = ispa_vec_ispa_vec_int64_tP_new();
    ispa_vec_ispa_vec_int64_tP_push(table, r0); ispa_vec_ispa_vec_int64_tP_push(table, r1);
    return NS(dfa_run_DfaTables_new)(table, classes, ispa_vec_ispa_vec_int64_tP_new(), NS(dfa_api_TdfaLayout_new)(0, 0),
                                     ispa_vec_python_ispa_std_dfa_api_DFADebugP_new(), NS(dfa_api_DebugIndex_new)());
}

static int64_t scan(const char *text, int64_t *end) {
    NS(raw_token_RawToken) *empty = NS(raw_token_RawToken_empty_token)();
    NS(refs_Cursor) *cur = NS(refs_Cursor_new)(0, 1);
    ispa_fn cb = { (void *)semantic, empty };
    accepted_len = -1;
    NS(dfa_run_run)(NS(dfa_api_SourceText_new)(ispa_str_from(ispa_cur, text, strlen(text))), cur, 0, tables(), cb, empty);
    *end = cur->pos;
    return accepted_len;
}

int main(void) {
    ispa_arena a; ispa_arena_init(&a, 1 << 16, NULL); ispa_cur = &a;
    ispa_std_init_globals();
    assert(ispa_str_eq(NS(format_int_to_string)(-42), ISPA_STR("-42")));
    assert(ispa_str_eq(NS(format_repeat)(ISPA_STR("ab"), 3), ISPA_STR("ababab")));

    NS(sortedmap_StrIntMap) *m = NS(sortedmap_StrIntMap_new)();
    NS(sortedmap_StrIntMap___init__)(m);
    assert(NS(sortedmap_StrIntMap_try_emplace)(m, ISPA_STR("pear"), 3));
    assert(NS(sortedmap_StrIntMap_try_emplace)(m, ISPA_STR("apple"), 1));
    assert(NS(sortedmap_StrIntMap_try_emplace)(m, ISPA_STR("fig"), 2));
    assert(!NS(sortedmap_StrIntMap_try_emplace)(m, ISPA_STR("fig"), 9));
    assert(NS(sortedmap_StrIntMap_get)(m, ISPA_STR("fig")) == 2);
    assert(NS(sortedmap_StrIntMap_contains)(m, ISPA_STR("apple")));
    assert(!NS(sortedmap_StrIntMap_contains)(m, ISPA_STR("plum")));

    int64_t end;
    assert(scan("abc def", &end) == 3 && end == 3);
    assert(scan("x", &end) == 1 && end == 1);
    ispa_arena_free(&a);
    puts("lowered ok");
    return 0;
}
