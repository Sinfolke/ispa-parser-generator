#include "ispa_lexer.h"

typedef python_ispa_std_utility_Pair__int64_t_python_ispa_std_raw_token_RawTokenP ispa_pair;
typedef ispa_pair *(*ispa_semantic_fn)(void *, int64_t, python_ispa_std_dfa_api_Captures *, int64_t, ispa_str, int64_t, int64_t, ispa_str);
typedef struct { ispa_fn user; ispa_str text; } ispa_scan_env;

/* the core's semantic callback (index, captures, first, length, line, cursor) -> the generated 7-argument action */
static ispa_pair *ispa_scan_adapter(void *env_, int64_t index, python_ispa_std_dfa_api_Captures *captures, int64_t first, int64_t length,
                                    int64_t line, python_ispa_std_refs_Cursor *cur) {
    ispa_scan_env *e = (ispa_scan_env *)env_;
    ispa_str start = { e->text.data + first, e->text.len - (size_t)first };
    ispa_str at = { e->text.data + cur->pos, e->text.len - (size_t)cur->pos };
    return ((ispa_semantic_fn)e->user.fn)(e->user.env, index, captures, first, start, length, line, at);
}

/* a lexer scans with the same tables token after token: build the core's DfaTables once per generated table */
enum { ISPA_TABLE_CACHE = 16 };
static _Thread_local struct { ispa_vec_ispa_vec_int64_tP *key; python_ispa_std_dfa_run_DfaTables *tables; } ispa_table_cache[ISPA_TABLE_CACHE];
static _Thread_local int ispa_table_cache_n;

static python_ispa_std_dfa_run_DfaTables *ispa_tables(ispa_vec_ispa_vec_int64_tP *table, ispa_vec_int64_t *classes,
                                                      ispa_vec_ispa_vec_int64_tP *actions, python_ispa_std_dfa_api_TdfaLayout *layout) {
    for (int i = 0; i < ispa_table_cache_n; ++i)
        if (ispa_table_cache[i].key == table) return ispa_table_cache[i].tables;
    python_ispa_std_dfa_run_DfaTables *t = python_ispa_std_dfa_run_DfaTables_new(
        table, classes, actions, layout, ispa_vec_python_ispa_std_dfa_api_DFADebugP_new(), python_ispa_std_dfa_api_DebugIndex_new());
    if (ispa_table_cache_n < ISPA_TABLE_CACHE) { ispa_table_cache[ispa_table_cache_n].key = table; ispa_table_cache[ispa_table_cache_n].tables = t; ++ispa_table_cache_n; }
    return t;
}

python_ispa_std_raw_token_RawToken *ispa_lexer_lookup(python_ispa_std_lexer_Lexer_base *self, ispa_vec_ispa_vec_int64_tP *table,
                                                      ispa_vec_int64_t *classes, ispa_vec_ispa_vec_int64_tP *actions,
                                                      python_ispa_std_dfa_api_TdfaLayout *layout, ispa_fn semantic,
                                                      python_ispa_std_refs_Cursor *pos) {
    ispa_scan_env env = { semantic, self->_src->text };
    ispa_fn adapter = { (void *)ispa_scan_adapter, &env };
    return python_ispa_std_lexer_Lexer_base_lookup(self, ispa_tables(table, classes, actions, layout), adapter, pos);
}
