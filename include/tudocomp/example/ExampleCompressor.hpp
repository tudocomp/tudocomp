// [/include/tudocomp/example/ExampleCompressor.hpp]

#ifndef _INCLUDED_EXAMPLE_COMPRESSOR_HPP_
#define _INCLUDED_EXAMPLE_COMPRESSOR_HPP_

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/io.hpp>
#include <chrono>
#include <thread>

namespace tdc {

class ExampleCompressor: public Compressor {
private:
    const char m_escape_symbol;
public:
    inline static Meta meta() {
        Meta m("compressor", "example_compressor",
               "This is a example compressor.");
        m.option("escape_symbol").dynamic("%");
        m.option("debug_sleep").dynamic("false");
        return m;
    }

    inline ExampleCompressor(Env&& env):
        Compressor(std::move(env)),
        m_escape_symbol(this->env().option("escape_symbol").as_string()[0])
    {
        // ...
    }

    inline virtual void compress(Input& input, Output& output) override {
        env().begin_stat_phase("init");

        auto istream = input.as_stream();
        auto ostream = output.as_stream();

        char last;
        char current;
        size_t counter = 0;

        size_t stat_max_repeat_counter = 0;
        size_t stat_count_repeat_segments = 0;
        bool opt_enable_sleep = env().option("debug_sleep").as_bool();

        // Use a lambda here to keep the code DRY
        auto emit_run = [&]() {
            if (counter > 3) {
                // Only replace if we actually get shorter
                ostream << last << m_escape_symbol << counter << m_escape_symbol;
                stat_count_repeat_segments++;
            } else {
                // Else output chars as normal
                for (size_t i = 0; i <= counter; i++) {
                    ostream << last;
                }
            }
            stat_max_repeat_counter = std::max(stat_max_repeat_counter,
                                               counter);
        };

        // FIXME, cheat needed to add a small delay so you can see the stats diagram
        if (opt_enable_sleep) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        env().end_stat_phase();
        env().begin_stat_phase("run length encoding");

        if (istream.get(last)) {
            while(istream.get(current)) {
                if (current == last) {
                    counter++;
                } else {
                    // Emit run length encoding here
                    emit_run();
                    // Then continue with the next character
                    last = current;
                    counter = 0;
                }
            }
            // Don't forget trailing chars
            emit_run();
        }

        env().log_stat("max repeat", stat_max_repeat_counter);
        env().log_stat("count repeat segments", stat_count_repeat_segments);

        // FIXME, cheat needed to add a small delay so you can see the stats diagram
        if (opt_enable_sleep) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            // FIXME, adding a fake memory allocation
            std::vector<uint8_t> v(100 * 1024, 42); // 100 KiB of the byte 42
        }
        env().end_stat_phase();

        env().begin_stat_phase("nothing");
        // FIXME, cheat needed to add a small delay so you can see the stats diagram
        if (opt_enable_sleep) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        env().end_stat_phase();
    }

    inline virtual void decompress(Input& input, Output& output) override {
        // Use the input as a memory view here, just to have both variants used
        auto iview = input.as_view();
        auto ostream = output.as_stream();

        char last = '?';

        for (size_t i = 0; i < iview.size(); i++) {
            if (iview[i] == m_escape_symbol) {
                size_t counter = 0;
                for (i++; i < iview.size(); i++) {
                    if (iview[i] == m_escape_symbol) {
                        break;
                    } else {
                        counter *= 10;
                        counter += (iview[i] - '0');
                    }
                }
                for (size_t x = 0; x < counter; x++) {
                    ostream << last;
                }
            } else {
                ostream << iview[i];
                last = iview[i];
            }
        }
    }

};


template<typename T>
class TemplatedExampleCompressor: public Compressor {
private:
    // We want to keep a instance of the encoder
    // alive for the same duration as the compressor itself, so
    // keep it as a field
    T m_encoder;
public:
    inline static Meta meta() {
        Meta m("compressor", "templated_example_compressor",
               "This is a templated example compressor.");
        m.option("encoder").templated<T>();
        m.option("debug_sleep").dynamic("false");
        return m;
    }

    inline TemplatedExampleCompressor(Env&& env):
        Compressor(std::move(env)),
        // Initialize it with an `Env` for all its options
        m_encoder(this->env().env_for_option("encoder")) {}

