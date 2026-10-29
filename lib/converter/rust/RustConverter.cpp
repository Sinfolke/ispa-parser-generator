module Rust.Init;

import Rust.CoreFunctions;
import Rust.Rewriter;
import corelib;
import Converter.Writer;
import logging;
import dstd;
import std;

namespace Rust {
    // ------------------------------------------------------------------------
    // Writer conventions (same as the C++ backend): every line goes through writeln("{}", text) so braces in the text
    // are never treated as format placeholders and indentation is always applied.
    // ------------------------------------------------------------------------
    namespace {
        auto emitLine(Converter::Writer &w, const std::string &s) -> void { w.writeln("{}", s); }

        auto joinStrings(const std::vector<std::string> &items, const std::string &sep) -> std::string {
            std::string out;
            for (std::size_t i = 0; i < items.size(); ++i) {
                if (i) out += sep;
                out += items[i];
            }
            return out;
        }
        auto fileSafe(const std::string &name) -> std::string {
            std::string out;
            for (const char c : name) out += (std::isalnum(static_cast<unsigned char>(c)) || c == '_') ? c : '_';
            return out.empty() ? "_" : out;
        }
        auto genericParams(const std::vector<std::string> &names) -> std::string {
            if (names.empty()) return "";
            std::vector<std::string> idents;
            for (const auto &n : names) idents.push_back(RustCore::ident(n));
            return "<" + joinStrings(idents, ", ") + ">";
        }
        // impl-block generics: every lowered Python type parameter is assumed copyable/defaultable/comparable
        auto genericBounds(const std::vector<std::string> &names) -> std::string {
            if (names.empty()) return "";
            std::vector<std::string> idents;
            for (const auto &n : names)
                idents.push_back(RustCore::ident(n) + (n == "K" ? ": ::ispa_std::PyKey" : ": ::ispa_std::PyVal"));
            return "<" + joinStrings(idents, ", ") + ">";
        }
        auto isValueTypeOf(const LangAPI::Type &t, LangAPI::ValueType vt) -> bool {
            return t.isValueType() && t.getValueType() == vt;
        }
        auto singleStringLiteral(const LangAPI::Expression &e) -> bool {
            return e.size() == 1 && e[0].type() == LangAPI::ExpressionValueType::RValue &&
                   e[0].getRValue().type() == LangAPI::RValueType::String;
        }
        constexpr const char *kAllow =
            "#![allow(non_snake_case, non_camel_case_types, non_upper_case_globals, unused_mut, unused_variables,\n"
            "         unused_parens, unused_labels, unused_imports, unreachable_patterns, unreachable_code, dead_code,\n"
            "         unused_braces, invalid_reference_casting, unused_must_use, clippy::all)]\n"
            "use ::ispa_std::pycoll::{PyContains, PyPop};\n";
    }

    // ========================================================================
    // initialisers
    // ========================================================================
    // C++ converts a string literal to whatever the declaration asks for; Rust needs it spelled out.
    auto Construct::initializer(const LangAPI::Type &type, const LangAPI::Expression &value) -> std::string {
        return RustCore::convertInitializer(type, value);
    }

    // ========================================================================
    // statements
    // ========================================================================
    auto Construct::produceIf(const LangAPI::If &if_stmt, Converter::Writer &o) -> void {
        emitLine(o, "if " + RustCore::convertExpression(if_stmt.expr) + " {");
        o.increaseIndentation();
        convertStatements(if_stmt.stmt, o);
        o.decreaseIndentation();
        if (!if_stmt.else_stmt.empty()) {
            emitLine(o, "} else {");
            o.increaseIndentation();
            convertStatements(if_stmt.else_stmt, o);
            o.decreaseIndentation();
        }
        emitLine(o, "}");
    }

    auto Construct::produceWhile(const LangAPI::While &while_stmt, Converter::Writer &o) -> void {
        const auto label = "'w" + std::to_string(++RustCore::label_counter);
        RustCore::loop_frames.push_back({"break " + label, "continue " + label});
        const auto condition = RustCore::convertExpression(while_stmt.expr);
        emitLine(o, label + (condition == "true" ? std::string(": loop {") : ": while " + condition + " {"));
        o.increaseIndentation();
        convertStatements(while_stmt.stmt, o);
        o.decreaseIndentation();
        emitLine(o, "}");
        RustCore::loop_frames.pop_back();
    }

    // do { body } while (cond): `continue` has to run the condition, so the body sits in a labeled block and
    // `continue` becomes `break 'body`.
    auto Construct::produceDoWhile(const LangAPI::DoWhile &do_while_stmt, Converter::Writer &o) -> void {
        const auto n = std::to_string(++RustCore::label_counter);
        const auto outer = "'dw" + n, inner = "'dwb" + n;
        RustCore::loop_frames.push_back({"break " + outer, "break " + inner});
        emitLine(o, outer + ": loop {");
        o.increaseIndentation();
        emitLine(o, inner + ": {");
        o.increaseIndentation();
        convertStatements(do_while_stmt.stmt, o);
        o.decreaseIndentation();
        emitLine(o, "}");
        RustCore::loop_frames.pop_back();                 // the condition is outside the body frame
        emitLine(o, "if !(" + RustCore::convertExpression(do_while_stmt.expr) + ") { break " + outer + "; }");
        o.decreaseIndentation();
        emitLine(o, "}");
    }

    // Every case of the C++ output ends in `break`, so there is no fallthrough to model. A `break` written inside a case
    // targets the labeled block around the match.
    auto Construct::produceSwitch(const LangAPI::Switch &switch_stmt, Converter::Writer &o) -> void {
        const auto label = "'sw" + std::to_string(++RustCore::label_counter);
        RustCore::loop_frames.push_back({"break " + label, ""});
        emitLine(o, label + ": {");
        o.increaseIndentation();
        emitLine(o, "match " + RustCore::convertExpression(switch_stmt.expression) + " {");
        o.increaseIndentation();
        for (const auto &case_stmt : switch_stmt.cases) {
            emitLine(o, RustCore::convertRValue(case_stmt.first) + " => {");
            o.increaseIndentation();
            convertStatements(case_stmt.second, o);
            o.decreaseIndentation();
            emitLine(o, "}");
        }
        emitLine(o, "_ => {}");
        o.decreaseIndentation();
        emitLine(o, "}");
        o.decreaseIndentation();
        emitLine(o, "}");
        RustCore::loop_frames.pop_back();
    }

    auto Construct::produceVariableStatement(const LangAPI::Variable &variable, Converter::Writer &o) -> void {
        auto type = RustCore::convertType(variable.type);
        if (RustCore::isSharedClassType(variable.type)) type = "::ispa_std::Shared<" + type + ">";
        const bool inferred = type == "_";
        std::string line = "let mut " + RustCore::ident(variable.name);
        if (!inferred) line += ": " + type;
        if (!variable.value.empty()) {
            line += " = " + initializer(variable.type, variable.value);
        } else {
            if (inferred) throw Error("Rust Converter: `auto {}` without initialiser", variable.name);
            line += " = Default::default()";       // C++ default-initialises; the type has to implement Default
        }
        emitLine(o, line + ";");
        RustCore::bind(variable.name);
        RustCore::local_types[variable.name] = &variable.type;
        if (RustCore::isSharedClassType(variable.type) || RustCore::isSharedObjectInitializer(variable.value)) RustCore::shared_locals.insert(RustCore::ident(variable.name));
        if (debug_instrumentation_ && !variable.value.empty() &&
            RustCore::convertExpression(variable.value).find("captures.") != std::string::npos)
            emitLine(o, "if ::ispa_std::debug::enabled() { ::ispa_std::debug::lexer_capture(\"" + variable.name + "\", &" +
                        RustCore::ident(variable.name) + "); }");
    }

    // The error path of the generated parsers is `throw`. IR has no try/catch, the catching happens in the runtime
    // (required/optional/...), so a throw becomes a panic with a typed payload that the runtime catches with
    // catch_unwind + downcast. See the notes: this is the biggest design decision of the backend.
    auto Construct::produceThrow(const LangAPI::Throw &throw_stmt, Converter::Writer &o) -> void {
        emitLine(o, "::std::panic::panic_any(" + RustCore::convertExpression(throw_stmt.throw_value) + ");");
    }

