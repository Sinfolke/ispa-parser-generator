export module Cpp.Statement;

import Converter.Statement;
import Converter.Writer;
import LangAPI;
import Rope.String;
import std;

export namespace Cpp {
    class Statement : public ::Converter::Statement {
    public:
        Statement(Converter::Writer &output);

        auto createIf(const LangAPI::Expression &expression)-> void override;
        auto closeIf() -> void override;
        auto createWhile(const LangAPI::Expression &expression) -> void override;
        auto closeWhile() -> void override;
        auto openDoWhile() -> void override;
        auto closeDoWhile(const LangAPI::Expression &expression) -> void override;
        auto createSwitch(const LangAPI::Expression &expression) -> void override;
        auto createCase(const LangAPI::RValue &rvalue) -> void override;
        auto closeCase() -> void override;
        auto closeSwitch() -> void override;
        auto createExpression(const LangAPI::Expression &expression) -> void override;
        auto createVariable(const LangAPI::Variable &v) -> void override;
        auto createThrow(const LangAPI::Throw &t) -> void override;
        Converter::Writer &getWriter() override;
        Converter::Writer &getWriter() const override;
        virtual ~Statement() = default;
    };
}
