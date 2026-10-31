module;
#include <tsl/ordered_map.h>
export module ParserIR.API;

import LangAPI;
import AST.API;
import AST.Tree;
import LangAPI.Accessor;
import hash;
import dstd;

export namespace ParserIR {
    using inclosed_map = tsl::ordered_map<std::string, std::pair<LangAPI::Expression, LangAPI::Type>>;
    using regular_data_block = std::pair<LangAPI::Expression, LangAPI::Type>;
    struct DataBlock {
        std::variant<std::monostate, regular_data_block, inclosed_map> value;
        bool is_inclosed_map() const { return std::holds_alternative<inclosed_map>(value); };
        bool is_regular_data_block() const { return std::holds_alternative<regular_data_block>(value); };
        bool empty() const { return std::holds_alternative<std::monostate>(value); };
        regular_data_block &getRegularDataBlock() { return std::get<regular_data_block>(value); };
        inclosed_map &getInclosedMap() { return std::get<inclosed_map>(value); };
        const regular_data_block &getRegularDataBlock() const { return std::get<regular_data_block>(value); };
        const inclosed_map &getInclosedMap() const { return std::get<inclosed_map>(value); };
    private:
        friend struct ::uhash;
        friend struct LangAPI::MembersAccess;
        [[nodiscard]] auto members() const {
            return std::tie(value);
        }
        auto members() {
            return std::tie(value);
        }
    };
    struct ExportsAfterBuild {
        LangAPI::Variable svar;
        LangAPI::Variable uvar;
        LangAPI::Variable var;
        LangAPI::Variable shadow_var;
        char quantifier;
    private:
        friend struct ::uhash;
        friend struct ::LangAPI::MembersAccess;
        [[nodiscard]] auto members() const {
            return std::tie(svar, uvar, var, shadow_var, quantifier);
        }
        auto members() {
            return std::tie(svar, uvar, var, shadow_var, quantifier);
        }
    };
    // Serializable subset of ParserIR::BuilderData needed by downstream rules
    // and data-block construction after an incremental cache hit.
    struct IncrementalBuilderData {
        stdu::vector<LangAPI::Variable> unnamed_datablock_units;
        stdu::vector<std::pair<std::string, LangAPI::Variable>> key_vars;
        stdu::vector<ExportsAfterBuild> exports_list;

        // Missing persistent generation context
        stdu::vector<LangAPI::Variable> vars;
        std::size_t variable_count = 0;

    private:
        friend struct ::uhash;
        friend struct ::LangAPI::MembersAccess;

        [[nodiscard]] auto members() const {
            return std::tie(
                unnamed_datablock_units,
                key_vars,
                exports_list,
                vars,
                variable_count
            );
        }

        auto members() {
            return std::tie(
                unnamed_datablock_units,
                key_vars,
                exports_list,
                vars,
                variable_count
            );
        }
    };
    struct Production {
        DataBlock block;
        stdu::vector<std::string> name;
        LangAPI::Statements rule_members;

        // Builder state needed to resume incremental construction.
        IncrementalBuilderData builder_data;
    private:
        friend struct ::uhash;
        friend struct ::LangAPI::MembersAccess;
        [[nodiscard]] auto members() const {
            return std::tie(block, name, rule_members, builder_data);
        }
        auto members() {
            return std::tie(block, name, rule_members, builder_data);
        }
    };
    auto dataBlock(const AST::DataBlock& block) -> DataBlock;
    // The rule's data block extended with the AST fields its actions and fail handlers declare (ctx.ast.x).
    auto withActionFields(AST::Tree& tree, const stdu::vector<std::string>& name, const AST::Rule& rule, DataBlock block) -> DataBlock;
    using DataBlockList = tsl::ordered_map<stdu::vector<std::string>, DataBlock, uhash>;
}
