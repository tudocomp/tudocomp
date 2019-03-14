#include <gtest/gtest.h>
#include <glog/logging.h>

#include <sstream>
#include <tudocomp/io.hpp>

using namespace tdc;

TEST(bit_io, bits_only) {
    const size_t N = 6'500'000;

    std::stringstream ss;
    {
        Output output(ss);
        BitOStream out(output);

        for(size_t i = 0; i < N; i++) {
            out.write_bit(i % 2);
        }

        ASSERT_EQ(N, out.bits_written());
    }

    auto result = ss.str();
    {
        Input input(result);
        BitIStream in(input);

        for(size_t i = 0; i < N; i++) {
            ASSERT_EQ(i%2, in.read_bit());
        }

        ASSERT_EQ(N, in.bits_read());
    }
}

TEST(bit_io, bits_and_ints) {
    const size_t N = 100'000;

    std::stringstream ss;
    {
        Output output(ss);
        BitOStream out(output);

        for(size_t i = 0; i < N; i++) {
            out.write_bit(i % 2);
            out.write_int(~i, 64);
        }

        ASSERT_EQ(N * 65, out.bits_written());
    }

    auto result = ss.str();
    {
        Input input(result);
        BitIStream in(input);

        for(size_t i = 0; i < N; i++) {
            ASSERT_EQ(i%2, in.read_bit());
            ASSERT_EQ(~i , in.template read_int<size_t>(64)) << "i=" << i;
        }

        ASSERT_EQ(N * 65, in.bits_read());
    }
}

TEST(bit_io, main) {
    std::stringstream ss_out;

    {
        Output output(ss_out);
        BitOStream out(output);
        out.write_bit(0);                   //0
        out.write_bit(1);                   //1
        out.write_int(-1, 2);               //11
        out.write_int(0b11010110, 4);       //0110
        out.write_compressed_int(0x27, 3); //1 111 0 100
        out.write_compressed_int(0x33);    //0 0110011

        ASSERT_EQ(24U, out.bits_written());
    }
    //output should contain 0111 0110 1111 0100 0011 0011 = 76 F4 33

    std::string result = ss_out.str();
    ASSERT_EQ(result.length(), 4U); //24 bits = 3 bytes + terminator byte

    //basic input test
    {
        Input input(result);
        BitIStream in(input);

        ASSERT_EQ(in.read_int<uint32_t>(24), 0x76F433U);
        ASSERT_EQ(24U, in.bits_read());
        ASSERT_TRUE(in.eof());
    }

    //advanced input test
    {
        Input input(result);
        BitIStream in(input);

        ASSERT_EQ(in.read_bit(), 0);
        ASSERT_EQ(in.read_bit(), 1);
        ASSERT_EQ(in.read_int<size_t>(2), 3U);
        ASSERT_EQ(in.read_int<size_t>(4), 6U);
        ASSERT_EQ(in.read_compressed_int<size_t>(3), 0x27U);
        ASSERT_EQ(in.read_compressed_int<size_t>(), 0x33U);
        ASSERT_EQ(24U, in.bits_read());
        ASSERT_TRUE(in.eof());
    }
}

TEST(bit_io, eof) {
    // write i bits to a bit stream, then read the whole
    // bit stream until EOF and ensure exactly i bits have been read
    for(size_t i = 0; i < 100U; i++) {
        // write i bits
        std::string result;
        {
            std::ostringstream ss_result;
            Output output(ss_result);
            {
                BitOStream out(output);
                for(size_t k = i; k; k--) out.write_bit(1);
            }
            result = ss_result.str();
        }

        // read bits until EOF
        size_t n = 0;
        {
            Input input(result);
            BitIStream in(input);
            ASSERT_EQ(i == 0, in.eof());
            for(; !in.eof(); n++) ASSERT_EQ(1, in.read_bit());
        }

        ASSERT_EQ(i, n);
    }
}

TEST(bit_io, compressed) {
    // test border case for compressed_int
    std::stringstream ss_out;
    {
        Output output(ss_out);
        BitOStream out(output);
        out.write_compressed_int(1ULL << 63, 3);
    }

    std::string result = ss_out.str();

    //advanced input test
    {
        Input input(result);
        BitIStream in(input);
        ASSERT_EQ(in.read_compressed_int<size_t>(3), 1ULL << 63);
    }
}