    // ISPA_DEBUG_LINE / ISPA_DEBUG_SCOPE markers carry the debugger instrumentation; same format as for C++.
    auto Construct::produceComment(const LangAPI::Comment &comment, Converter::Writer &o) -> void {
        const std::string &text = comment.value;
        const auto quoted = [](const std::string &s) {
            std::ostringstream out;
            out << std::quoted(s);
            return out.str();
        };
        if (text.starts_with("ISPA_DEBUG_LINE ")) {
            std::istringstream input(text.substr(16));
            std::string file, local;
            int row = 0, column = 0;
            input >> std::quoted(file) >> row >> column;
            if (!input || row <= 0) throw std::runtime_error("Malformed ISPA debug line marker");
            std::string snapshot = "::ispa_std::debug::Snapshot::new()";
            std::string phase = "before";
            while (input >> local) {
                if (local == "@after") phase = "after";
                else if (local == "ctx") {
                    snapshot += ".add(\"ctx.rule\", &ctx.rule).add(\"ctx.pos\", &ctx.pos).add(\"ctx.ast\", &ctx.ast)";
                } else {
                    const auto equal = local.find('=');
                    const auto label = equal == std::string::npos ? local : local.substr(0, equal);
                    const auto symbol = equal == std::string::npos ? local : local.substr(equal + 1);
                    snapshot += ".add(\"" + label + "\", &" + symbol + ")";
                }
            }
            emitLine(o, "if ::ispa_std::debug::enabled() { ::ispa_std::debug::line(::ispa_std::debug::Loc::new(" + quoted(file) + ", " +
                        std::to_string(row) + ", " + std::to_string(column) + "), " + snapshot + ", \"" + phase + "\"); }");
            return;
        }
        if (text.starts_with("ISPA_DEBUG_SCOPE ")) {
            std::istringstream input(text.substr(17));
            std::string kind, name, file;
            int row = 0, column = 0;
            input >> kind >> std::quoted(name) >> std::quoted(file) >> row >> column;
            if (!input || (kind != "rule" && kind != "production"))
                throw std::runtime_error("Malformed ISPA debug scope marker");
            // RAII guard: Drop also runs while a panic unwinds, so no try/catch is needed
            emitLine(o, "let ispa_debug_" + kind + " = ::ispa_std::debug::" + (kind == "rule" ? "RuleScope" : "ProductionScope") +
                        "::new(" + quoted(name) + ", ::ispa_std::debug::Loc::new(" + quoted(file) + ", " + std::to_string(row) + ", " +
                        std::to_string(column) + "));");
            return;
        }
        std::size_t start = 0;
        while (true) {
            const auto nl = text.find('\n', start);
            emitLine(o, "// " + text.substr(start, nl == std::string::npos ? std::string::npos : nl - start));
            if (nl == std::string::npos) break;
            start = nl + 1;
        }
    }

    auto Construct::convertStatements(const LangAPI::Statements &statements, Converter::Writer &o) -> void {
        const auto saved_borrowed = RustCore::borrowed_parameters;
        RustCore::locals.emplace_back();
        for (const auto &stmt : statements) convertStatement(stmt, o);
        RustCore::locals.pop_back();
        RustCore::borrowed_parameters = saved_borrowed;
    }

    auto Construct::convertStatement(const LangAPI::Statement &stmt, Converter::Writer &o) -> void {
        if (stmt.isExpression()) {
            const std::string text = RustCore::convertExpression(stmt.getExpression());
            if (!text.empty()) {
                emitLine(o, text + ";");
                if (debug_instrumentation_ && text.find("assign_compatible(") != std::string::npos && text.find("result") != std::string::npos)
                    emitLine(o, "if ::ispa_std::debug::enabled() { ::ispa_std::debug::value(\"result\", &result, ::ispa_std::debug::active_location()); }");
            }
        } else if (stmt.isIf()) produceIf(stmt.getIf(), o);
        else if (stmt.isWhile()) produceWhile(stmt.getWhile(), o);
        else if (stmt.isDoWhile()) produceDoWhile(stmt.getDoWhile(), o);
        else if (stmt.isSwitch()) produceSwitch(stmt.getSwitch(), o);
        else if (stmt.isThrow()) produceThrow(stmt.getThrow(), o);
        else if (stmt.isComment()) produceComment(stmt.getComment(), o);
        else if (stmt.isVariable()) produceVariableStatement(stmt.getVariable(), o);
        else throw std::runtime_error("Rust Converter: unsupported statement kind");
    }

    // ========================================================================
    // functions
    // ========================================================================
    // free function / method / constructor:
    //   free          pub fn f<T: Marker>(a: A) -> R { ... }
    //   method        pub fn f(&mut self, ...) / (&self, ...) / (no self when static)
    //   constructor   pub fn new(...) -> Self { let mut this = Self::default(); ...; this }
    // Constrained templates (every type parameter has a fixed type list, C++ `enable_if<is_same<..> || ...>`) get a
    // marker trait implemented for exactly those types.
    // ISPA_RUST_SURVEY=1 records each function that cannot be emitted and keeps going, to inventory every blocker at once.
    // A callable parameter that the body moves into another object (a constructor argument) has to outlive the call:
    // the signature is only known to need `+ 'static` once the body has been seen, so such a function is emitted again.
    auto Construct::emitFunction(const LangAPI::Function &func, Converter::Writer &w, Converter::Writer &aux, FnKind kind,
                                 const std::string &scope, const std::vector<std::string> &bases) -> void {
        const auto w0 = w;
        const auto aux0 = aux;
        const auto counts = context.fn_counts;
        const auto requested_name = RustCore::emit_name;
        RustCore::callable_moved = false;
        emitFunctionSurveyed(func, w, aux, kind, scope, bases);
        if (RustCore::callable_moved && !RustCore::force_static_callable) {
            w = w0;
            aux = aux0;
            context.fn_counts = counts;
            RustCore::emit_name = requested_name;
            RustCore::force_static_callable = true;
            emitFunctionSurveyed(func, w, aux, kind, scope, bases);
            RustCore::force_static_callable = false;
        }
    }

    auto Construct::emitFunctionSurveyed(const LangAPI::Function &func, Converter::Writer &w, Converter::Writer &aux, FnKind kind,
                                 const std::string &scope, const std::vector<std::string> &bases) -> void {
        if (!std::getenv("ISPA_RUST_SURVEY")) return emitFunctionChecked(func, w, aux, kind, scope, bases);
        const auto saved_receiver = RustCore::receiver;
        const auto saved_return_type = RustCore::return_type;
        const auto saved_locals = RustCore::locals;
        const auto saved_borrowed = RustCore::borrowed_parameters;
        const auto saved_counter = RustCore::counter;
        const auto saved_frames = RustCore::loop_frames;
        const auto backup = w;
        const auto aux_backup = aux;
        try {
            emitFunctionChecked(func, w, aux, kind, scope, bases);
        } catch (const std::exception &e) {
            w = backup;
            aux = aux_backup;
            std::cerr << "SURVEY " << scope << "::" << func.name << ": " << e.what() << "\n";
            RustCore::receiver = saved_receiver;
            RustCore::return_type = saved_return_type;
            RustCore::locals = saved_locals;
            RustCore::borrowed_parameters = saved_borrowed;
            RustCore::counter = saved_counter;
            RustCore::loop_frames = saved_frames;
        }
    }

