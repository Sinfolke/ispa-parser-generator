export module Rust.Rewriter;

import LangAPI;
import std;

// IR -> IR passes that make the tree expressible in Rust before Rust.Init prints it. Same role as the Python backend's Rewriter.
//
//   1. mergeNamespaces   same-named sibling namespaces become one `mod` (Rust rejects duplicate modules)
//   2. flattenNested     Rust has no nested types / static members inside a struct:
//                        nested classes, enums, aliases and static variables are hoisted next to their parent as
//                        <Parent>_<Name>, and every reference (Parent::Name, bare Name inside Parent, rule::value::X ...) is renamed
// Member qualification and argument ownership are handled during emission; this pass
// never inserts target-specific calls or receiver names into the shared IR.
//
// The input is never modified; shared sub-trees are copied before they are changed.
export namespace Rust::Rewriter {
    auto rewrite(const LangAPI::Declarations &source) -> LangAPI::Declarations;
}
