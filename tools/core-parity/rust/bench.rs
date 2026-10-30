// Read outside the timer and report lexing separately from parsing. No AST printing.
use json_bench::*;
use ispa_std::Shared;
use std::time::Instant;

fn main() {
    let text = std::fs::read_to_string(std::env::args().nth(1).expect("input file")).expect("read input");
    let start = Instant::now();
    let mut lexer = Lexer::default();
    lexer.set_input(&text);
    lexer.make_tokens().expect("lex input");
    let lex_ms = start.elapsed().as_secs_f64() * 1000.0;
    let start = Instant::now();
    let mut parser = Parser::default();
    parser.lexer = Shared::new(lexer).upcast();
    parser.parse_tokens();
    let parse_ms = start.elapsed().as_secs_f64() * 1000.0;
    assert!(parser.diagnostics().is_empty(), "parse diagnostics");
    println!("{{\"lex_ms\":{lex_ms},\"parse_ms\":{parse_ms}}}");
}
