export module Converter.Declarations;

import Converter.Writer;
import LangAPI;
import Rope.String;
import dstd;
import std;
export namespace Converter {
    class Declarations {
    protected:
        Writer *output;

    public:
        // declarations
        virtual auto openFile(const std::string &namespace_name) -> void = 0;
        virtual auto closeFile(const std::string &namespace_name) -> void = 0;
        virtual auto initImports() -> void = 0;
        virtual auto createNamespace(const std::string &name) -> void = 0;
        virtual auto closeNamespace() -> void = 0;
        virtual auto createClass(const LangAPI::Class &the_class) -> void = 0;
        virtual auto closeClass() -> void = 0;
        virtual auto createForwardDeclarationClass(LangAPI::ForwardDeclaredClass forward_declared_class) -> void = 0;
        virtual auto setVisibility(LangAPI::Visibility visibility) -> void = 0;
        virtual auto createFunction(const LangAPI::Function &func) -> void = 0;
        virtual auto createPrintFunction(const LangAPI::Function &func) -> void = 0;
        virtual auto closeFunction() -> void = 0;
        virtual auto closePrintFunction() -> void = 0;
        virtual auto openTemplateParameters() -> void = 0;
        virtual auto createTemplateParameter(const std::string &name) -> void = 0;
        virtual auto closeTemplateParameters() -> void = 0;

        // types
        virtual auto createTypeAlias(const std::string &name, const LangAPI::Type type) -> void = 0;
        virtual auto createEnum(const std::string &name, const stdu::vector<std::string> &names) -> void = 0;
        virtual auto closeEnum() -> void = 0;

        virtual auto createVariable(const LangAPI::Variable &v) -> void = 0;
        Declarations(Writer &output) : output(&output) {}
        virtual ~Declarations() = default;

        virtual Writer &getWriter() const { return *output; }
        virtual Writer &getWriter() { return *output; }
    };
}