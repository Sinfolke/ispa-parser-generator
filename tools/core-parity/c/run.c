/* Parses argv[1] with the generated C parser and prints the tree (or the failure) like rust/ref_main.cpp does for C++.
 * Built with -DNS=<prefix of the generated module> and the module's header included as "parser.h". */
#include "parser.h"
#include <stdlib.h>

#define CAT_(a, b) a##b
#define CAT(a, b) CAT_(a, b)
#define PARSER CAT(NS, _Parser)
#define LEXER CAT(NS, _Lexer)
#define PARSER_NEW CAT(NS, _Parser_new)
#define LEXER_NEW CAT(NS, _Lexer_new)
#define INIT_GLOBALS CAT(NS, _init_globals)
#define CORE(x) python_ispa_std_##x

typedef struct { const char *text; bool lazy; ispa_any tree; PARSER *parser; LEXER *lexer; } run_env;

static void parse(void *env_) {
    run_env *e = (run_env *)env_;
    ispa_str text = ispa_str_from(ispa_cur, e->text, strlen(e->text));
    if (e->lazy) {
        e->parser->lazy_lexer_ = e->lexer;
        e->tree = CORE(parser_base_Parser_base_parse_text)((CORE(parser_base_Parser_base) *)e->parser, text);
    } else {
        CORE(lexer_Lexer_base_setInput)((CORE(lexer_Lexer_base) *)e->lexer, text);
        CORE(lexer_Lexer_base_makeTokens)((CORE(lexer_Lexer_base) *)e->lexer);
        e->tree = CORE(parser_base_Parser_base_parse_lexer)((CORE(parser_base_Parser_base) *)e->parser, (CORE(lexer_Lexer_base) *)e->lexer);
    }
}

static void put(ispa_str s) { fwrite(s.data, 1, s.len, stdout); }
/* the library prefixes internal errors with "[issue | origin] "; the C++ exception text does not carry it */
static void put_message(ispa_str s) {
    if (s.len && s.data[0] == '[')
        for (size_t i = 1; i + 1 < s.len; ++i)
            if (s.data[i] == ']' && s.data[i + 1] == ' ') { s.data += i + 2; s.len -= i + 2; break; }
    put(s);
}

int main(int argc, char **argv) {
    ispa_arena arena; ispa_arena_init(&arena, 1 << 20, NULL); ispa_cur = &arena;
    ispa_std_init_globals();
    INIT_GLOBALS();
    run_env e = { argc > 1 ? argv[1] : "", getenv("LAZY") != NULL, ISPA_ANY_NONE, PARSER_NEW(), LEXER_NEW() };
    /* the C++ reference sets the input on the lexer first (lexing errors surface there) */
    if (ispa_attempt(parse, &e) != 0) {
        printf("exception: "); put_message(ispa_failure_message()); printf("\n");
        return 0;
    }
    CORE(ast_printer_ASTPrinter) *pr = CORE(ast_printer_ASTPrinter_new)();
    ispa_print_any(pr, e.tree);
    put(CORE(ast_printer_ASTPrinter_text)(pr));
    CORE(parser_ll_LLParser) *ll = (CORE(parser_ll_LLParser) *)e.parser;
    ispa_vec_python_ispa_std_recovery_DiagnosticP *local = CORE(parser_ll_LLParser_getErrors)(ll);
    for (size_t i = 0; i < local->len; ++i) { printf("ERR "); put(local->data[i]->rule); printf(" | "); put(local->data[i]->message); printf("\n"); }
    ispa_vec_python_ispa_std_recovery_ContextDiagnosticP *ctx = CORE(parser_ll_LLParser_getContextErrors)(ll);
    for (size_t i = 0; i < ctx->len; ++i) { printf("ERR "); put(ctx->data[i]->rule); printf(" | "); put(ctx->data[i]->message); printf("\n"); }
    return 0;
}
