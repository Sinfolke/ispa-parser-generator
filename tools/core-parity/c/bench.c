#define _POSIX_C_SOURCE 200809L
/* Times lexing and parsing of the file argv[1] with the generated C parser (no output of the tree is timed).
 * Built like run.c: -DNS=<module prefix>, "parser.h" from the generated module. Prints {"lex_ms":..,"parse_ms":..}. */
#include "parser.h"
#include <stdlib.h>
#include <time.h>

#define CAT_(a, b) a##b
#define CAT(a, b) CAT_(a, b)
#define PARSER_NEW CAT(NS, _Parser_new)
#define LEXER_NEW CAT(NS, _Lexer_new)
#define INIT_GLOBALS CAT(NS, _init_globals)
#define CORE(x) python_ispa_std_##x

static double now_ms(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec * 1e3 + t.tv_nsec / 1e6; }

int main(int argc, char **argv) {
    if (argc != 2) return 1;
    FILE *f = fopen(argv[1], "rb");
    if (!f) return 1;
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    char *buf = malloc((size_t)n + 1);
    if (fread(buf, 1, (size_t)n, f) != (size_t)n) return 1;
    fclose(f);
    ispa_arena arena; ispa_arena_init(&arena, 1 << 20, NULL); ispa_cur = &arena;
    ispa_std_init_globals();
    INIT_GLOBALS();
    ispa_str text = ispa_str_from(ispa_cur, buf, (size_t)n);
    const double t0 = now_ms();
    CAT(NS, _Lexer) *lexer = LEXER_NEW();
    CORE(lexer_Lexer_base_setInput)((CORE(lexer_Lexer_base) *)lexer, text);
    CORE(lexer_Lexer_base_makeTokens)((CORE(lexer_Lexer_base) *)lexer);
    const double t1 = now_ms();
    CAT(NS, _Parser) *parser = PARSER_NEW();
    CORE(parser_base_Parser_base_parse_lexer)((CORE(parser_base_Parser_base) *)parser, (CORE(lexer_Lexer_base) *)lexer);
    const double t2 = now_ms();
    CORE(parser_ll_LLParser) *ll = (CORE(parser_ll_LLParser) *)parser;
    if (CORE(parser_ll_LLParser_getErrors)(ll)->len || CORE(parser_ll_LLParser_getContextErrors)(ll)->len) return 1;
    printf("{\"lex_ms\":%.3f,\"parse_ms\":%.3f}\n", t1 - t0, t2 - t1);
    return 0;
}