    inline virtual void compress(Input& input, Output& output) override {
        auto istream = input.as_stream();
        auto ostream = output.as_stream();

        char last;
        char current;
        size_t counter = 0;

        size_t stat_max_repeat_counter = 0;
        size_t stat_count_repeat_segments = 0;
        bool opt_enable_sleep = env().option("debug_sleep").as_bool();

        auto emit_run = [&]() {
            if (counter > m_encoder.threshold()) {
                // Delegate encoding to encoder
                m_encoder.encode_repeat(last, counter, ostream);

                stat_count_repeat_segments++;
            } else {
                // Else output chars as normal
                for (size_t i = 0; i <= counter; i++) {
                    ostream << last;
                }
            }
            stat_max_repeat_counter = std::max(stat_max_repeat_counter,
                                               counter);
        };

        env().begin_stat_phase("run length encoding");

        if (istream.get(last)) {
            while(istream.get(current)) {
                if (current == last) {
                    counter++;
                } else {
                    // Emit run length encoding here
                    emit_run();
                    // Then continue with the next character
                    last = current;
                    counter = 0;
                }
            }
            // Don't forget trailing chars
            emit_run();
        }

        env().log_stat("max repeat", stat_max_repeat_counter);
        env().log_stat("count repeat segments", stat_count_repeat_segments);

        if (opt_enable_sleep) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            // FIXME, adding a fake memory allocation
            std::vector<uint8_t> v(100 * 1024, 42); // 100 KiB of the byte 42
        }
        env().end_stat_phase();
    }

    inline virtual void decompress(Input& input, Output& output) override {
        // Use the input as a memory view here, just to have both variants used
        auto iview = input.as_view();
        auto ostream = output.as_stream();

        char last = '?';

        for (size_t i = 0; i < iview.size(); i++) {
            if (m_encoder.is_start_of_encoding(iview, i)) {
                size_t counter = m_encoder.decode(iview, i);
                for (size_t x = 0; x < counter; x++) {
                    ostream << last;
                }
            } else {
                ostream << iview[i];
                last = iview[i];
            }
        }
    }
};

class ExampleDebugCoder: public Algorithm {
private:
    const char m_escape_symbol;
public:
    inline ExampleDebugCoder(Env&& env):
        Algorithm(std::move(env)),
        m_escape_symbol(this->env().option("escape_symbol").as_string()[0]) {}

    inline static Meta meta() {
        Meta m("example_coder", "debug",
               "This is a example debug coder, encoding human readable.");
        m.option("escape_symbol").dynamic("%");
        return m;
    }

    inline void encode_repeat(char last, size_t repeat, io::OutputStream& ostream) {
        ostream << last << m_escape_symbol << repeat << m_escape_symbol;
    }

    inline bool is_start_of_encoding(const io::InputView& iview, size_t i) {
        return iview[i] == m_escape_symbol;
    }

    inline size_t decode(const io::InputView& iview, size_t& i) {
        size_t counter = 0;
        for (i++; i < iview.size(); i++) {
            if (iview[i] == m_escape_symbol) {
                break;
            } else {
                counter *= 10;
                counter += (iview[i] - '0');
            }
        }
        return counter;
    }

    inline size_t threshold() { return 3; }
};

class ExampleBitCoder: public Algorithm {
private:
    const char m_escape_symbol;
public:
    inline ExampleBitCoder(Env&& env):
        Algorithm(std::move(env)),
        m_escape_symbol(this->env().option("escape_byte").as_integer()) {}

    inline static Meta meta() {
        Meta m("example_coder", "bit",
               "This is a example bit coder, encoding as a binary integer.");
        m.option("escape_byte").dynamic("255");
        return m;
    }

    inline void encode_repeat(char last, size_t repeat, io::OutputStream& ostream) {
        ostream << last << m_escape_symbol << char(uint8_t(repeat));
    }

    inline bool is_start_of_encoding(const io::InputView& iview, size_t i) {
        return iview[i] == uint8_t(m_escape_symbol);
    }

    inline size_t decode(const io::InputView& iview, size_t& i) {
        i++;
        size_t counter = uint8_t(iview[i]);
        return counter;
    }

    inline size_t threshold() { return 2; }
};

}

#endif
