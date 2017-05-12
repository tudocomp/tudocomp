#pragma once

#include <vector>
#include <array>
#include <tuple>
#include <algorithm>
#include <stdlib.h>

#include <sdsl/bit_vectors.hpp>
#include <sdsl/rank_support.hpp>

#include <tudocomp/ds/CompressMode.hpp>
#include <tudocomp/ds/ArrayDS.hpp>
#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/util.hpp>

namespace tdc {

class SparseISA : public ArrayDS {
private:
    sdsl::bit_vector MarksShortcuts;
    std::vector<int> Shortcuts;
    const size_t n;
    int c;

    void computeCycle(int i, std::vector<bool> &marks) {
        std::vector<int> cycle;
        cycle.push_back(i);
        int perm = (*m_data)[i];
        while (perm != i) {
            cycle.push_back(perm);
            marks.at(perm) = true;
            perm = (*m_data)[perm];
        }
        for (unsigned int k = 0; k < cycle.size(); k += c) {
            MarksShortcuts[cycle.at(k)] = 1;
            Shortcuts.push_back(cycle.at((cycle.size() - c + k) % cycle.size()));
        }
    }

    void build() {
        Shortcuts.clear();
        this->MarksShortcuts = sdsl::bit_vector{n, 0};
        std::vector<bool> marks;
        marks.assign(n, false);
        for (unsigned int i = 0; i < n; i++) {
            if (marks.at(i) == false) {
                computeCycle(i, marks);
            }
        }
    }

public:
    inline static Meta meta() {
        Meta m("isa", "implementation");
        return m;
    }

    template<typename textds_t>
    SparseISA(Env&& env, textds_t& t, CompressMode cm) : ArrayDS(std::move(env)), n(t.size()){

         // Require Suffix Array
         auto& sa = t.require_sa(cm);

         this->env().begin_stat_phase("Construct ISA");

         const size_t w = bits_for(n);
         m_data = std::make_unique<iv_t>(n, 0,
            (cm == CompressMode::compressed) ? w : LEN_BITS);

         // Construct
         for(len_t i = 0; i < n; i++) {
             (*m_data)[i] = sa[i];
         }
         build();

         this->env().log_stat("bit_width", size_t(m_data->width()));
         this->env().log_stat("size", m_data->bit_size() / 8);
         this->env().end_stat_phase();

         if(cm == CompressMode::delayed) compress();
     }

    inline void setC(const int c) {
        this->c = c;
        build();
    }

    inline int getInv(int const i) const {
        int perm = (*m_data)[i], fperm = i;
        bool tookShortcut = false;
        sdsl::rank_support_v<> rankStructure{&MarksShortcuts};
        while (perm != i) {
            if (MarksShortcuts[fperm] == 1 && !tookShortcut) {
                fperm = Shortcuts.at(rankStructure.rank(fperm));
                tookShortcut = true;
            } else {
                fperm = perm;
            }
            perm = (*m_data)[fperm];
        }
        return fperm;
    }

    inline void compress() {
        Shortcuts.shrink_to_fit();
        DCHECK(m_data);

        env().begin_stat_phase("Compress ISA");

        m_data->width(bits_for(m_data->size()));
        m_data->shrink_to_fit();

        env().log_stat("bit_width", size_t(m_data->width()));
        env().log_stat("size", m_data->bit_size() / 8);
        env().end_stat_phase();
    }

};

} // namespace tdc
