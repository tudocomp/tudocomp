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
                if (tdc_likely(m_decode_literal_factor.len)) {
                    m_resolved.push_back(m_decode_literal_factor);
                }

                bool done = false;
                do {
                    done = process_round();
                } while(!done);

            }

            void decode_eagerly() {
            }

            PointerJump(PointerJump&& other) = default;

            inline PointerJump(Env&& env, len_t size)
                    : Algorithm(std::move(env))
                    , m_cursor(0)
                    , m_buffer(size)
            {}


        // decoding statge
            inline void decode_literal(uliteral_t c) {
                m_buffer[m_cursor] = c;

                if (tdc_unlikely(m_decode_literal_factor.len == 0)) {
                    m_decode_literal_factor.source = m_cursor;
                }

                m_decode_literal_factor.len++;
                m_cursor++;

                // we assume that the text to restore does not contain a NULL-byte but at its very end
                DCHECK(c != 0 || m_cursor == m_buffer.size());
            }

            inline void decode_factor(const len_t source_position, const len_t factor_length) {
                if (tdc_likely(m_decode_literal_factor.len)) {
                    m_resolved.push_back(m_decode_literal_factor);
                    m_decode_literal_factor.len = 0;
                }

                m_unresolved.emplace_back(source_position, m_cursor, factor_length);
                m_new_requests.emplace_back(source_position, m_cursor, factor_length);

                m_cursor += factor_length;
            }

        // final touch
            IF_STATS(
            inline len_t longest_chain() const {
                // We would need additional data structures to compute it. so we output a dummy value.
                // if you need it, use a different decompressor
                return 0;
            })


            inline void write_to(std::ostream& out) const {
                for(auto c : m_buffer) out << c;
            }


        private:
            // literals and insertion
            len_t m_cursor;
            IntVector<uliteral_t> m_buffer;

            // request handling
            struct Factor {
                len_t source;
                len_t len;
                IF_DEBUG(len_t target);

                Factor() {}
                Factor(len_t source, len_t len) : source(source), len(len) {}
                Factor(len_t source, len_t target, len_t len)
                        : source(source), len(len)
                {
                    IF_DEBUG(this->target = target);
                }

                Factor(const Factor&) = default;

                bool operator< (const Factor& o) const {
                    return std::tie(source, len) < std::tie(o.source, o.len);
                }
            };

            struct Request {
                len_t source;
                len_t target;
                len_t len;

                Request() {}
                Request(len_t source, len_t target, len_t len)
                        : source(source), target(target), len(len)
                {}
                Request(const Request&) = default;

                bool operator< (const Request& o) const {
                    return std::tie(source, target, len) < std::tie(o.source, o.target, o.len);
                }
            };

            std::vector<Factor> m_resolved;
            std::vector<Factor> m_unresolved;
            std::vector<Request> m_requests;
            std::vector<Request> m_new_requests;

            Factor m_decode_literal_factor{0, 0};

            void check_invariants() const {
                DCHECK(std::is_sorted(m_resolved.cbegin(), m_resolved.cend()));
                DCHECK(std::is_sorted(m_requests.cbegin(), m_requests.cend()));

                // Ensure that each character is exactly resolved or unresolved
                {
                    len_t cursor = 0;
                    auto iter_un = m_unresolved.cbegin();
                    auto iter_res = m_resolved.cbegin();

                    while(cursor < m_buffer.size()) {
                        if (iter_res != m_resolved.cend() && iter_res->source == cursor) {
                            // cursor points to unresolved
                            cursor += iter_res->len;
                            iter_res++;
                            continue;
                        }

                        DCHECK(iter_un != m_unresolved.cend());
                        DCHECK_EQ(iter_un->target, cursor);
                        cursor += iter_un->len;
                        iter_un++;
                    }

                    DCHECK_EQ(cursor, m_buffer.size());
                }
            }

            bool process_round() {
                // sort and exchange requests
                std::sort(m_new_requests.begin(), m_new_requests.end());
                m_requests.swap(m_new_requests);
                m_new_requests.clear();

                IF_DEBUG(check_invariants());

                std::vector<Factor> new_resolved;

            }
        };

    }} //ns

