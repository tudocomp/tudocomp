#ifndef _INCLUDED_DECODE_BUFFER_HPP_
#define _INCLUDED_DECODE_BUFFER_HPP_

#include <iostream>
#include <vector>

#include <boost/variant.hpp>
#include <sdsl/int_vector.hpp>

#include <glog/logging.h>

#include <tudocomp/util/DCBStrategyNone.hpp>
#include <tudocomp/util/DCBStrategyMap.hpp>
#include <tudocomp/util/DCBStrategyRetargetArray.hpp>

namespace tudocomp {

using DecodeCallbackStrategy = boost::variant<DCBStrategyNone, DCBStrategyMap, DCBStrategyRetargetArray>;

class DecodeBuffer {
    
private:
    DecodeCallbackStrategy m_cb_strategy;
    
    size_t m_len;
    sdsl::int_vector<8> m_text;
    sdsl::bit_vector m_decoded;
    
    size_t m_pos;
    
    struct visitor_next_waiting_for : public boost::static_visitor<bool> {
        size_t pos;
        size_t* out_waiting;
        
        visitor_next_waiting_for(size_t _pos, size_t& _out_waiting)
            : pos(_pos), out_waiting(&_out_waiting) {}
        
        inline bool operator()(DCBStrategyNone& cb) const {
            return cb.next_waiting_for(pos, *out_waiting);
        }
        
        inline bool operator()(DCBStrategyMap& cb) const {
            return cb.next_waiting_for(pos, *out_waiting);
        }
        
        inline bool operator()(DCBStrategyRetargetArray& cb) const {
            return cb.next_waiting_for(pos, *out_waiting);
        }
    };
    
    inline bool next_waiting_for(size_t pos, size_t& out_waiting) {
        return boost::apply_visitor(visitor_next_waiting_for(pos, out_waiting), m_cb_strategy);
    }
    
    struct visitor_wait : public boost::static_visitor<void> {
        size_t pos, src;
        
        visitor_wait(size_t _pos, size_t _src)
            : pos(_pos), src(_src) {}

        inline void operator()(DCBStrategyNone& cb) const {
            return cb.wait(pos, src);
        }
        
        inline void operator()(DCBStrategyMap& cb) const {
            return cb.wait(pos, src);
        }
        
        inline void operator()(DCBStrategyRetargetArray& cb) const {
            return cb.wait(pos, src);
        }
    };
    
    inline void wait(size_t pos, size_t src) {
        return boost::apply_visitor(visitor_wait(pos, src), m_cb_strategy);
    }
    
public:
    DecodeBuffer(size_t len, DecodeCallbackStrategy cb_strategy)
        : m_cb_strategy(cb_strategy), m_len(len), m_pos(0) {

        m_text = sdsl::int_vector<8>(len, 0);
        m_decoded = sdsl::bit_vector(len, 0);
    }
    
    inline void decode(size_t pos, uint8_t sym) {
        assert(pos < m_len);
        
        m_text[pos] = sym;
        m_decoded[pos] = 1;
        
        size_t waiting_pos;
        while(next_waiting_for(pos, waiting_pos)) {
            decode(waiting_pos, sym); //recursion!
        }
    }
    
    inline void decode(uint8_t sym) {
        decode(m_pos, sym);
        ++m_pos;
    }
    
    inline void defact(size_t pos, size_t src, size_t num) {
        assert(pos + num <= m_len);
        
        while(num) {
            if(m_decoded[src]) {
                decode(pos, m_text[src]);
            } else {
                wait(pos, src);
            }
            
            ++pos;
            ++src;
            --num;
        }
    }
    
    inline void defact(size_t src, size_t num) {
        defact(m_pos, src, num);
        m_pos += num;
    }
    
    /// Write the current state of the decode buffer into an ostream.
    inline void write_to(std::ostream& out) const {
        out.write((const char*)m_text.data(), m_text.size());
    }

    /// Return the expected size of the decoded text.
    inline size_t size() const {
        return m_text.size();
    }
};

}

#endif
