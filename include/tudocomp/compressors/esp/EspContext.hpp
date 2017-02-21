#pragma once

#include <tudocomp/util/View.hpp>
#include <tudocomp/compressors/esp/SLP.hpp>
#include <tudocomp/compressors/esp/Rounds.hpp>

namespace tdc {namespace esp {
    struct EspContext {
        inline EspContext() {}

        bool behavior_metablocks_maximimze_repeating = true;
        bool behavior_landmarks_tie_to_right = true;
        bool behavior_iter_log_mode = false; // UNUSED

        Rounds generate_grammar_rounds(string_ref, bool);
        SLP generate_grammar(const Rounds&);
    };
}}
