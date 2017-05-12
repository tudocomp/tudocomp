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
    std::vector<int> Perm;
    sdsl::bit_vector MarksShortcuts;
    std::vector<int> Shortcuts;
    int c;

    void computeCycle(int i, std::vector<bool> &marks) {
        std::vector<int> cycle;
        cycle.push_back(i);
        int perm = Perm.at(i);
        while (perm != i) {
            cycle.push_back(perm);
            marks.at(perm) = true;
            perm = Perm.at(perm);
        }
        for (unsigned int k = 0; k < cycle.size(); k += c) {
            MarksShortcuts[cycle.at(k)] = 1;
            Shortcuts.push_back(cycle.at((cycle.size() - c + k) % cycle.size()));
        }
    }

    void build() {
        Shortcuts.clear();
        this->MarksShortcuts = sdsl::bit_vector{Perm.size(), 0};
        std::vector<bool> marks;
        marks.assign(Perm.size(), false);
        for (unsigned int i = 0; i < Perm.size(); i++) {
            if (marks.at(i) == false) {
                computeCycle(i, marks);
            }
        }
    }

protected:
    using iv_t = DynamicIntVector;

public:

    using data_type = iv_t;

    inline static Meta meta() {
        Meta m("sparseisa", "sparseisa implementation");
        return m;
    }

    template<typename textds_t>
    SparseISA(Env&& env, textds_t& t, CompressMode cm) : ArrayDS(std::move(env)){

         // Require Suffix Array
         auto& sa = t.require_sa(cm);

         this->env().begin_stat_phase("Construct ISA");

         // Allocate
         const size_t n = t.size();
         const int c = n/3;
         this->c = int(c);

         for (unsigned int i = 0; i < n; i++) {
             Perm.push_back(sa[i]);
         }
         build();
     }

    inline void setC(const int c) {
        this->c = c;
        build();
    }

    inline int getPower(int const i, unsigned int const k) const {
        int perm = Perm.at(i);
        for (unsigned int j = 1; j < k; j++) {
            perm = Perm.at(perm);
        }
        return perm;
    }

    inline int getInv(int const i) const {
        int perm = Perm.at(i), fperm = i;
        bool tookShortcut = false;
        sdsl::rank_support_v<> rankStructure{&MarksShortcuts};
        while (perm != i) {
            if (MarksShortcuts[fperm] == 1 && !tookShortcut) {
                fperm = Shortcuts.at(rankStructure.rank(fperm));
                tookShortcut = true;
            } else {
                fperm = perm;
            }
            perm = Perm.at(fperm);
        }
        return fperm;
    }

    inline std::vector<int> getPerm() const {
        return Perm;
    }

    inline void compress() {
        Perm.shrink_to_fit();
        Shortcuts.shrink_to_fit();
    }

};

} // namespace tdc
