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

    // Emission state only. Target syntax must never be written back into LangAPI.
    std::vector<std::string> scope;
    std::unordered_map<std::string, std::vector<const LangAPI::Function*>> functions;
    std::unordered_set<std::string> members, static_members;
    std::unordered_map<std::string, std::vector<const LangAPI::Function*>> methods;   // by bare name, for receiver-less calls
    std::unordered_set<std::string> globals;            // qualified names of namespace-level variables held in a `Lazy`
    auto isConstScalar(const LangAPI::Type &t) -> bool;
    std::vector<std::unordered_set<std::string>> locals;
    std::unordered_set<std::string> borrowed_parameters;
    std::unordered_set<std::string> callable_parameters;   // `impl FnMut` parameters; stored into fields they are boxed
    std::unordered_set<std::string> callable_fields;       // fields holding a Box<dyn FnMut>, called as `(x.field)(...)`
    std::unordered_set<std::string> any_fields;            // fields typed `Any` (PyAny): assigned values are wrapped, None is empty
    std::unordered_set<std::string> shared_classes;        // bare names of reference-semantic (shared-object) classes
    auto isSharedClassType(const LangAPI::Type &t) -> bool;
    std::unordered_set<std::string> shared_locals;         // locals holding a Shared<..> handle (by emitted identifier)
    std::string member_class;                              // bare class name of the receiver of the storage symbol being converted
    std::unordered_map<std::string, std::vector<const LangAPI::Function*>> class_methods;   // "Class::method" (bare class name)
    std::unordered_map<std::string, std::unordered_map<std::string, const LangAPI::Type*>> class_fields;
    std::unordered_map<std::string, const LangAPI::Type*> local_types;   // declared types of locals/parameters of the function being emitted
    auto currentClass() -> std::string;
    auto isSharedObjectInitializer(const LangAPI::Expression &value) -> bool;
    auto typeOfExpression(const LangAPI::Expression &e) -> const LangAPI::Type *;
    auto classOfType(const LangAPI::Type *t) -> std::string;
    std::unordered_map<std::string, const LangAPI::Type*> callable_types;   // Callable parameter name -> its type, for call arguments
    bool owned_callable = false;                           // Callable converts to an owning Box<dyn FnMut> (struct fields)
    std::string receiver;
    bool shared_receiver = false;
    const LangAPI::Type* return_type = nullptr;
    auto indexFunctions(const LangAPI::Declarations& declarations) -> void;
    auto bind(const std::string& name) -> void;

    auto warn(std::string message) -> void;
    auto crateRoot() -> std::string;                    // "crate::<name>" (or "crate" without a name)

    // naming / literals
    auto ident(const std::string &id) -> std::string;
    auto moduleIdent(const std::string &name) -> std::string;
    auto rustStringBody(std::string_view raw) -> std::string;
    auto rustCharLiteral(char value, bool escaped) -> std::string;
    auto variantEnum(const std::vector<std::string> &alternatives) -> std::string;

    // type
    auto convertType(const LangAPI::Type &type) -> std::string;
    auto convertTemplates(const decltype(LangAPI::Type::template_parameters) &template_parameters) -> std::string;
    auto convertTemplates(const stdu::vector<std::variant<std::shared_ptr<LangAPI::Type>, std::shared_ptr<LangAPI::RValue>>> &template_parameters) -> std::string;
    auto convertSymbol(const LangAPI::Symbol &symbol, bool value_context = true) -> std::string;
    auto convertStorageSymbol(const LangAPI::StorageSymbol &symbol, bool place = false) -> std::string;
    // expr == true: expression context (turbofish `::<>`), false: type context (`<>`)
    auto convertIspaLibSymbol(const LangAPI::IspaLibSymbol &symbol, bool expr = false) -> std::string;

    // expression
    auto convertExpression(const LangAPI::Expression &expression) -> std::string;
    auto convertInitializerValue(const LangAPI::Type &type, const LangAPI::Expression &value) -> std::string;
    auto convertInitializer(const LangAPI::Type& type, const LangAPI::Expression& value) -> std::string;
    auto convertExpressionElement(LangAPI::ExpressionElement element) -> const char *;
    auto convertLambda(const LangAPI::Lambda &lambda) -> std::string;
    auto convertFunctionCall(const LangAPI::FunctionCall &call, bool member = false) -> std::string;
    auto convertIspaLibFunctionCall(const LangAPI::IspaLibFunctionCall &call) -> std::string;
    auto convertMakeTuple(const LangAPI::MakeTuple &make_tuple) -> std::string;
    auto convertGetVariant(const LangAPI::GetVariant &get_variant) -> std::string;
    auto convertCheckVariant(const LangAPI::CheckVariant &check_variant) -> std::string;
    auto convertDFADebug(const LangAPI::DFADebug &dfa_debug) -> std::string;
    // rvalue
    auto convertRValue(const LangAPI::RValue &rvalue) -> std::string;
}
