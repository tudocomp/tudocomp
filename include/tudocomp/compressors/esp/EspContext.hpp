#pragma once

#include <tudocomp/util/View.hpp>
#include <tudocomp/compressors/esp/SLP.hpp>
#include <tudocomp/compressors/esp/Rounds.hpp>
#include <tudocomp/compressors/esp/utils.hpp>

namespace tdc {namespace esp {
    template<typename ipd_t>
    class EspContext {
        const Env* m_env;
    public:
        using esp_view_t = ConstGenericView<size_t>;

        IPDStats ipd_stats;

        EspContext(const Env* e):
            m_env(e)
        {}

        bool behavior_metablocks_maximimze_repeating = true;
        bool behavior_landmarks_tie_to_right = true;
        bool behavior_iter_log_mode = false; // UNUSED

        template<typename T>
        SLP generate_grammar(T&& s);
    };
}}