    auto Construct::emitFunctionChecked(const LangAPI::Function &func, Converter::Writer &w, Converter::Writer &aux, FnKind kind,
                                 const std::string &scope, const std::vector<std::string> &bases) -> void {
        std::string name = kind == FnKind::Constructor ? "new" : RustCore::ident(func.name);
        if (!RustCore::emit_name.empty()) { name = RustCore::emit_name; RustCore::emit_name.clear(); }
        const auto key = scope + "::" + name;
        if (++context.fn_counts[key] > 1)
            throw Error("Rust target: overloaded function `{}` needs a resolved callee identity in IR", key);
        if (func.override && !RustCore::overrides.contains(&func))
            throw Error("Rust target: virtual method `{}` overrides a method the IR does not declare in a base class", func.name);

        // generics + marker traits
        std::vector<std::string> generics;
        for (const auto &tp : func.template_parameters) {
            const auto param = RustCore::ident(tp.first);
            if (tp.second.empty()) {
                RustCore::warn("template parameter `" + tp.first + "` of `" + func.name +
                               "` has no fixed type list: the Rust generic is unbounded and the body will not type-check without bounds");
                generics.push_back(param + (tp.first == "K" ? ": ::ispa_std::PyKey" : ": ::ispa_std::PyVal"));
                continue;
            }
            const auto marker = "__Allowed_" + fileSafe(scope) + "_" + fileSafe(func.name) + "_" + fileSafe(tp.first) + "_" +
                                std::to_string(++context.marker_counter);
            // a generic cursor type brings what the combinators need of it
            bool cursor = false;
            for (const auto &allowed : tp.second) cursor = cursor || RustCore::convertType(allowed).find("::LexerTypes>::") != std::string::npos;
            emitLine(aux, "pub trait " + marker + (cursor ? ": ::ispa_std::ParserCursor" : "") + " {}");
            if (cursor)   // every parser cursor is allowed, so rules can call one another with their own generic cursor
                emitLine(aux, "impl<__C: ::ispa_std::ParserCursor> " + marker + " for __C {}");
            else
                for (const auto &allowed : tp.second) emitLine(aux, "impl " + marker + " for " + RustCore::convertType(allowed) + " {}");
            generics.push_back(param + ": " + marker);
        }

        const auto saved_receiver = RustCore::receiver;
        const auto saved_return_type = RustCore::return_type;
        const auto saved_locals = RustCore::locals;
        const auto saved_borrowed = RustCore::borrowed_parameters;
        RustCore::borrowed_parameters.clear();
        RustCore::callable_parameters.clear();
        RustCore::const_borrowed_parameters.clear();
        RustCore::callable_types.clear();
        RustCore::reference_parameters.clear();
        RustCore::local_types.clear();
        RustCore::locals.clear();
        RustCore::locals.emplace_back();
        RustCore::return_type = &func.type;
        if (kind == FnKind::Constructor) RustCore::receiver = "this";
        else if (kind != FnKind::Method || func.is_static) RustCore::receiver.clear();
        else RustCore::receiver = "self";
        for (const auto& parameter : func.parameters) {
            RustCore::bind(parameter.second);
            RustCore::local_types[parameter.second] = &parameter.first;
            if (isValueTypeOf(parameter.first, LangAPI::ValueType::BorrowSemantic)) {
                RustCore::borrowed_parameters.insert(parameter.second);
                if (parameter.first.template_parameters.size() == 1 && std::holds_alternative<LangAPI::Type>(parameter.first.template_parameters[0]) &&
                    isValueTypeOf(std::get<LangAPI::Type>(parameter.first.template_parameters[0]), LangAPI::ValueType::ImmutableSemantic))
                    RustCore::const_borrowed_parameters.insert(parameter.second);
            }
            if (parameter.first.isIspaLibSymbol() && parameter.first.getIspaLibSymbol().Reference)
                RustCore::reference_parameters.insert(parameter.second);
            if (isValueTypeOf(parameter.first, LangAPI::ValueType::Callable))
                { RustCore::callable_parameters.insert(parameter.second); RustCore::callable_types[parameter.second] = &parameter.first; }
        }

        // signature
        std::vector<std::string> params;
        if (kind == FnKind::Method && !func.is_static) params.push_back(func.is_const ? "&self" : "&mut self");
        RustCore::static_callable = RustCore::force_static_callable || kind == FnKind::Constructor || func.name == "__init__";   // stored in a field
        for (const auto &p : func.parameters) {
            const auto type = RustCore::convertType(p.first);
            params.push_back((type.starts_with("&") ? "" : "mut ") + RustCore::ident(p.second) + ": " + type);
        }
        RustCore::static_callable = false;
        std::string ret;
        if (kind == FnKind::Constructor) ret = " -> Self";
        else if (!(func.type.isValueType() && func.type.getValueType() == LangAPI::ValueType::Void)) {
            const auto type = RustCore::convertType(func.type);
            if (type == "_") throw Error("Rust Converter: `{}` has an `auto` return type; Rust cannot infer it (add a Rewriter pass or type the IR)", func.name);
            ret = " -> " + type;
        }
        emitLine(w, "pub fn " + name + (generics.empty() ? "" : "<" + joinStrings(generics, ", ") + ">") + "(" +
                    joinStrings(params, ", ") + ")" + ret + " {");
        w.increaseIndentation();
        if (kind == FnKind::Method && !func.is_static) emitLine(w, "let __p = self as *const Self as *mut Self;");   // for closures that reach the object

        const auto saved_counter = RustCore::counter;
        const auto saved_frames = RustCore::loop_frames;
        RustCore::counter = {RustCore::base_pointer};
        RustCore::loop_frames.clear();

        if (kind == FnKind::Constructor) { emitLine(w, "let mut this = Self::default();"); emitLine(w, "let __p = (&mut this) as *mut Self;"); }
        if (func.statements.empty() && kind != FnKind::Constructor && !ret.empty()) {
            throw Error("Rust target: function `{}` has no implementation", func.name);
        }
        for (const auto &stmt : func.statements) {
            // `Base{args}` initialiser-list entries of a constructor assign the embedded base
            if (kind == FnKind::Constructor && stmt.isExpression() && stmt.getExpression().size() == 1 &&
                stmt.getExpression()[0].type() == LangAPI::ExpressionValueType::RValue &&
                stmt.getExpression()[0].getRValue().type() == LangAPI::RValueType::Inheritance) {
                const auto &inheritance = stmt.getExpression()[0].getRValue().getInheritance();
                std::string base_name;
                if (std::holds_alternative<LangAPI::Symbol>(inheritance.name)) base_name = RustCore::convertSymbol(std::get<LangAPI::Symbol>(inheritance.name));
                else if (std::holds_alternative<LangAPI::IspaLibSymbol>(inheritance.name)) base_name = RustCore::convertIspaLibSymbol(std::get<LangAPI::IspaLibSymbol>(inheritance.name), false);
                else base_name = RustCore::convertType(*std::get<std::shared_ptr<LangAPI::Type>>(inheritance.name));
                const auto it = std::find(bases.begin(), bases.end(), base_name);
                if (it == bases.end()) throw Error("Rust Converter: constructor initialises `{}` which is not a base class", base_name);
                emitLine(w, "this.base_" + std::to_string(it - bases.begin()) + " = " + RustCore::convertRValue(stmt.getExpression()[0].getRValue()) + ";");
                continue;
            }
            convertStatement(stmt, w);
        }
        if (kind == FnKind::Constructor) emitLine(w, "this");

        RustCore::counter = saved_counter;
        RustCore::loop_frames = saved_frames;
        RustCore::receiver = saved_receiver;
        RustCore::return_type = saved_return_type;
        RustCore::locals = saved_locals;
        RustCore::borrowed_parameters = saved_borrowed;
        w.decreaseIndentation();
        emitLine(w, "}");
    }

    // C++ `ostream& operator<<(ostream& os, <params>)`. The IR passes the object as an ordinary parameter, so this is a
    // free function; the stream becomes `&mut dyn fmt::Write`.
    auto Construct::emitOutputFunction(const LangAPI::Function &func, Converter::Writer &w) -> void {
        std::vector<std::string> params{"os: &'a mut dyn ::std::fmt::Write"};
        for (const auto &p : func.parameters) params.push_back(RustCore::ident(p.second) + ": " + RustCore::convertType(p.first));
        const auto suffix = func.parameters.empty() ? std::string("value") : fileSafe(RustCore::convertType(func.parameters.front().first));
        emitLine(w, "pub fn operator_shl_" + suffix + "<'a>(" + joinStrings(params, ", ") + ") -> &'a mut dyn ::std::fmt::Write {");
        w.increaseIndentation();
        RustCore::loop_frames.clear();
        RustCore::reference_parameters = {"os"};   // the stream is a mutable reference, lent on to the printers
        convertStatements(func.statements, w);
        RustCore::reference_parameters.clear();
        w.decreaseIndentation();
        emitLine(w, "}");
    }

