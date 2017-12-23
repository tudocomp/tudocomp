#pragma once

//#include <tudocomp/compressors/esp/>

namespace tdc {namespace esp {
    // TODO: Change to take iterator-like
    template<class T>
    bool check_landmarks(const T& t, bool allow_long = false) {
        if (allow_long) return true;
        size_t last = 0;
        size_t i = 0;
        for(; i < t.size(); i++) {
            if (t[i] == 1u) {
                if (i > 1) return false;
                last = i;
                i++;
                break;
            }
        }
        for(; i < t.size(); i++) {
            if (t[i] == 1u) {
                if ((i - last) > 3 || (i - last) < 2) return false;
                last = i;
            }
        }
        return true;
    }

    template<typename LmPred, typename SpanPush>
    void landmark_spanner(size_t size,
                          LmPred pred,
                          SpanPush push,
                          bool tie_to_right = true) {
        struct Block {
            size_t left;
            size_t right;
            inline size_t size() { return right - left + 1; }
        };
        std::array<Block, 2> blocks {{
            {0, 0},
            {0, 0},
        }};
        uint8_t bi = 0;

        for(size_t i = 0; i < size; i++) {
            if (pred(i)) {
                blocks[1].left  = (i == 0)        ? (i) : (i - 1);
                blocks[1].right = (i == size - 1) ? (i) : (i + 1);
                /*std::cout <<
                    "i: " << i << ", "
                    "old: (" << left << ", " << right << "), "
                    "new: (" << new_left << ", " << new_right << ")\n";*/

                if (bi > 0) {
                    // Adjust overlap of adjacent landmarks
                    if (blocks[1].left == blocks[0].right) {
                        if (tie_to_right) {
                            blocks[0].right--;
                        } else {
                            blocks[1].left++;
                        }
                    }
                }

                if (bi == 0) {
                    bi = 1;
                } else {
                    push(blocks[0].left, blocks[0].right);
                }
                for (size_t j = 0; j < 1; j++) {
                    blocks[j] = blocks[j + 1];
                }
            }
        }
        if (bi == 1) {
            // entered at least once, one unused block
            push(blocks[1].left, blocks[1].right);
        }
    }
}}
