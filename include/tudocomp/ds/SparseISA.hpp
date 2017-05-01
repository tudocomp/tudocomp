#pragma once

#include <vector>
#include <array>
#include <tuple>
#include <algorithm>
#include <stdlib.h>

#include <sdsl/bit_vectors.hpp>
#include <sdsl/rank_support.hpp>

namespace tdc {

class SparseISA {
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
        for (int k = 0; k < cycle.size(); k += c) {
            MarksShortcuts[cycle.at(k)] = 1;
            Shortcuts.push_back(cycle.at((cycle.size() - c + k) % cycle.size()));
        }
    }

    void build() {
        std::vector<bool> marks;
        marks.assign(Perm.size(), false);
        for (int i = 0; i < Perm.size(); i++) {
            if (marks.at(i) == false) {
                computeCycle(i, marks);
            }
        }
    }

public:
    SparseISA(std::vector<int> const vec, const int c) : Perm{}, MarksShortcuts{vec.size(), 0}, Shortcuts{}, c{c} {
        Perm.assign(vec.begin(), vec.end());
        build();
    }

    SparseISA(int const arr[], std::size_t const size, int const c)
            : Perm{}, MarksShortcuts{size, 0}, Shortcuts{}, c{c} {
        for (int i = 0; i < size; i++) {
            Perm.push_back(arr[i]);
        }
        build();
    }

    template<std::size_t const size>
    SparseISA(std::array<int, size> const arr, int const c)
            : Perm{}, MarksShortcuts{arr.size(), 0}, Shortcuts{}, c{c} {
        Perm.assign(arr.begin(), arr.end());
        build();
    }

    virtual ~SparseISA();

    inline int getPower(int const i, int const k) const {
        int perm = Perm.at(i);
        for (int j = 1; j < k; j++) {
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

};

} //ns