    // ========================================================================
    // classes
    // ========================================================================
    // struct + impl. Rust has no inheritance, nested types or in-class initialisers, so:
    //   bases            -> `base_N` fields, Deref/DerefMut to the first base
    //   field initialisers -> a hand-written Default (derive(Default) when there are none)
    //   constructor      -> `new`, built on Default
    //   nested types / statics were hoisted by Rust.Rewriter; anything that still shows up is emitted as a sibling
    auto Construct::emitClass(const LangAPI::Class &cls, Converter::Writer &w) -> void {
        for (const auto& [visibility, base] : cls.inherit_members) {
            if (const auto* library = std::get_if<LangAPI::IspaLibSymbol>(&base);
                library && (library->exports == LangAPI::StdlibExports::Lexer ||
                            library->exports == LangAPI::StdlibExports::Parser ||
                            library->exports == LangAPI::StdlibExports::LRParser ||
                            library->exports == LangAPI::StdlibExports::LRGeneralizedParser)) {
                if (!std::getenv("ISPA_RUST_EXPERIMENTAL"))   // set to develop the Rust parser backend
                    throw Error("Rust target: parser generation is incomplete: shared lowering still uses C++ container/iterator operations and implicit inherited runtime calls. IR needs semantic token-stream operations and explicit runtime interface contracts; refusing to emit an invalid Rust parser");
            }
        }
        const std::string name = RustCore::convertSymbol(cls.name, false);
        const std::string generics = genericParams(cls.template_parameters);
        const std::string impl_generics = genericBounds(cls.template_parameters);
        const bool templated = !cls.template_parameters.empty();
        const auto saved_shared_receiver = RustCore::shared_receiver;
        RustCore::shared_receiver = cls.shared_object;
        const auto saved_members = RustCore::members;
        const auto saved_static_members = RustCore::static_members;
        RustCore::members.clear();
        RustCore::static_members.clear();
        RustCore::scope.push_back(name);
        for (const auto& [member, visibility] : cls.data) {
            if (member->isVariable()) {
                const auto& variable = member->getVariable();
                (variable.is_static ? RustCore::static_members : RustCore::members).insert(variable.name);
            } else if (member->isFunction()) {
                const auto& function = member->getFunction();
                (function.is_static ? RustCore::static_members : RustCore::members).insert(function.name);
            }
        }
        if (cls.to_str_fun) RustCore::members.insert(cls.to_str_fun->name);
        // data members the runtime base class provides (reached unqualified, like `this->tokens`)
        if (!cls.inherit_members.empty())
            if (const auto* library = std::get_if<LangAPI::IspaLibSymbol>(&cls.inherit_members.front().second)) {
                if (library->exports == LangAPI::StdlibExports::Lexer)
                    RustCore::members.insert({"tokens", "errors", "line", "external_boundary", "bound"});
                else if (library->exports == LangAPI::StdlibExports::Parser)
                    RustCore::members.insert({"lexer", "has_lexer", "text", "has_text", "tree"});
            }


        std::vector<std::string> bases;
        for (const auto &inherit : cls.inherit_members) {
            if (std::holds_alternative<LangAPI::Symbol>(inherit.second)) bases.push_back(RustCore::convertSymbol(std::get<LangAPI::Symbol>(inherit.second)));
            else bases.push_back(RustCore::convertIspaLibSymbol(std::get<LangAPI::IspaLibSymbol>(inherit.second), false));
        }

        struct Field { std::string name, type; const LangAPI::Type* ir_type; const LangAPI::Expression *init; bool is_public; };
        std::vector<Field> fields;
        std::vector<std::string> methods;
        std::vector<const LangAPI::Declaration *> siblings;
        for (const auto &[declaration, visibility] : cls.data) {
            if (declaration->isVariable() && !declaration->getVariable().is_static) {
                const auto &v = declaration->getVariable();
                RustCore::owned_callable = true;
                auto field_type = RustCore::convertType(v.type);
                RustCore::owned_callable = false;
                fields.push_back({RustCore::ident(v.name), std::move(field_type), &v.type, &v.value, visibility == LangAPI::Visibility::Public});
            } else if (declaration->isFunction()) {
                if (visibility == LangAPI::Visibility::Public) methods.push_back(declaration->getFunction().name);
            } else {
                siblings.push_back(declaration.get());
            }
        }
        // dynamic dispatch: a method some subclass overrides becomes a function-pointer slot in this class; overriding
        // methods register a thunk in the slot when the object is built
        std::vector<const LangAPI::Function *> slots, overriding;
        for (const auto &[declaration, visibility] : cls.data) {
            if (!declaration->isFunction()) continue;
            const auto &fn = declaration->getFunction();
            if (RustCore::slot_functions.contains(&fn)) slots.push_back(&fn);
            if (RustCore::overrides.contains(&fn)) overriding.push_back(&fn);
        }
        if ((!slots.empty() || !overriding.empty()) && (templated || bases.size() > 1))
            throw Error("Rust target: virtual dispatch in generic or multiply-inheriting class `{}` is not supported", name);
        const auto fn_pointer_type = [&](const LangAPI::Function &fn, const std::string &receiver_type) {
            std::vector<std::string> types{std::string(fn.is_const ? "&" : "&mut ") + receiver_type};
            for (const auto &p : fn.parameters) {
                auto type = RustCore::convertType(p.first);
                if (type.starts_with("impl "))
                    throw Error("Rust target: virtual method `{}` takes a callable parameter", fn.name);
                types.push_back(type);
            }
            return "fn(" + joinStrings(types, ", ") + ")" +
                   (fn.type.isValueType() && fn.type.getValueType() == LangAPI::ValueType::Void ? std::string() : " -> " + RustCore::convertType(fn.type));
        };
        for (const auto &f : fields) if (f.type == "::ispa_std::PyAny") RustCore::any_fields.insert(f.name);
        const auto is_callable = [](const Field &f) { return f.type.starts_with("Box<dyn FnMut"); };
        for (const auto &f : fields) if (is_callable(f)) RustCore::callable_fields.insert(f.name);
        const bool has_callable = std::any_of(fields.begin(), fields.end(), is_callable);   // Box<dyn FnMut>: neither Clone nor Default
        const bool has_defaults = has_callable || !slots.empty() || !overriding.empty() ||
                                  std::any_of(fields.begin(), fields.end(), [](const Field &f) { return !f.init->empty(); });

        // --- struct ---
        if (cls.shared_object || !overriding.empty()) emitLine(w, "#[repr(C)]");   // the base part is the first field: handles to a base are plain pointers
        if (has_callable) emitLine(w, "// a Callable field makes this class neither Clone nor derivable Default");
        else emitLine(w, std::string("#[derive(Clone") + (has_defaults ? "" : ", Default") + ")]");
        emitLine(w, "pub struct " + name + generics + " {");
        w.increaseIndentation();
        // the base part comes first: a pointer to it is a pointer to the derived object (dispatch thunks rely on it)
        for (std::size_t i = 0; i < bases.size(); ++i) emitLine(w, "pub base_" + std::to_string(i) + ": " + bases[i] + ",");
        if (cls.shared_object) emitLine(w, "pub __ispa_owner: ::ispa_std::WeakShared<Self>,");
        for (const auto *slot : slots) emitLine(w, "pub __vt_" + RustCore::ident(slot->name) + ": " + fn_pointer_type(*slot, name) + ",");
        for (const auto &f : fields) emitLine(w, "pub " + f.name + ": " + f.type + ",");
        if (templated) {
            std::vector<std::string> names;
            for (const auto &n : cls.template_parameters) names.push_back(RustCore::ident(n));
            emitLine(w, "pub __phantom: ::std::marker::PhantomData<(" + joinStrings(names, ", ") + (names.size() == 1 ? ",)>," : ")>,"));
        }
        w.decreaseIndentation();
        emitLine(w, "}");

        if (has_defaults) {
            emitLine(w, "impl" + impl_generics + " Default for " + name + generics + " {");
            w.increaseIndentation();
            emitLine(w, "fn default() -> Self {");
            w.increaseIndentation();
            emitLine(w, "Self {");
            w.increaseIndentation();
            if (cls.shared_object) emitLine(w, "__ispa_owner: Default::default(),");
            for (std::size_t i = 0; i < bases.size(); ++i) {
                if (overriding.empty()) { emitLine(w, "base_" + std::to_string(i) + ": Default::default(),"); continue; }
                std::string registrations;
                for (const auto *fn : overriding) {
                    std::string path = "__base";
                    for (std::size_t hop = 1; hop < RustCore::overrides.at(fn).depth; ++hop) path += ".base_0";
                    registrations += path + ".__vt_" + RustCore::ident(fn->name) + " = " + name + "::__thunk_" + RustCore::ident(fn->name) + "; ";
                }
                emitLine(w, "base_" + std::to_string(i) + ": { let mut __base: " + bases[i] + " = Default::default(); " + registrations + "__base },");
            }
            for (const auto *slot : slots)
                emitLine(w, "__vt_" + RustCore::ident(slot->name) + ": " + name + "::__base_" + RustCore::ident(slot->name) + ",");
            for (const auto &f : fields) {
                std::string value = f.init->empty() ? std::string("Default::default()") : initializer(*f.ir_type, *f.init);
                if (is_callable(f) && f.init->empty()) {
                    const auto arity = f.ir_type->template_parameters.size() > 1 ? f.ir_type->template_parameters.size() - 1 : 0;
                    std::vector<std::string> ignored(arity, "_");
                    value = "Box::new(|" + joinStrings(ignored, ", ") + "| ::ispa_std::fail(9, \"callable field was never set\"))";
                }
                emitLine(w, f.name + ": " + value + ",");
            }
            if (templated) emitLine(w, "__phantom: Default::default(),");
            w.decreaseIndentation();
            emitLine(w, "}");
            w.decreaseIndentation();
            emitLine(w, "}");
            w.decreaseIndentation();
            emitLine(w, "}");
        }
        if (!bases.empty()) {
            if (bases.size() > 1) throw Error("Rust target: multiple inheritance on `{}` needs resolved base-member identities in IR", name);
            emitLine(w, "impl" + impl_generics + " ::std::ops::Deref for " + name + generics + " { type Target = " + bases[0] + "; fn deref(&self) -> &Self::Target { &self.base_0 } }");
            emitLine(w, "impl" + impl_generics + " ::std::ops::DerefMut for " + name + generics + " { fn deref_mut(&mut self) -> &mut Self::Target { &mut self.base_0 } }");
            {
                // Shared<Self> converts to Shared<Base> and to everything Base converts to
                std::string generic_list = impl_generics.empty() ? "<__X: 'static>" : impl_generics.substr(0, impl_generics.size() - 1) + ", __X: 'static>";
                emitLine(w, "impl" + generic_list + " ::ispa_std::Upcast<__X> for " + name + generics + " where " + bases[0] +
                            ": ::ispa_std::Upcast<__X> { unsafe fn up_ptr(this: *mut Self) -> *mut __X { <" + bases[0] +
                            " as ::ispa_std::Upcast<__X>>::up_ptr(::std::ptr::addr_of_mut!((*this).base_0)) } }");
            }
        }

        // a lexer exposes the iterator types of its runtime base to the parser generics
        if (!cls.inherit_members.empty() && !templated)
            if (const auto* library = std::get_if<LangAPI::IspaLibSymbol>(&cls.inherit_members.front().second);
                library && library->exports == LangAPI::StdlibExports::Lexer) {
                // spelled out: projections through the base would leave the marker-trait impls looking overlapping
                const auto open = bases[0].find('<');
                const auto token = open == std::string::npos ? std::string("()") : bases[0].substr(open + 1, bases[0].size() - open - 2);
                emitLine(w, "impl ::ispa_std::LexerTypes for " + name + " { type Token = " + token + "; type Iterator = ::ispa_std::TokenIterator<" +
                            token + ">; type LazyIterator = ::ispa_std::LazyIterator<" + token + ">; }");
            }

        // --- impl ---
        Converter::Writer aux;
        const auto saved_modules = context.module_path;
        std::vector<std::string> scope_parts = context.module_path;
        scope_parts.push_back(name);
        const auto scope = joinStrings(scope_parts, "::");
        emitLine(w, "impl" + impl_generics + " " + name + generics + " {");
        w.increaseIndentation();
        if (cls.shared_object) {
            emitLine(w, "pub fn shared_default() -> ::ispa_std::Shared<Self> {");
            w.increaseIndentation();
            emitLine(w, "::ispa_std::Shared::new_with(|owner| { let mut value = Self::default(); value.__bind_owner(owner); value })");
            w.decreaseIndentation();
            emitLine(w, "}");
            // every class of the chain learns the handle of the full object, as itself
            emitLine(w, "pub fn __bind_owner(&mut self, owner: &::ispa_std::WeakShared<Self>) {");
            w.increaseIndentation();
            emitLine(w, "self.__ispa_owner = owner.clone();");
            if (!bases.empty()) emitLine(w, "self.base_0.__bind_owner(&owner.upcast());");
            w.decreaseIndentation();
            emitLine(w, "}");
        }
        // `construct[T](...)` runs `__init__` on every shared class; a root class without one gets the empty initialiser
        if (cls.shared_object && cls.inherit_members.empty() && !cls.constructor &&
            std::none_of(cls.data.begin(), cls.data.end(), [](const auto &entry) {
                return entry.first->isFunction() && entry.first->getFunction().name == "__init__"; }))
            emitLine(w, "pub fn __init__(&mut self) {}");
        if (cls.constructor) emitFunction(*cls.constructor, w, aux, FnKind::Constructor, scope, bases);
        else if (!cls.shared_object && bases.empty() && !templated &&
                 std::none_of(cls.data.begin(), cls.data.end(), [](const auto &entry) {
                     return entry.first->isFunction() && entry.first->getFunction().name == "new"; })) {
            // an aggregate: `T{a, b}` builds it from its fields in declaration order
            std::vector<std::string> params;
            for (const auto &f : fields) params.push_back(f.name + ": " + f.type);
            emitLine(w, "pub fn new(" + joinStrings(params, ", ") + ") -> Self {");
            w.increaseIndentation();
            emitLine(w, std::string(fields.empty() ? "Self::default()" : "let mut __value = Self::default();"));
            for (const auto &f : fields) emitLine(w, "__value." + f.name + " = " + f.name + ";");
            if (!fields.empty()) emitLine(w, "__value");
            w.decreaseIndentation();
            emitLine(w, "}");
        }
        for (const auto &[declaration, visibility] : cls.data) {
            if (!declaration->isFunction()) continue;
            const auto &fn = declaration->getFunction();
            if (RustCore::slot_functions.contains(&fn)) {
                // the body keeps running as the default of the slot; the method itself dispatches
                RustCore::emit_name = "__base_" + RustCore::ident(fn.name);
                emitFunction(fn, w, aux, FnKind::Method, scope, bases);
                std::vector<std::string> params, args;
                for (const auto &p : fn.parameters) {
                    params.push_back(RustCore::ident(p.second) + ": " + RustCore::convertType(p.first));
                    args.push_back(RustCore::ident(p.second));
                }
                const bool is_void = fn.type.isValueType() && fn.type.getValueType() == LangAPI::ValueType::Void;
                emitLine(w, "pub fn " + RustCore::ident(fn.name) + "(" + (fn.is_const ? "&self" : "&mut self") +
                            (params.empty() ? "" : ", " + joinStrings(params, ", ")) + ")" +
                            (is_void ? "" : " -> " + RustCore::convertType(fn.type)) + " { (self.__vt_" + RustCore::ident(fn.name) + ")(self" +
                            (args.empty() ? "" : ", " + joinStrings(args, ", ")) + ") }");
                continue;
            }
            if (!RustCore::overrides.contains(&fn)) {
                emitFunction(fn, w, aux, FnKind::Method, scope, bases);
                continue;
            }
            // an overriding method: the body is the registered implementation, the method itself dispatches through the
            // slot of the class that introduced it (so a call through any level of the hierarchy reaches the most derived)
            const auto &info = RustCore::overrides.at(&fn);
            RustCore::emit_name = "__impl_" + RustCore::ident(fn.name);
            emitFunction(fn, w, aux, FnKind::Method, scope, bases);
            std::vector<std::string> params, args;
            for (const auto &p : fn.parameters) {
                params.push_back(RustCore::ident(p.second) + ": " + RustCore::convertType(p.first));
                args.push_back(RustCore::ident(p.second));
            }
            const bool is_void = fn.type.isValueType() && fn.type.getValueType() == LangAPI::ValueType::Void;
            // the type of the class that owns the slot, spelled the way this class spells its own base
            std::string introducer = bases[0];
            if (!info.library) {
                const LangAPI::Class *walker = &cls;
                for (std::size_t hop = 0; hop < info.depth; ++hop) {
                    const auto &symbol = std::get<LangAPI::Symbol>(walker->inherit_members.front().second);
                    introducer = RustCore::convertSymbol(symbol);
                    walker = RustCore::class_decls.at(std::get<std::string>(symbol.path.back()));
                }
            }
            std::string path = "self.base_0";
            for (std::size_t hop = 1; hop < info.depth; ++hop) path += ".base_0";
            const auto ret = std::string(is_void ? "" : " -> " + RustCore::convertType(fn.type));
            emitLine(w, "pub fn " + RustCore::ident(fn.name) + "(" + (fn.is_const ? "&self" : "&mut self") +
                        (params.empty() ? "" : ", " + joinStrings(params, ", ")) + ")" + ret + " { (" + path + ".__vt_" +
                        RustCore::ident(fn.name) + ")(" + (fn.is_const ? "&" : "&mut ") + path +
                        (args.empty() ? "" : ", " + joinStrings(args, ", ")) + ") }");
            emitLine(w, "pub fn __thunk_" + RustCore::ident(fn.name) + "(base: " + (fn.is_const ? "&" : "&mut ") + introducer +
                        (params.empty() ? "" : ", " + joinStrings(params, ", ")) + ")" + ret + " { let this = unsafe { &" +
                        (fn.is_const ? "" : "mut ") + "*(base as *const " + introducer + " as *" + (fn.is_const ? "const " : "mut ") +
                        name + ") }; this.__impl_" + RustCore::ident(fn.name) + "(" + joinStrings(args, ", ") + ") }");
        }
        if (cls.to_str_fun) emitFunction(*cls.to_str_fun, w, aux, FnKind::Method, scope, bases);

        // reflection used by printers: members() / members_mut() / members_str(), methods_str()
        std::vector<std::string> refs, mut_refs, types, mut_types, names, method_names;
        for (const auto &f : fields) {
            if (!f.is_public) continue;
            refs.push_back("&self." + f.name);
            mut_refs.push_back("&mut self." + f.name);
            types.push_back("&" + f.type);
            mut_types.push_back("&mut " + f.type);
            names.push_back("\"" + f.name + "\"");
        }
        for (const auto &m : methods) method_names.push_back("\"" + m + "\"");
        const auto tuple = [](const std::vector<std::string> &items) {
            return "(" + joinStrings(items, ", ") + (items.size() == 1 ? ",)" : ")");
        };
        emitLine(w, "pub fn members(&self) -> " + tuple(types) + " { " + tuple(refs) + " }");
        emitLine(w, "pub fn members_mut(&mut self) -> " + tuple(mut_types) + " { " + tuple(mut_refs) + " }");
        emitLine(w, "pub fn members_str() -> [&'static str; " + std::to_string(names.size()) + "] { [" + joinStrings(names, ", ") + "] }");
        emitLine(w, "pub fn methods_str() -> [&'static str; " + std::to_string(method_names.size()) + "] { [" + joinStrings(method_names, ", ") + "] }");
        w.decreaseIndentation();
        emitLine(w, "}");

        if (!templated) {
            const bool prints = std::any_of(cls.data.begin(), cls.data.end(), [](const auto &entry) {
                return entry.first->isFunction() && entry.first->getFunction().name == "write_to_output"; });
            if (prints)
                emitLine(w, "impl ::ispa_std::PrintValue for " + name + " { fn structured(&self) -> bool { true } "
                            "fn print_subtree(&self, printer: &mut ::ispa_std::ASTPrinter) { self.write_to_output(printer) } }");
            if (cls.to_str_fun)
                emitLine(w, "impl ::ispa_std::ToText for " + name + " { fn to_text(&self) -> String { self.to_string() } }");
        }
        if (debug_instrumentation_ && !templated) {
            const auto in_types = std::find(context.module_path.begin(), context.module_path.end(), "Types") != context.module_path.end() ||
                                  std::find(context.module_path.begin(), context.module_path.end(), "FlatTypes") != context.module_path.end();
            if (!fields.empty() || in_types) {
                std::vector<std::string> path = context.module_path;
                path.push_back(name);
                emitLine(w, "impl ::ispa_std::debug::DebugFields for " + name + " {");
                w.increaseIndentation();
                emitLine(w, "fn ispa_debug_type(&self) -> &'static str { \"" + joinStrings(path, ".") + "\" }");
                emitLine(w, "fn ispa_debug_fields(&self, field: &mut dyn FnMut(&str, &dyn ::ispa_std::debug::DebugValue)) {");
                w.increaseIndentation();
                for (const auto &f : fields) emitLine(w, "field(\"" + f.name + "\", &self." + f.name + ");");
                w.decreaseIndentation();
                emitLine(w, "}");
                w.decreaseIndentation();
                emitLine(w, "}");
            }
        }
        if (cls.output_fun && !templated) emitOutputFunction(*cls.output_fun, w);

        const auto aux_text = std::string(aux.get());
        if (!aux_text.empty()) w.dwrite("{}", aux_text);

        for (const auto *sibling : siblings) {
            throw Error("Rust target: nested declaration in `{}` was not resolved by structural lowering", name);
        }
        context.module_path = saved_modules;
        RustCore::scope.pop_back();
        RustCore::shared_receiver = saved_shared_receiver;
        RustCore::members = saved_members;
        RustCore::static_members = saved_static_members;
    }

