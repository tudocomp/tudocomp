#pragma once

//#define PAR_SORT
#define INT_SORT

#include <vector>
#include <algorithm>


#ifdef PAR_SORT
 #include <parallel/algorithm>
 #define PARSEQ __gnu_parallel
#else
 #define PARSEQ std
#endif

#ifdef INT_SORT
 #include <tudocomp/util/IntSort.hpp>
#endif

#include <tudocomp/def.hpp>
#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/Algorithm.hpp>

#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {
    namespace lcpcomp {
        namespace PointerJumpIntEM_interal {
            struct Factor {
                len_t source;
                len_t len;
#ifdef DEBUG
                len_t target;
#endif

                Factor() {}
                constexpr Factor(len_t source, len_t target, len_t len)
                        : source(source), len(len)
#ifdef DEBUG
                        , target(target)
#endif
                {}
                constexpr Factor(len_t source, len_t len) : Factor(source, 0, len) {}

                Factor(const Factor&) = default;

                bool operator< (const Factor& o) const {return source < o.source ; }//   as_tuple() < o.as_tuple();}
                bool operator== (const Factor& o) const {return  as_tuple() == o.as_tuple();}

                friend std::ostream& operator<<(std::ostream& o, const Factor& f) {
                    o << "[src=" << f.source << ", len=" << f.len
#ifdef DEBUG
                    << ", dbg-target=" << f.target
#endif
                    << "]";

                    return o;
                }

                constexpr static Factor max_sentinel() {
                    return {
                            std::numeric_limits<decltype(Factor().source)>::max(),
                            std::numeric_limits<decltype(Factor().len)>::max()
                    };
                }

            private:
                std::tuple<const len_t&, const len_t&>
                as_tuple() const { return std::tie(source, len); }
            };

            struct Request {
                len_t source;
                len_t target;
                len_t len;

                Request() {}
                constexpr Request(len_t source, len_t target, len_t len)
                        : source(source), target(target), len(len)
                {}
                Request(const Request&) = default;
                Request(const Factor& f) : Request(f.source, 0, f.len) {}

                bool operator< (const Request& o) const {return source < o.source; } //as_tuple() < o.as_tuple();}
                bool operator== (const Request& o) const {return as_tuple() == o.as_tuple();}

                constexpr static Request max_sentinel() {
                    return {
                            std::numeric_limits<decltype(Request().source)>::max(),
                            std::numeric_limits<decltype(Request().target)>::max(),
                            std::numeric_limits<decltype(Request().len)>::max()
                    };
                }

            private:
                std::tuple<const len_t&, const len_t&, const len_t&>
                as_tuple() const { return std::tie(source, target, len); }
            };


            template<typename IterResolved, typename IterUnresolved>
            class FactorStreamMergerImpl {
            public:
                FactorStreamMergerImpl() = delete;
                FactorStreamMergerImpl(IterResolved res_begin,  IterResolved res_end,
                                       IterUnresolved un_begin, IterUnresolved un_end,
                                       len_t size)
                        : m_res_it(res_begin), m_res_end(res_end),
                          m_un_it(un_begin), m_un_end(un_end),
                          m_cursor(0), m_size(size)
                {
                    next();
                }

                FactorStreamMergerImpl(const FactorStreamMergerImpl&) = default;

                const Request& current() const {
                    return m_current;
                }

                const bool resolved() const {
                    return m_is_resolved;
                }

                const bool empty() const {
                    const bool empty = (m_size == m_cursor);

                    DCHECK(!empty || (std::next(m_un_it) == m_un_end));
                    DCHECK(!empty || (std::next(m_res_it) == m_res_end));

                    return empty;
                }

                const len_t cursor_begin() const {
                    return m_begin;
                }

                const len_t cursor_end() const {
                    return m_cursor;
                }

                void next() {
                    //DCHECK(!empty()); // implied by next assertion
                    DCHECK_GT(m_size, m_cursor);

                    DCHECK(m_res_it != m_res_end);
                    DCHECK(m_un_it != m_un_end);

                    m_is_resolved = (m_cursor == m_res_it->source);

                    if (m_is_resolved) {
                        m_current = *m_res_it;

                        DCHECK_EQ(m_cursor, m_current.source);
                        ++m_res_it;
                    } else {
                        m_current = *m_un_it;

                        IF_DEBUG(DCHECK_EQ(m_cursor, m_current.target));
                        ++m_un_it;
                    }

                    DCHECK(m_current.len);
                    m_begin = m_cursor;
                    m_cursor += m_current.len;
                }

            private:
                IterResolved m_res_it;
                const IterResolved m_res_end;

                IterUnresolved m_un_it;
                const IterUnresolved m_un_end;

                len_t m_cursor;
                len_t m_begin;
                const len_t m_size;

                Request m_current;
                bool m_is_resolved;
            };

            using FactorVector = std::vector<Factor>;
            using RequestVector = std::vector<Request>;
            using FactorStreamMerger = FactorStreamMergerImpl<FactorVector::const_iterator, RequestVector::const_iterator>;

            /**
             * Runs a number of scans of the factors.
             * In each scan, it tries to decode all factors.
             * Factors that got fully decoded are dropped.
             */
            class PointerJumpIntEM : public Algorithm {

            public:
                inline static Meta meta() {
                    Meta m("lcpcomp_dec", "pjintem");
                    return m;
                }

                inline void decode_lazy() {
                    if (tdc_likely(m_decode_literal_factor.len)) {
                        m_resolved.push_back(m_decode_literal_factor);
                    }

                    // Put sentinels in factor streams to make merging easier
                    m_unresolved.push_back(Request::max_sentinel());
                    m_resolved.push_back(Factor::max_sentinel());

                    bool done = false;
                    do {
                        StatPhase::wrap("Round", [&] () {
                            std::cout << "Round" << std::endl;
                            done = process_round();
                        });
                    } while(!done);

                }

                void decode_eagerly() {
                }

                PointerJumpIntEM(PointerJumpIntEM&& other) = default;

                inline PointerJumpIntEM(Env&& env)
                        : Algorithm(std::move(env))
                        , m_cursor(0)
                {}

                inline void initialize(size_t n) {
                    if(tdc_unlikely(n == 0)) throw std::runtime_error(
                        "no text length provided");

                    m_buffer = IntVector<uliteral_t>(n);
                }

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

                inline void decode_factor(const len_t source_position, len_t factor_length) {
                    if (tdc_likely(m_decode_literal_factor.len)) {
                        m_resolved.push_back(m_decode_literal_factor);
                        m_decode_literal_factor.len = 0;
                    }

                    DCHECK_NE(m_cursor, source_position);
                    DCHECK(factor_length);

                    auto add = [&] (len_t source, len_t target, len_t len, bool print = false) {
                        if (print)
                            std::cout << " [" << target << ".." << (target+len) << " <- " << source << ".." << (source+len) << "]\n";

                        m_unresolved.emplace_back(source, target, len);
                        m_new_requests.emplace_back(source, target, len);
                        m_cursor += len;
                        factor_length -= len;
                    };


                    const bool overlap_at_end = (m_cursor < source_position) && (source_position < m_cursor + factor_length);
                    if (false && tdc_unlikely(overlap_at_end)) {
                        const len_t ov_len = source_position - m_cursor;
                        const len_t source_end = source_position + factor_length;

                        const len_t first_factor = factor_length % ov_len;
                        if (first_factor) {
                            add(source_end - first_factor, m_cursor, first_factor);
                        }

                        while(factor_length) {
                            add(source_end - ov_len, m_cursor, ov_len);
                        }

                    } else {
                        add(source_position, m_cursor, factor_length);
                    }

                }

            // final touch
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


            private:
                // literals and insertion
                len_t m_cursor;
                IntVector<uliteral_t> m_buffer;


                FactorVector m_resolved;
                RequestVector m_unresolved;

                RequestVector m_requests;
                RequestVector m_new_requests;

                Factor m_decode_literal_factor{0, 0};

                void check_invariants() const {
                    // Resolved
                    DCHECK(!m_resolved.empty());
                    DCHECK(m_resolved.back() == Factor::max_sentinel());
                    DCHECK(std::is_sorted(m_resolved.cbegin(), m_resolved.cend()));

                    // Unresolved
                    DCHECK(!m_unresolved.empty());
                    DCHECK(m_unresolved.back() == Request::max_sentinel());

                    // Check full coverage
                    const len_t resolved = std::accumulate(
                        m_resolved.cbegin(), m_resolved.cbegin() + (m_resolved.size() - 1),
                        static_cast<len_t>(0), [] (len_t x, const auto& f) {return x+f.len;}
                    );

                    const len_t unresolved = std::accumulate(
                            m_unresolved.cbegin(), m_unresolved.cbegin() + (m_unresolved.size() - 1),
                            static_cast<len_t>(0), [] (len_t x, const auto& f) {return x+f.len;}
                    );

                    DCHECK_EQ(m_buffer.size(), resolved + unresolved);

                    // Requests
                    DCHECK(std::is_sorted(m_requests.cbegin(), m_requests.cend()));
                }

                bool process_round() {
                    // sort and exchange requests
#ifdef INT_SORT
                    intsort(m_new_requests, [] (const Request& r) {return r.source;}, static_cast<len_t>(m_buffer.size()));
#else
                    PARSEQ::sort(m_new_requests.begin(), m_new_requests.end());
#endif

                    m_requests.swap(m_new_requests);
                    m_new_requests.clear();


                    IF_DEBUG(check_invariants());

                    std::vector<Factor> new_resolved;

                    FactorStreamMerger merger{
                        m_resolved.cbegin(), m_resolved.cend(),
                        m_unresolved.cbegin(), m_unresolved.cend(),
                        static_cast<len_t>(m_buffer.size())
                    };


                    auto process_request =
                        [&new_resolved, &m_new_requests = this->m_new_requests, &m_buffer = this->m_buffer]
                        (Request request, FactorStreamMerger merger /* call by value !! */)
                    {
                        while(true) {
                            // check that we have an overlap with the current input token
                            DCHECK_GE(request.source, merger.cursor_begin());
                            DCHECK_GT(merger.cursor_end(), request.source);

                            const len_t token_starting_before = request.source - merger.cursor_begin();
                            const len_t length = std::min<len_t>(
                                request.len, merger.current().len - token_starting_before);
                            DCHECK(length);

                            if (merger.resolved()) {
                                // we found an already resolved piece, so copy the data

                                new_resolved.emplace_back( request.target, length );

                                if (true) {
                                    // copy data (unfortunately this triggers unstructured I/Os); easy to replace in EM
                                    std::copy(
                                            std::next(m_buffer.cbegin(), request.source),
                                            std::next(m_buffer.cbegin(), request.source + length),
                                            std::next(m_buffer.begin(), request.target)
                                    );

                                    // make sure we only copy initialised data
                                    for (len_t j = request.source; j < request.source + length; ++j) {
                                        DCHECK(m_buffer[j]);
                                    }
                                }

                            } else {
                                // pointer jumping
                                m_new_requests.emplace_back(merger.current().source + token_starting_before,
                                                            request.target, length);
                            }

                            // remove process snippet from request by moving the request to the right
                            // and shortening it accordingly
                            request.target += length;
                            request.source += length;
                            request.len -= length;

                            if (!request.len) break;

                            merger.next();
                        }
                    };

                    for(const auto& request : m_requests) {
                        // skip all irrelevant tokens
                        while(merger.cursor_end() <= request.source) {
                            merger.next();
                        }

                        process_request(request, merger);
                    }

                    // new requests -> new unresolved
                    StatPhase::log("m_unresolved.size", m_unresolved.size() - 1);
                    m_unresolved.clear();
                    m_unresolved.reserve(m_new_requests.size() + 1);
                    m_unresolved.assign(m_new_requests.cbegin(), m_new_requests.cend());
#ifdef INT_SORT
                    intsort(m_unresolved, [] (const Request& r) {return r.target;}, static_cast<len_t>(m_buffer.size()));
#else
                    PARSEQ::sort(m_unresolved.begin(), m_unresolved.end(),
                         [] (const Request& a, const Request& b) {return a.target < b.target;});
#endif
                    StatPhase::log("m_new_unresolved.size", m_unresolved.size());
                    m_unresolved.push_back(Request::max_sentinel());

                    // merge resolved vector
                    {
                        std::vector<Factor> resolved_merged;
                        resolved_merged.reserve(new_resolved.size() + m_resolved.size());

#ifdef INT_SORT
                        intsort(new_resolved, [] (const Factor& r) {return r.source;}, static_cast<len_t>(m_buffer.size()));
#else
                        PARSEQ::sort(new_resolved.begin(), new_resolved.end());
#endif
                        new_resolved.emplace_back(Factor::max_sentinel());

                        auto it_old = m_resolved.cbegin();
                        auto it_new = new_resolved.cbegin();

                        resolved_merged.push_back(it_old->source < it_new->source ? *it_old++ : *it_new++);

                        while(resolved_merged.back().source != Request::max_sentinel().source) {
                            const auto token = it_old->source < it_new->source ? *it_old++ : *it_new++;

                            if (resolved_merged.back().source + resolved_merged.back().len == token.source) {
                                resolved_merged.back().len += token.len;
                            } else {
                                resolved_merged.push_back(token);
                            }
                        }

                        StatPhase::log("m_resolved.size", m_resolved.size());
                        StatPhase::log("new_resolved.size", new_resolved.size());
                        StatPhase::log("resolved_merged.size", resolved_merged.size());

                        m_resolved.swap(resolved_merged);
                    }

                    return m_unresolved.size() == 1;
                }
            };
        } // internal namespace

        using PointerJumpIntEM = PointerJumpIntEM_interal::PointerJumpIntEM;
}} //ns


