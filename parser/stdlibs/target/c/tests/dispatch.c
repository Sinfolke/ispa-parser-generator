/* Virtual dispatch through a base pointer: only overridden methods go through a vtable.
 *   cc -std=c11 -I.. -I../lowered tests/dispatch.c lowered/ispa_std.c ispa_c.c */
#include "../lowered/ispa_std.h"
#include <assert.h>
int main(void) {
    ispa_arena a; ispa_arena_init(&a, 1 << 16, NULL); ispa_cur = &a;
    python_ispa_std_lexer_TokenIterator *it = python_ispa_std_lexer_TokenIterator_new(NULL, 3);
    python_ispa_std_lexer_TokenCursor *base = (python_ispa_std_lexer_TokenCursor *)it;
    python_ispa_std_lexer_TokenCursor_next_v(base);            /* TokenIterator::next (override), not TokenCursor::next */
    assert(it->token_index == 4);
    python_ispa_std_lexer_TokenCursor_prev_v(base);
    python_ispa_std_lexer_TokenCursor_prev_v(base);
    assert(it->token_index == 2);
    ispa_arena_free(&a);
    puts("dispatch ok");
    return 0;
}
