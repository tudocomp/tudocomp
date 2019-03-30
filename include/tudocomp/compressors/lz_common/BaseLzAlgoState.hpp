#pragma once

namespace tdc {namespace lz_common {
    template<typename encoder_t, typename dict_t, typename stats_t>
    class BaseLzAlgoState {
    protected:
        size_t& m_factor_count;
        encoder_t& m_coder;
        dict_t& m_dict;
        stats_t& m_stats;
    public:
        inline BaseLzAlgoState(size_t& factor_count,
                               encoder_t& coder,
                               dict_t& dict,
                               stats_t& stats):
           m_factor_count(factor_count),
           m_coder(coder),
           m_dict(dict),
           m_stats(stats) {}
        BaseLzAlgoState(BaseLzAlgoState const&) = delete;
        BaseLzAlgoState& operator=(BaseLzAlgoState const&) = delete;
    };
}}
