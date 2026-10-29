// Parses argv[1] with the generated Rust parser and prints the tree (or the failure) like rust/ref_main.cpp does for C++.
use parity::*;
use ispa_std::Shared;

fn main() {
    let text = std::env::args().nth(1).unwrap_or_default();
    let lazy = std::env::var_os("LAZY").is_some();
    let mut parser = Parser::default();
    if !lazy {
        let mut lexer = Lexer::default();
        lexer.set_input(&text);
        if let Err(error) = lexer.make_tokens() {
            println!("exception: {error}");
            return;
        }
        parser.lexer = Shared::new(lexer).upcast();
    }
    let outcome = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
        if lazy { parser.parse_text(&text); } else { parser.parse_tokens(); }
        let mut out = String::new();
        parser.tree.print(&mut out);
        out
    }));
    match outcome {
        Ok(out) => {
            print!("{out}");
            for (rule, message) in parser.diagnostics() { println!("ERR {rule} | {message}"); }
        }
        Err(payload) => {
            let message = payload.downcast_ref::<ispa_std::StdError>().map(|e| e.message.clone()).unwrap_or_else(|| "panic".into());
            println!("exception: {message}");
        }
    }
}
