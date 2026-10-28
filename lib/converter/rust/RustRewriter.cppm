export module Rust.Rewriter;

import LangAPI;
import std;

// IR -> IR passes that make the tree expressible in Rust before Rust.Init prints it. Same role as the Python backend's Rewriter.
//
//   1. mergeNamespaces   same-named sibling namespaces become one `mod` (Rust rejects duplicate modules)
//   2. flattenNested     Rust has no nested types / static members inside a struct:
//                        nested classes, enums, aliases and static variables are hoisted next to their parent as
//                        <Parent>_<Name>, and every reference (Parent::Name, bare Name inside Parent, rule::value::X ...) is renamed
//   3. qualifyMembers    unqualified member use inside methods becomes `self.x` / `this.x` (constructors) / `Self::f` (statics)
//   4. adaptCallSites    call sites of IR-defined functions get &x / &mut x / x.clone() according to the callee's parameter types
//                        (done with pseudo calls that RustCore::convertFunctionCall prints)
//
// The input is never modified; shared sub-trees are copied before they are changed.
export namespace Rust::Rewriter {
    auto rewrite(const LangAPI::Declarations &source) -> LangAPI::Declarations;
}