    // ========================================================================
    // enums / aliases / variables
    // ========================================================================
    auto Construct::emitEnum(const LangAPI::Enum &enum_, Converter::Writer &w) -> void {
        emitLine(w, "#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash, PartialOrd, Ord, Default)]");
        emitLine(w, "pub enum " + RustCore::ident(enum_.name) + " {");
        w.increaseIndentation();
        bool first = true;
        for (const auto &n : enum_.value) {
            emitLine(w, std::string(first ? "#[default] " : "") + RustCore::ident(n) + ",");
            first = false;
        }
        w.decreaseIndentation();
        emitLine(w, "}");
    }

    auto Construct::emitTypeAlias(const LangAPI::TypeAlias &alias, Converter::Writer &w) -> void {
        // The flattening pass may leave a compatibility alias that points back
        // to the item in the very same module. Emitting it would duplicate the
        // item (and create a recursive alias in Rust).
        const auto self_path = "crate::" + joinStrings(context.module_path, "::") + "::" + RustCore::ident(alias.name);
        if (RustCore::convertType(alias.type) == self_path) return;
        emitLine(w, "pub type " + RustCore::ident(alias.name) + genericParams(alias.template_parameters) + " = " +
                    RustCore::convertType(alias.type) + ";");
    }

    // namespace-level variables: scalars -> const, everything else -> a lazily built static.
    // Mutation is not expressible this way (see notes).
    auto Construct::emitVariable(const LangAPI::Variable &v, Converter::Writer &w) -> void {
        const auto type = RustCore::convertType(v.type);
        if (type == "_") throw Error("Rust Converter: static `{}` needs an explicit type", v.name);
        const auto name = RustCore::ident(v.name);
        const auto value = v.value.empty() ? std::string("Default::default()") : initializer(v.type, v.value);
        if (RustCore::isConstScalar(v.type) && !v.value.empty()) {
            emitLine(w, "pub const " + name + ": " + type + " = " + value + ";");
        } else {
            emitLine(w, "pub static " + name + ": ::ispa_std::Lazy<" + type + "> = ::ispa_std::Lazy::new(|| " + value + ");");
        }
    }

