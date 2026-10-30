// ISPA_CORE::LRGeneralizedParser: LR with runtime conflict resolution, driven by the lowered core.
#pragma once
#include <ispastdlib/lr_conflicts.hpp>
#include <ispa_core/lr_machine.hpp>
#include <ispa_core/namespace.hpp>

namespace ISPA_CORE {
template<class MainNode, class Token>
class LRGeneralizedParser : public ISPA_STD::LRGeneralizedParser<MainNode, Token> {
    std::shared_ptr<core_detail::lo::lr_conflicts::LRGeneralizedParser> core_;
protected:
    template<class Generated, class IT>
    ISPA_STD::MatchResult<MainNode, IT> lr_parse_generalized(IT pos) {
        if (!core_) { ISPA_STD::HeapScope heap; core_ = ISPA_STD::construct<core_detail::lo::lr_conflicts::LRGeneralizedParser>(); }
        return core_detail::lr_core_run<Generated, MainNode, Token>(core_, this->errors_, std::move(pos), true);
    }
};
}  // namespace ISPA_CORE
