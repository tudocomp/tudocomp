#pragma once

#include <vector>
#include <sdsl/int_vector.hpp>
#include <sdsl/rank_support.hpp>
#include <tudocomp/def.hpp>
#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/Algorithm.hpp>
#include <algorithm>

#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {
    namespace lcpcomp {

        /**
         * Runs a number of scans of the factors.
         * In each scan, it tries to decode all factors.
         * Factors that got fully decoded are dropped.
         */
        class PointerJump : public Algorithm {
        public:
            inline static Meta meta() {
                Meta m("lcpcomp_dec", "pj");
                return m;
            }

            inline void decode_lazy() {
                len_t rounds = 0;
                len_t positions_completed = 0;

                m_sources_temp.resize(m_buffer.size());

                if (0) {
                    while (true) {
                        positions_completed = pointer_jump_round();
                        rounds++;

                        if (positions_completed == m_buffer.size()) {
                            break;
                        }

//                    if (positions_completed >= m_buffer.size() * 0.9) {
//                        exhaustive_jumps();
//                    }
                    };

                    //exhaustive_jumps();

                    StatPhase::log("PJ rounds", rounds);
                }

                exhaustive_jumps();

                m_sources_temp.clear();
                m_sources_temp.shrink_to_fit();
            }

            void decode_eagerly() {
                #pragma omp parallel for
                for(len_t i=0; i < m_buffer.size(); i++) {
                    m_buffer[i] = m_buffer[m_sources[i]];
                }
            }

        private:
            inline len_t pointer_jump_round() {
                len_t positions_completed = 0;

                #pragma omp parallel for reduction(+:positions_completed)
                for(len_t i=0; i < m_sources.size(); i++) {
                    const auto parent = m_sources[i];

                    m_sources_temp[i] = m_sources[parent];
                    positions_completed += (m_sources[parent] == parent);
                }

                std::swap(m_sources, m_sources_temp);

                StatPhase::log("Positions completed", positions_completed);

                return positions_completed;
            }

            inline void exhaustive_jumps() {
                #pragma omp parallel for
                for(len_t i=0; i < m_sources.size(); i++) {
                    len_t parent = i;

                    do {
                        parent = m_sources[parent];
                    } while (parent != m_sources[parent]);

                    m_sources_temp[i] = parent;
                }

                std::swap(m_sources, m_sources_temp);
            }

            len_t m_cursor;

            IntVector<uliteral_t> m_buffer;

            //storing factors
            const len_t source_complete = std::numeric_limits<len_t>::max();
            using SourceVector = std::vector<len_t>;

            SourceVector m_sources;
            SourceVector m_sources_temp;

            IF_STATS(len_t m_longest_chain = 0);

        public:
            PointerJump(PointerJump&& other) = default;

            inline PointerJump(Env&& env)
                    : Algorithm(std::move(env))
                    , m_cursor(0)
            {}

            inline void initialize(size_t n) {
                if(tdc_unlikely(n == 0)) throw std::runtime_error(
                    "no text length provided");

                m_buffer = IntVector<uliteral_t>(n);
                m_sources = SourceVector(n);
            }

            inline void decode_literal(uliteral_t c) {
                m_buffer[m_cursor] = c;
                m_sources[m_cursor] = m_cursor;
                m_cursor++;

                DCHECK(c != 0 || m_cursor == m_buffer.size()); // we assume that the text to restore does not contain a NULL-byte but at its very end
            }

            inline void decode_factor(const len_t source_position, const len_t factor_length) {
                for(len_t i = 0; i < factor_length; ++i, ++m_cursor) {
                    const len_t src_pos = source_position+i;
                    m_sources[m_cursor] = src_pos;
                }
            }

            inline void process() {
            }

            inline len_t longest_chain() const {
                // We would need additional data structures to compute it. so we output a dummy value.
                // if you need it, use a different decompressor
                return 0;
            }

            inline void write_to(std::ostream& out) const {
                for(auto c : m_buffer) out << c;
            }
        };

    }} //ns

