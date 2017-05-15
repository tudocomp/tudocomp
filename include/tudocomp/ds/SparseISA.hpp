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
template<typename textds_t>
class SparseISA : public ArrayDS {
private:
    sdsl::bit_vector MarksShortcuts;
    std::vector<int> Shortcuts;
    int c;
    const int n;
    CompressMode cm;
    textds_t &SA;

    const std::vector<int> getSA() const {
        std::vector<int> result;
        const auto &sa = SA.require_sa(cm);
        for(int i = 0; i < n; i++) {
            result.push_back(sa[i]);
        }
        return result;
    }

    void computeCycle(int i, std::vector<bool> &marks) {
        std::vector<int> cycle, sa{getSA()};
        cycle.push_back(i);
        int perm = sa[i];
        while (perm != i) {
            cycle.push_back(perm);
            marks.at(perm) = true;
            perm = sa[perm];
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

    SparseISA(Env&& env, textds_t& t, CompressMode cm) : ArrayDS(std::move(env)), n(t.size()), cm(cm), SA(t) {

         this->env().begin_stat_phase("Construct ISA");

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
protected:

    inline int getInv(const size_t i) const {
        std::vector<int> sa{getSA()};
        int perm = sa[i], fperm = i;
        bool tookShortcut = false;
        sdsl::rank_support_v<> rankStructure{&MarksShortcuts};
        while (perm != i) {
            if (MarksShortcuts[fperm] == 1 && !tookShortcut) {
                fperm = Shortcuts.at(rankStructure.rank(fperm));
                tookShortcut = true;
            } else {
                fperm = perm;
            }
            perm = sa[fperm];
        }
        return fperm;
    }

    inline void compress() {
        Shortcuts.shrink_to_fit();

        env().begin_stat_phase("Compress ISA");

        //env().log_stat("bit_width", size_t(m_data->width()));
        //env().log_stat("size", m_data->bit_size() / 8);
        env().end_stat_phase();
    }

};

} // namespace tdc
