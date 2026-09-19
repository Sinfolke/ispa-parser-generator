module Cpp.Declarations;
import Cpp.CoreFunctions;
import Cpp.Statement;
import Cpp.CoreFunctions;
import corelib;
import logging;
import std;

namespace Cpp {
    Declarations::Declarations(Converter::Writer &output) :  Converter::Declarations(output) {
        Core::declarations_converter = this;
    }
    auto Declarations::openFile(const std::string &namespace_name) -> void {
        Core::output->writeln("#ifndef {}_H", corelib::text::ToUpper(namespace_name));
        Core::output->writeln("#define {}_H", corelib::text::ToUpper(namespace_name));
    }

    auto Declarations::closeFile(const std::string &namespace_name) -> void {
        Core::output->writeln("#endif // {}_H", corelib::text::ToUpper(namespace_name));
    }
    auto Declarations::initImports() -> void {
        Core::output->writeln("#include <string>");
        Core::output->writeln("#include <vector>");
        Core::output->writeln("#include <unordered_map>");
        Core::output->writeln("#include <array>");
        Core::output->writeln("#include <string>");
        Core::output->writeln("#include <variant>");
        Core::output->writeln("#include <optional>");
        Core::output->writeln("#include <memory>");
        Core::output->writeln("#include <ispastdlib.hpp>");
    }

    auto Declarations::createNamespace(const std::string &name) -> void {
        Core::output->writeln("namespace {} {", name);;
        Core::output->increaseIndentation();
    }
    auto Declarations::closeNamespace() -> void {
        Core::output->decreaseIndentation();
        Core::output->writeln("}");
    }
    auto Declarations::createClass(const LangAPI::Class &the_class) -> void {
        Core::prev_visibility = the_class.default_visibility;
        if (the_class.default_visibility == LangAPI::Visibility::Public) {
            Core::output->write("struct {}", the_class.name);
        } else {
            Core::output->write("class {}", the_class.name);
        }
        if (!the_class.inherit_members.empty()) {
            Core::output->dwrite(" : ");
        }
        for (const auto &inherit : the_class.inherit_members) {
            if (inherit.first != the_class.default_visibility) {
                if (inherit.first == LangAPI::Visibility::Public) {
                    Core::output->dwrite("public");
                } else {
                    Core::output->dwrite("private");
                }
            } else Core::output->pop_back(); // remove space
            if (std::holds_alternative<LangAPI::Symbol>(inherit.second))
                Core::output->dwrite(" {}", Core::convertSymbol(std::get<LangAPI::Symbol>(inherit.second)));
            else
                Core::output->dwrite(" {}", Core::convertIspaLibSymbol(std::get<LangAPI::IspaLibSymbol>(inherit.second)));
            if (&inherit != &the_class.inherit_members.back()) {
                Core::output->dwrite(", ");
            }
        }
        Core::output->dwriteln(" {");
        Core::output->increaseIndentation();
        Core::symbol_path.push_back(the_class.name);
    }
    auto Declarations::closeClass() -> void {
        Core::flushInitContent();
        Core::output->decreaseIndentation();
        Core::output->writeln("};");
        Core::symbol_path.pop_back();
        Core::templated = false;
    }
    auto Declarations::createForwardDeclarationClass(const LangAPI::ForwardDeclaredClass forward_declared_class) -> void {
        Core::output->writeln("{} {};", forward_declared_class.isStruct ? "struct" : "class", forward_declared_class.name);
    }
    auto Declarations::setVisibility(const LangAPI::Visibility visibility) -> void {
        if (Core::prev_visibility != visibility) {
            Core::output->decreaseIndentation();
            switch (visibility) {
                case LangAPI::Visibility::Private:
                    Core::output->writeln("private: ");
                    break;
                case LangAPI::Visibility::Public:
                    Core::output->writeln("public: ");
                    break;
                default:
                    throw Error("Unknown visibility");
            }
            Core::output->increaseIndentation();
            Core::prev_visibility = visibility;
        }
    }

