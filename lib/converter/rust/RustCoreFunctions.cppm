export module Rust.CoreFunctions;

import LangAPI;
import Converter.Writer;
import dstd;
import std;

// Rust counterpart of Cpp.CoreFunctions. Namespace is RustCore (not Core) so that both backends can live in one process.
export namespace RustCore {
    constexpr const char* base_pointer = "pos";
    std::vector<std::string> counter = { base_pointer };
    std::string name;                                   // generated parser / root module name
    std::function<std::string(const LangAPI::Statements&)> lambda_body;

    // break / continue targets. Rust needs labels for everything that is not the innermost loop, and an
    // unlabeled `break` inside a labeled block is an error (E0695), so every construct registers a frame.
    struct LoopFrame {
        std::string break_stmt;      // e.g. "break 'w3"
        std::string continue_stmt;   // e.g. "continue 'w3" / "break 'dwb3"; empty for switch (inherits the enclosing loop)
    };
    std::vector<LoopFrame> loop_frames;
    std::size_t label_counter = 0;

    // std::variant<std::monostate, A, B> has no Rust spelling: every distinct alternative list becomes a generated enum.
    struct VariantDef {
        std::string name;
        std::vector<std::string> alternatives;
    };
    std::vector<VariantDef> variants;
    std::vector<std::string> diagnostics;               // written as `// ISPA-RUST-WARNING:` lines at the top of the root file

    // names of the pseudo calls the Rewriter inserts to adapt call sites (see Rust.Rewriter)
    constexpr const char* kRefWrapper = "__ispa_rust_ref";      // &x
    constexpr const char* kMutWrapper = "__ispa_rust_mut";      // &mut x
    constexpr const char* kCloneWrapper = "__ispa_rust_clone";  // x.clone()

    auto warn(std::string message) -> void;
    auto crateRoot() -> std::string;                    // "crate::<name>" (or "crate" without a name)

    // naming / literals
    auto ident(const std::string &id) -> std::string;
    auto rustStringBody(std::string_view raw) -> std::string;
    auto rustCharLiteral(char value, bool escaped) -> std::string;
    auto variantEnum(const std::vector<std::string> &alternatives) -> std::string;

    // type
    auto convertType(const LangAPI::Type &type) -> std::string;
    auto convertTemplates(const decltype(LangAPI::Type::template_parameters) &template_parameters) -> std::string;
    auto convertTemplates(const stdu::vector<std::variant<std::shared_ptr<LangAPI::Type>, std::shared_ptr<LangAPI::RValue>>> &template_parameters) -> std::string;
    auto convertSymbol(const LangAPI::Symbol &symbol) -> std::string;
    auto convertStorageSymbol(const LangAPI::StorageSymbol &symbol) -> std::string;
    // expr == true: expression context (turbofish `::<>`), false: type context (`<>`)
    auto convertIspaLibSymbol(const LangAPI::IspaLibSymbol &symbol, bool expr = false) -> std::string;

    // expression
    auto convertExpression(const LangAPI::Expression &expression) -> std::string;
    auto convertExpressionElement(LangAPI::ExpressionElement element) -> const char *;
    auto convertLambda(const LangAPI::Lambda &lambda) -> std::string;
    auto convertFunctionCall(const LangAPI::FunctionCall &call) -> std::string;
    auto convertIspaLibFunctionCall(const LangAPI::IspaLibFunctionCall &call) -> std::string;
    auto convertMakeTuple(const LangAPI::MakeTuple &make_tuple) -> std::string;
    auto convertGetVariant(const LangAPI::GetVariant &get_variant) -> std::string;
    auto convertCheckVariant(const LangAPI::CheckVariant &check_variant) -> std::string;
    auto convertDFADebug(const LangAPI::DFADebug &dfa_debug) -> std::string;
    // rvalue
    auto convertRValue(const LangAPI::RValue &rvalue) -> std::string;
}
