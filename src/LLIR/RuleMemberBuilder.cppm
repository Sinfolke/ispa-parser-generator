export module LLIR.Rule.MemberBuilder;

import LLIR.Builder.Base;
import LLIR.Builder.DataWrapper;
import AST.API;
import LangAPI;
import dstd;
import std;

export namespace LLIR {
    class MemberBuilder : public BuilderBase {
        const stdu::vector<std::shared_ptr<AST::RuleMember>> *rules = nullptr;
        const AST::RuleMember *rule = nullptr;
        bool addSpaceSkipFirst;
        void buildMember(const AST::RuleMember &member);
    public:
        void build() override;
        MemberBuilder(BuilderDataWrapper &data, const AST::RuleMember &rule) : BuilderBase(data), rule(&rule) {}
        MemberBuilder(BuilderDataWrapper &data, const stdu::vector<std::shared_ptr<AST::RuleMember>> &rules) : BuilderBase(data), rules(&rules) {}
        auto getAddSpaceSkipFirst() const -> bool { return addSpaceSkipFirst; }
    };

    class GroupBuilder : public BuilderBase {
        const AST::RuleMember &rule;
        void pushBasedOnQuantifier(
            MemberBuilder &builder,
            const AST::RuleMember &rule,
            LangAPI::Variable &shadow_var,
            LangAPI::Variable &uvar,
            const LangAPI::Variable &var,
            char quantifier
        );
    public:
        void build() override;
        GroupBuilder(BuilderDataWrapper &data, const AST::RuleMember &rule) : BuilderBase(data), rule(rule) {}
    };

    class CsequenceBuilder : public BuilderBase {
        const AST::RuleMember &rule;
    public:
        void build() override;
        CsequenceBuilder(BuilderDataWrapper &data, const AST::RuleMember &rule) : BuilderBase(data), rule(rule) {}
    };

    class StringBuilder : public BuilderBase {
        const AST::RuleMember &rule;
    public:
        void build() override;
        StringBuilder(BuilderDataWrapper &data, const AST::RuleMember &rule) : BuilderBase(data), rule(rule) {}
    };

    // class HexBuilder : public BuilderBase {
    //     const AST::RuleMember &rule;
    // public:
    //     void build() override;
    //     HexBuilder(BuilderDataWrapper &data, const AST::RuleMember &rule) : BuilderBase(data), rule(rule) {}
    // };
    //
    // class BinBuilder : public BuilderBase {
    //     const AST::RuleMember &rule;
    // public:
    //     void build() override;
    //     BinBuilder(BuilderDataWrapper &data, const AST::RuleMember &rule) : BuilderBase(data), rule(rule) {}
    // };

    class NameBuilder : public BuilderBase {
        const AST::RuleMember &rule;
        auto pushBasedOnQualifier(
            const AST::RuleMember &rule,
            LangAPI::Expression &expr,
            LangAPI::Statements &stmt,
            LangAPI::Variable &uvar,
            const LangAPI::Variable &var,
            const LangAPI::Variable &svar,
            const LangAPI::Statement &call,
            char quantifier,
            stdu::vector<std::string> &name,
            bool add_shadow_var = false
        ) -> LangAPI::Variable;
        auto createAssignUvarBlock(LangAPI::Statements &statements, const LangAPI::Variable &uvar, const LangAPI::Variable &var, const LangAPI::Variable &shadow_var) -> void;
    public:
        void build() override;
        NameBuilder(BuilderDataWrapper &data, const AST::RuleMember &rule) : BuilderBase(data), rule(rule) {}
    };

    class NospaceBuilder : public BuilderBase {
    public:
        void build() override;
        NospaceBuilder(BuilderDataWrapper &data) : BuilderBase(data) {}
    };

    // class EscapedBuilder : public BuilderBase {
    //     const AST::RuleMember &rule;
    // public:
    //     void build() override;
    //     EscapedBuilder(BuilderDataWrapper &data, const AST::RuleMember &rule) : BuilderBase(data), rule(rule) {}
    // };

    class AnyBuilder : public BuilderBase {
        const AST::RuleMember &rule;
    public:
        void build() override;
        AnyBuilder(BuilderDataWrapper &data, const AST::RuleMember &rule) : BuilderBase(data), rule(rule) {}
    };

    class OpBuilder : public BuilderBase {
        const AST::RuleMember &rule;
        auto createBlock(
            const stdu::vector<std::shared_ptr<AST::RuleMember>> &rules,
            std::size_t index,
            LangAPI::Variable &var,
            LangAPI::Variable &svar
        ) -> LangAPI::Statements;
    public:
        void build() override;
        OpBuilder(BuilderDataWrapper &data, const AST::RuleMember &rule) : BuilderBase(data), rule(rule) {}
    };
}