    auto Declarations::createFunction(const LangAPI::Function &func) -> void {
        Core::output->write("{}auto {}(", func.is_static ? "static " : "", func.name);
        if (!func.parameters.empty()) {
            for (const auto &p : func.parameters) {
                Core::output->dwrite("{} {}, ", Core::convertType(p.first), p.second);
            }
            Core::output->pop_back();
            Core::output->pop_back();
        }
        Core::output->dwrite(") -> {}{}", Core::convertType(func.type), func.override ? " override" : "");
        if (!func.statements.empty()) {
            if (func.template_parameters.empty()) {
                Core::output->dwriteln(";");
                Core::output = &Core::cpp_file;
                Core::output->dwrite("auto {}::{} (", corelib::text::join(Core::symbol_path, "::"), func.name);
                if (!func.parameters.empty()) {
                    for (const auto &p : func.parameters) {
                        Core::output->dwrite("{} {}, ", Core::convertType(p.first), p.second);
                    }
                    Core::output->pop_back();
                    Core::output->pop_back();
                }
                Core::output->dwrite(") -> {}", Core::convertType(func.type));
            }
            Core::output->dwriteln("{");
            Core::output->increaseIndentation();
            Core::symbol_path.push_back(func.name);
        } else {
            Core::output->dwriteln(";");
            Core::forward_declared = true;
        }
    }
    auto Declarations::closeFunction() -> void {
        if (Core::forward_declared) {
            Core::forward_declared = false;
        } else {
            Core::output->decreaseIndentation();
            Core::output->write("}\n");
            Core::symbol_path.pop_back();
            Core::templated = false;
            Core::output = &Core::h_file;
        }
    }
    auto Declarations::openTemplateParameters() -> void {
        Core::output->write("template<");
        Core::templated = true;
    }
    auto Declarations::createTemplateParameter(const std::string &name) -> void {
        if (!Core::first_template_parameter) {
            Core::output->dwrite(", ");
            Core::first_template_parameter = false;
        }
        Core::output->dwrite("typename {}", name);
    }
    auto Declarations::closeTemplateParameters() -> void {
        Core::output->dwriteln(">");
        Core::first_template_parameter = true;
    }
    auto Declarations::createTypeAlias(const std::string &name, const LangAPI::Type type) -> void {
        Core::output->writeln("using {} = {};", name, Core::convertType(type));
    }
    auto Declarations::createEnum(const std::string &name, const stdu::vector<std::string> &names) -> void {
        Core::output->writeln("enum class {} {", name);
        Core::output->increaseIndentation();
        for (const auto &n : names) {
            Core::output->writeln("{},", n);
        }
        Core::output->decreaseIndentation();
    }
    auto Declarations::closeEnum() -> void {
        Core::output->writeln("};");
    }
    auto Declarations::createVariable(const LangAPI::Variable &v) -> void {
        Core::output->write("{}{} {}", v.is_static ? "static " : "", Core::convertType(v.type), v.name);
        bool nested_array_type = v.nested_array_type || Core::isNestedArrayType(v.type);
        if (v.is_static) {
            if (!v.set.empty()) {
                for (const auto &expr : v.set) {
                    Core::init_content << Core::convertExpression(Core::ensureNamespaced(v.name, expr)) << ";\n";
                }
            }
            if (!v.value.empty()) {
                auto cpp_sym_path = Core::symbol_path;
                cpp_sym_path.erase(cpp_sym_path.begin());
                Core::cpp_file.writeln("{} {}::{} = {}{}{};", Core::convertType(v.type), corelib::text::join(cpp_sym_path, "::"), v.name, nested_array_type ? "{" : "", Core::convertExpression(v.value), nested_array_type ? "}" : "");
            }
        }
        Core::output->dwriteln(";");
        Core::inside_array = false;
    }

    Converter::Writer &Declarations::getWriter() {
        return Core::h_file;
    }
    Converter::Writer &Declarations::getWriter() const {
        return Core::h_file;
    }
}
extern "C" Cpp::Declarations* create_cpp_declarations(Converter::Writer *output) {
    return new Cpp::Declarations(*Core::output);
}
extern "C" void delete_cpp_declarations(Cpp::Declarations *decl) {
    delete decl;
}