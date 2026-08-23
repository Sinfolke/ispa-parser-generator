export module LangRepr.Converter;

import LangRepr.Holder;
import LangAPI;
import Converter.Declarations;
import Converter.Statement;
import Converter.Writer;
import Converter.Init;
import Rope.String;
import cpuf.dlib;
import std;
export namespace LangRepr {
    class Converter {
        ::Converter::Writer output;
        dlib converter_lib;
        const Holder &holder;
        const std::string &lang;
        const std::string &namespace_name;
        std::unique_ptr<::Converter::Declarations, void(*)(::Converter::Declarations*)> decls;
        std::unique_ptr<::Converter::Statement, void(*)(::Converter::Statement*)> stmts;
        void buildDeclaration(const LangAPI::Declaration &decl) {
            if (decl.isNamespace()) {
                const auto &namespace_decl = decl.getNamespace();
                decls->createNamespace(namespace_decl.name);
                buildDeclarations(namespace_decl.declarations);
                decls->closeNamespace();
            } else if (decl.isClass()) {
                const auto &cls = decl.getClass();
                decls->createClass(cls);
                for (const auto &[declaration, visibility] : cls.data) {
                    decls->setVisibility(visibility);
                    buildDeclaration(*declaration);
                }
                decls->closeClass();
            } else if (decl.isForwardDeclaredClass()) {
                decls->createForwardDeclarationClass(decl.getForwardDeclaredClass());
            } else if (decl.isVariable()) {
                const auto &var_decl = decl.getVariable();
                decls->createVariable(var_decl);
            } else if (decl.isTypeAlias()) {
                const auto &type_alias = decl.getTypeAlias();
                decls->createTypeAlias(type_alias.name, type_alias.type);
            } else if (decl.isEnum()) {
                const auto &enum_decl = decl.getEnum();
                decls->createEnum(enum_decl.name, enum_decl.value);
                decls->closeEnum();
            } else if (decl.isFunction()) {
                const auto &func_decl = decl.getFunction();
                if (lang == "cpp") {
                    if (!func_decl.template_parameters.empty()) {
                        decls->openTemplateParameters();
                        for (const auto &t : func_decl.template_parameters) decls->createTemplateParameter(t);
                        decls->closeTemplateParameters();
                    }
                }
                decls->createFunction(func_decl);
                buildStatements(func_decl.statements);
                decls->closeFunction();
            }
        }
        void buildDeclarations(const LangAPI::Declarations &declarations) {
            for (const auto &declaration : declarations) {
                buildDeclaration(declaration);
            }
        }
        auto buildStatement(const LangAPI::Statement &stmt) -> void {
            if (stmt.isIf()) {
                stmts->createIf(stmt.getIf().expr);
                buildStatements(stmt.getIf().stmt);
                stmts->closeIf();
            } else if (stmt.isWhile()) {
                stmts->createWhile(stmt.getWhile().expr);
                buildStatements(stmt.getWhile().stmt);
                stmts->closeWhile();
            } else if (stmt.isDoWhile()) {
                const auto dowhile = stmt.getDoWhile();
                stmts->openDoWhile();
                buildStatements(dowhile.stmt);
                stmts->closeDoWhile(dowhile.expr);
            } else if (stmt.isSwitch()) {
                const auto switch_statement = stmt.getSwitch();
                stmts->createSwitch(switch_statement.expression);
                for (const auto &case_statement : switch_statement.cases) {
                    stmts->createCase(case_statement.first);
                    buildStatements(case_statement.second);
                    stmts->closeCase();
                }
                stmts->closeSwitch();
            } else if (stmt.isExpression()) {
                stmts->createExpression(stmt.getExpression());
            } else if (stmt.isVariable()) {
                stmts->createVariable(stmt.getVariable());
            } else if (stmt.isThrow()) {
                stmts->createThrow(stmt.getThrow());
            }
        }
        auto buildStatements(const LangAPI::Statements &statements) -> void {
            for (const auto &statement : statements) {
                buildStatement(statement);
            }
        }
    public:
        Converter(const Holder &holder, const std::string &lang, const std::string &namespace_name) :
        converter_lib("libispa-converter-" + lang), holder(holder), lang(lang), decls(nullptr, nullptr), stmts(nullptr, nullptr), namespace_name(namespace_name) {
            decls = std::unique_ptr<::Converter::Declarations, void(*)(::Converter::Declarations*)>
                (
                    converter_lib.loadfun<::Converter::Declarations*, ::Converter::Writer*>(std::string("create_") + lang + "_declarations")(&output),
                    converter_lib.loadfun<void, ::Converter::Declarations*>(std::string("delete_") + lang + "_declarations")
                );
            stmts = std::unique_ptr<::Converter::Statement, void(*)(::Converter::Statement*)>
                (
                    converter_lib.loadfun<::Converter::Statement*, ::Converter::Writer*>(std::string("create_") + lang + "_statement")(&output),
                    converter_lib.loadfun<void, ::Converter::Statement*>(std::string("delete_") + lang + "_statement")
                );
        }
        ~Converter() {}
        auto build() -> void {
            converter_lib.loadfun<void, const char*>(std::string("init"))(namespace_name.c_str());
            decls->openFile(namespace_name);
            decls->initImports();
            buildDeclarations(holder.get());
            decls->closeFile(namespace_name);
            converter_lib.loadfun<void>(std::string("close"))();
        }
        auto getSource() -> std::optional<std::string> {
            try {
                auto getSourceFun = converter_lib.loadfun<std::optional<std::string>*>(std::string("getSource"));
                auto ptr = getSourceFun();
                if (ptr == nullptr) return std::nullopt;
                auto res = *ptr;
                delete ptr;
                return res;
            } catch (...) {
                return std::nullopt;
            }
        }
        auto get() { return decls->getWriter(); }
    };

}