    // generated enums standing in for std::variant<std::monostate, ...>
    auto Construct::emitVariants(Converter::Writer &w) -> void {
        // `Shared<..>` and the `Types::`/`FlatTypes::` namespaces do not tell types apart for assignment
        const auto unboxed = [](std::string text) {
            const std::string open = "::ispa_std::Shared<";
            for (auto at = text.find(open); at != std::string::npos; at = text.find(open)) {
                int depth = 1;
                std::size_t close = at + open.size();
                for (; close < text.size() && depth > 0; ++close) depth += text[close] == '<' ? 1 : text[close] == '>' ? -1 : 0;
                text.erase(close - 1, 1);
                text.erase(at, open.size());
            }
            for (const std::string prefix : {RustCore::crateRoot() + "::FlatTypes::", RustCore::crateRoot() + "::Types::", std::string("FlatTypes::"), std::string("Types::")})
                for (auto at = text.find(prefix); at != std::string::npos; at = text.find(prefix)) text.erase(at, prefix.size());
            return text;
        };
        // the elements of a tuple type `(A, B<C, D>, E)`
        const auto tuple_elements = [](const std::string &text) {
            std::vector<std::string> elements;
            if (text.size() < 2 || text.front() != '(' || text.back() != ')') return elements;
            int depth = 0;
            std::string current;
            for (std::size_t i = 1; i + 1 < text.size(); ++i) {
                const char c = text[i];
                if (c == '<' || c == '(' || c == '[') ++depth;
                else if (c == '>' || c == ')' || c == ']') --depth;
                if (c == ',' && depth == 0) { elements.push_back(current); current.clear(); continue; }
                current += c;
            }
            if (!current.empty() && current.find_first_not_of(" ") != std::string::npos) elements.push_back(current);
            for (auto &element : elements) element.erase(0, element.find_first_not_of(" "));
            return elements;
        };
        // how a tuple `from` fills a tuple `to` that has extra `String` elements (omitted captures): the target index of each source element
        const auto tuple_mapping = [&](const std::string &from, const std::string &to) -> std::optional<std::vector<std::size_t>> {
            const auto source = tuple_elements(from), target = tuple_elements(to);
            if (source.empty() || target.size() < source.size()) return std::nullopt;
            std::vector<std::size_t> mapping;
            std::size_t next = 0;
            for (std::size_t d = 0; d < target.size(); ++d) {
                if (next < source.size() && unboxed(source[next]) == unboxed(target[d])) { mapping.push_back(d); ++next; }
                else if (target[d] != "String") return std::nullopt;
            }
            return next == source.size() ? std::optional(mapping) : std::nullopt;
        };
        // assign_compatible into a variant: an alternative sets it; another variant sets it when it only holds alternatives
        // this one holds as well
        for (const auto &def : RustCore::variants) {
            for (const auto &alternative : def.alternatives)
                emitLine(w, "impl ::ispa_std::AssignFrom<" + alternative + "> for " + def.name + " { fn assign_from(&mut self, source: " +
                            alternative + ") { *self = source.into(); } }");
            // a tuple (a group) is accepted from another tuple that leaves out some of the target's `String` elements
            for (const auto &other : RustCore::variants)
                for (const auto &source : other.alternatives) {
                    if (tuple_elements(source).empty()) continue;
                    for (const auto &target : def.alternatives) {
                        if (source == target) continue;
                        const auto mapping = tuple_mapping(source, target);
                        if (!mapping) continue;
                        std::string assigns;
                        for (std::size_t i = 0; i < mapping->size(); ++i)
                            assigns += "::ispa_std::AssignFrom::assign_from(&mut filled." + std::to_string((*mapping)[i]) + ", source." + std::to_string(i) + "); ";
                        emitLine(w, "impl ::ispa_std::AssignFrom<" + source + "> for " + def.name + " { fn assign_from(&mut self, source: " + source +
                                    ") { let mut filled: " + target + " = Default::default(); " + assigns + "*self = filled.into(); } }");
                        break;
                    }
                }
            // a node of a rule is accepted boxed or unboxed, whichever form the variant holds
            for (const auto &alternative : def.alternatives) {
                const std::string node = "::ispa_std::Node<", shared = "::ispa_std::Shared<";
                if (!alternative.starts_with(node) || !alternative.ends_with(">")) continue;
                const auto inner = alternative.substr(node.size(), alternative.size() - node.size() - 1);
                std::string other;
                if (inner.starts_with(shared) && inner.ends_with(">")) {
                    other = node + inner.substr(shared.size(), inner.size() - shared.size() - 1) + ">";
                    if (std::find(def.alternatives.begin(), def.alternatives.end(), other) == def.alternatives.end())
                        emitLine(w, "impl ::ispa_std::AssignFrom<" + other + "> for " + def.name + " { fn assign_from(&mut self, source: " + other +
                                    ") { let mut boxed: " + alternative + " = Default::default(); ::ispa_std::AssignFrom::assign_from(&mut boxed, source); *self = boxed.into(); } }");
                } else {
                    other = node + shared + inner + ">>";
                    if (std::find(def.alternatives.begin(), def.alternatives.end(), other) == def.alternatives.end())
                        emitLine(w, "impl ::ispa_std::AssignFrom<" + other + "> for " + def.name + " { fn assign_from(&mut self, source: " + other +
                                    ") { if !source.empty() { let mut node = ::ispa_std::Node::create(source.value.borrow().clone()); "
                                    "node.start_pos = source.start_pos; node.length = source.length; node.line = source.line; node.column = source.column; "
                                    "node.name = source.name; *self = node.into(); } } }");
                }
            }
            for (const auto &other : RustCore::variants) {
                if (other.name == def.name) continue;
                const bool covered = std::all_of(other.alternatives.begin(), other.alternatives.end(), [&](const auto &alternative) {
                    return std::any_of(def.alternatives.begin(), def.alternatives.end(), [&](const auto &candidate) {
                        return unboxed(candidate) == unboxed(alternative) || tuple_mapping(alternative, candidate).has_value(); }); });
                if (!covered) continue;
                emitLine(w, "impl ::ispa_std::AssignFrom<" + other.name + "> for " + def.name + " { fn assign_from(&mut self, source: " +
                            other.name + ") { match source { " + other.name + "::Monostate => {}");
                for (std::size_t i = 0; i < other.alternatives.size(); ++i)
                    emitLine(w, "    " + other.name + "::V" + std::to_string(i) + "(x) => ::ispa_std::AssignFrom::assign_from(self, x),");
                emitLine(w, "} } }");
            }
        }
        for (const auto &def : RustCore::variants) {
            emitLine(w, "#[derive(Clone, Default)]");
            emitLine(w, "pub enum " + def.name + " {");
            w.increaseIndentation();
            emitLine(w, "#[default] Monostate,");
            for (std::size_t i = 0; i < def.alternatives.size(); ++i) emitLine(w, "V" + std::to_string(i) + "(" + def.alternatives[i] + "),");
            w.decreaseIndentation();
            emitLine(w, "}");
            for (std::size_t i = 0; i < def.alternatives.size(); ++i) {
                const auto v = "V" + std::to_string(i);
                const auto &t = def.alternatives[i];
                emitLine(w, "impl From<" + t + "> for " + def.name + " { fn from(v: " + t + ") -> Self { " + def.name + "::" + v + "(v) } }");
                emitLine(w, "impl ::ispa_std::VariantGet<" + t + "> for " + def.name + " {");
                w.increaseIndentation();
                emitLine(w, "fn holds(&self) -> bool { matches!(self, " + def.name + "::" + v + "(_)) }");
                emitLine(w, "fn get(&self) -> &" + t + " { match self { " + def.name + "::" + v + "(x) => x, _ => ::ispa_std::bad_variant_access() } }");
                emitLine(w, "fn get_mut(&mut self) -> &mut " + t + " { match self { " + def.name + "::" + v + "(x) => x, _ => ::ispa_std::bad_variant_access() } }");
                w.decreaseIndentation();
                emitLine(w, "}");
            }
            emitLine(w, "impl ::ispa_std::PrintValue for " + def.name + " {");
            w.increaseIndentation();
            emitLine(w, "fn structured(&self) -> bool { true }");
            emitLine(w, "fn print_subtree(&self, printer: &mut ::ispa_std::ASTPrinter) {");
            w.increaseIndentation();
            emitLine(w, "match self {");
            w.increaseIndentation();
            emitLine(w, def.name + "::Monostate => {}");
            for (std::size_t i = 0; i < def.alternatives.size(); ++i)
                emitLine(w, def.name + "::V" + std::to_string(i) + "(x) => ::ispa_std::PrintValue::print_subtree(x, printer),");
            w.decreaseIndentation();
            emitLine(w, "}");
            w.decreaseIndentation();
            emitLine(w, "}");
            w.decreaseIndentation();
            emitLine(w, "}");
            emitLine(w, "impl ::ispa_std::ToText for " + def.name + " {");
            w.increaseIndentation();
            emitLine(w, "fn to_text(&self) -> String {");
            w.increaseIndentation();
            emitLine(w, "match self {");
            w.increaseIndentation();
            emitLine(w, def.name + "::Monostate => String::new(),");
            for (std::size_t i = 0; i < def.alternatives.size(); ++i)
                emitLine(w, def.name + "::V" + std::to_string(i) + "(x) => ::ispa_std::ToText::to_text(x),");
            w.decreaseIndentation();
            emitLine(w, "}");
            w.decreaseIndentation();
            emitLine(w, "}");
            w.decreaseIndentation();
            emitLine(w, "}");
            // which alternative a type is (a rule type is none of them): how the core addresses tokens by type
            emitLine(w, "impl ::ispa_std::TokenVariant for " + def.name + " {");
            w.increaseIndentation();
            emitLine(w, "fn variant_index(&self) -> usize { self.index() }");
            emitLine(w, "fn held(&self) -> &dyn ::std::any::Any {");
            w.increaseIndentation();
            emitLine(w, "match self {");
            w.increaseIndentation();
            emitLine(w, def.name + "::Monostate => &(),");
            for (std::size_t i = 0; i < def.alternatives.size(); ++i)
                emitLine(w, def.name + "::V" + std::to_string(i) + "(x) => x,");
            w.decreaseIndentation();
            emitLine(w, "}");
            w.decreaseIndentation();
            emitLine(w, "}");
            emitLine(w, "fn index_of<__T: 'static>() -> Option<usize> {");
            w.increaseIndentation();
            emitLine(w, "let id = ::std::any::TypeId::of::<__T>();");
            for (std::size_t i = 0; i < def.alternatives.size(); ++i)
                emitLine(w, "if id == ::std::any::TypeId::of::<" + def.alternatives[i] + ">() { return Some(" + std::to_string(i + 1) + "); }");
            emitLine(w, "None");
            w.decreaseIndentation();
            emitLine(w, "}");
            w.decreaseIndentation();
            emitLine(w, "}");
            emitLine(w, "impl " + def.name + " {");
            w.increaseIndentation();
            emitLine(w, "pub fn index(&self) -> usize {");
            w.increaseIndentation();
            emitLine(w, "match self {");
            w.increaseIndentation();
            emitLine(w, def.name + "::Monostate => 0,");
            for (std::size_t i = 0; i < def.alternatives.size(); ++i)
                emitLine(w, def.name + "::V" + std::to_string(i) + "(_) => " + std::to_string(i + 1) + ",");
            w.decreaseIndentation();
            emitLine(w, "}");
            w.decreaseIndentation();
            emitLine(w, "}");
            w.decreaseIndentation();
            emitLine(w, "}");
        }
    }

