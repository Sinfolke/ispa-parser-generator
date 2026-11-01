/* ispa_lexer.h - the native half of generated lexers: running a generated lexer's DFA tables through the lowered scanner.
 * Application code is built against lowered/ispa_std.h; add ispa_lexer.c to the sources. */
#ifndef ISPA_LEXER_H
#define ISPA_LEXER_H
#include "ispa_std.h"

/* Scan one token at `pos` with generated tables. `semantic` is the generated action
 *   Pair<state, token> (void *env, state, captures, start_pos, start, length, line, cursor)
 * where `start` and `cursor` are slices of the source text from the match start / the scan cursor. */
python_ispa_std_raw_token_RawToken *ispa_lexer_lookup(python_ispa_std_lexer_Lexer_base *self, ispa_vec_ispa_vec_int64_tP *table,
                                                      ispa_vec_int64_t *classes, ispa_vec_ispa_vec_int64_tP *actions,
                                                      python_ispa_std_dfa_api_TdfaLayout *layout, ispa_fn semantic,
                                                      python_ispa_std_refs_Cursor *pos);
#endif
