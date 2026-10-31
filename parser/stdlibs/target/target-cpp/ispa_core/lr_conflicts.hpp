// ISPA_CORE::LRGeneralizedParser: LR with runtime conflict resolution, driven by the lowered core.
#pragma once
#include <ispastdlib/lr_conflicts.hpp>
#include <ispa_core/lr_machine.hpp>
#include <ispa_core/namespace.hpp>

namespace ISPA_CORE {
// Ambiguous grammars: the deterministic facade's value helpers and fail-block host, driven by the generalized core.
template<class MainNode, class Token>
class LRGeneralizedParser : public LRParser<MainNode, Token> {
    std::shared_ptr<core_detail::lo::lr_conflicts::LRGeneralizedParser> generalized_core_;
protected:
    template<class Generated, class IT>
    ISPA_STD::MatchResult<MainNode, IT> lr_parse_generalized(IT pos) {
        if (!generalized_core_) { ISPA_STD::HeapScope heap; generalized_core_ = ISPA_STD::construct<core_detail::lo::lr_conflicts::LRGeneralizedParser>(); }
        return core_detail::lr_core_run<Generated, MainNode, Token>(generalized_core_, this->failure_host_, this->errors_, std::move(pos), true);
    }
};
}  // namespace ISPA_CORE