    // ========================================================================
    // declarations
    // ========================================================================
    auto Construct::emitNamespace(const LangAPI::Namespace &ns, Converter::Writer &w) -> void {
        const bool root = context.module_path.empty() && ns.name == namespace_name;
        emitLine(w, "pub mod " + (root ? RustCore::moduleIdent(ns.name) : RustCore::ident(ns.name)) + " {");
        w.increaseIndentation();
        emitLine(w, "#[allow(unused_imports)] use super::*;");
        context.module_path.push_back(root ? RustCore::moduleIdent(ns.name) : ns.name);
        RustCore::scope.push_back(ns.name);
        emitDeclarations(ns.declarations, w);
        if (root) emitVariants(w);
        context.module_path.pop_back();
        RustCore::scope.pop_back();
        w.decreaseIndentation();
        emitLine(w, "}");
    }

    // one item, no file splitting
    auto Construct::emitItem(const LangAPI::Declaration &decl, Converter::Writer &w) -> void {
        if (decl.isNamespace()) {
            emitNamespace(decl.getNamespace(), w);
        } else if (decl.isFunction()) {
            Converter::Writer aux;
            const auto scope = joinStrings(context.module_path, "::");
            emitFunction(decl.getFunction(), w, aux, FnKind::Free, scope, {});
            const auto text = std::string(aux.get());
            if (!text.empty()) w.dwrite("{}", text);
        } else if (decl.isClass()) {
            emitClass(decl.getClass(), w);
        } else if (decl.isEnum()) {
            emitEnum(decl.getEnum(), w);
        } else if (decl.isTypeAlias()) {
            emitTypeAlias(decl.getTypeAlias(), w);
        } else if (decl.isVariable()) {
            emitVariable(decl.getVariable(), w);
        } else if (decl.isForwardDeclaredClass()) {
            // Rust needs no forward declarations: items are visible module-wide regardless of order
        }
    }

    // incremental mode: every namespace-level item gets its own file, declared and re-exported from the parent module
    auto Construct::emitDeclaration(const LangAPI::Declaration &decl, Converter::Writer &w) -> void {
        if (!incremental || decl.isNamespace() || decl.isForwardDeclaredClass()) {
            emitItem(decl, w);
            return;
        }
        std::string stem;
        if (decl.isFunction()) stem = decl.getFunction().name;
        else if (decl.isClass()) stem = RustCore::convertSymbol(decl.getClass().name);
        else if (decl.isEnum()) stem = decl.getEnum().name;
        else if (decl.isTypeAlias()) stem = decl.getTypeAlias().name;
        else stem = decl.getVariable().name;
        const auto base = "f_" + fileSafe(stem);
        std::string module_name = base;
        if (const auto n = ++context.file_counts[joinStrings(context.module_path, "/") + "/" + base]; n > 1)
            module_name += "_" + std::to_string(n);

        Converter::Writer file;
        emitLine(file, "#[allow(unused_imports)] use super::*;");
        emitItem(decl, file);

        const auto path = joinStrings(context.module_path, "/");
        context.files.emplace_back((path.empty() ? "" : path + "/") + module_name + ".rs", std::string(file.get()));
        emitLine(w, "pub mod " + module_name + ";");
        emitLine(w, "pub use self::" + module_name + "::*;");
    }

    auto Construct::emitDeclarations(const LangAPI::Declarations &decls, Converter::Writer &w) -> void {
        for (const auto &decl : decls) emitDeclaration(decl, w);
    }

    auto Construct::construct() -> void {
        RustCore::name = std::string(namespace_name);
        RustCore::variants.clear();
        RustCore::diagnostics.clear();
        RustCore::counter = {RustCore::base_pointer};
        RustCore::loop_frames.clear();
        RustCore::label_counter = 0;
        context = Context{};

        writable_files.clear();
        RustCore::scope.clear();
        RustCore::locals.clear();
        RustCore::borrowed_parameters.clear();
        RustCore::members.clear();
        RustCore::static_members.clear();
        RustCore::receiver.clear();
        RustCore::shared_receiver = false;
        RustCore::return_type = nullptr;
        program = Rust::Rewriter::rewrite(*source);
        RustCore::indexFunctions(program);

        Converter::Writer body;
        emitDeclarations(program, body);

        std::string header = "// @generated by the ISPA Rust backend. Expects the `ispa_std` crate and to live at `crate::" +
                             RustCore::moduleIdent(std::string(namespace_name)) + "`.\n";
        header += kAllow;
        for (const auto &d : RustCore::diagnostics) header += "// ISPA-RUST-WARNING: " + d + "\n";
        header += "\n";

        writable_files.clear();
        writable_files.emplace_back(std::string(namespace_name) + ".rs", header + std::string(body.get()));
        if (incremental)
            for (auto &file : context.files) writable_files.emplace_back(std::move(file));
    }
}

extern "C" auto rust_construct(const LangAPI::Declarations *tree, const char *namespace_name,
                               bool incremental, bool debug_instrumentation) -> Rust::Construct* {
    return new Rust::Construct(tree, namespace_name, incremental, debug_instrumentation);
}
extern "C" auto rust_construct_finish(Rust::Construct *c) -> void {
    delete c;
}
