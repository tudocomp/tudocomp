#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/util/IntegerBase.hpp>
#include <tudocomp/ds/uint_t.hpp>
#include <tudocomp/util/bit_packed_layout_t.hpp>

using namespace tdc;
using namespace int_vector;
using namespace cbp;

TEST(bit_layout, test1) {
    auto layout = bit_layout_t();

    auto bv = layout.aligned_elements<uint64_t>(1);
    auto values = layout.aligned_elements<uint32_t>(3);
    layout.aligned_elements<uint64_t>(0);
    auto quot_bv = layout.bit_packed_elements(30, 3);

    // [        |        |        |        |        ]
    // [   bv   ]
    //          [   values    ]
    //                            [    quot   ]

    ASSERT_EQ(bv.bit_offset(), 0);
    ASSERT_EQ(bv.bit_size(), 64);

    ASSERT_EQ(values.bit_offset(), 64);
    ASSERT_EQ(values.bit_size(), 3 * 32);

    ASSERT_EQ(quot_bv.bit_offset(), 64 * 3);
    ASSERT_EQ(quot_bv.bit_size(), 90);

    ASSERT_EQ(layout.get_size_in_uint64_t_units(), 5);

    std::array<uint64_t, 5> m_data;
    auto byte_offset = [&](auto ptr) {
        return ((char*) ptr) - ((char*) m_data.data());
    };

    auto val_ptr = values.ptr_relative_to(m_data.data());
    ASSERT_EQ(byte_offset(val_ptr), 8);
}

TEST(bit_layout, test2) {
    auto layout = bit_layout_t();

    auto bv = layout.aligned_elements<uint64_t>(1);
    auto values = layout.aligned_elements<uint32_t>(3);
    auto quot_bv = layout.bit_packed_elements(30, 3);

    // [        |        |        |        ]
    // [   bv   ]
    //          [   values    ]
    //                        [    quot   ]

    ASSERT_EQ(bv.bit_offset(), 0);
    ASSERT_EQ(bv.bit_size(), 64);

    ASSERT_EQ(values.bit_offset(), 64);
    ASSERT_EQ(values.bit_size(), 3 * 32);

    ASSERT_EQ(quot_bv.bit_offset(), 64 + 3 * 32);
    ASSERT_EQ(quot_bv.bit_size(), 90);

    ASSERT_EQ(layout.get_size_in_uint64_t_units(), 4);

    std::array<uint64_t, 4> m_data;
    auto byte_offset = [&](auto ptr) {
        return ((char*) ptr) - ((char*) m_data.data());
    };

    auto val_ptr = values.ptr_relative_to(m_data.data());
    ASSERT_EQ(byte_offset(val_ptr), 8);
}

TEST(bit_layout, test3) {
    auto layout = bit_layout_t();

    auto bv = layout.aligned_elements<uint64_t>(1);
    auto values = layout.bit_packed_elements(15, 3);
    auto quot_bv = layout.bit_packed_elements(15, 3);

    // [        |        |        ]
    // [   bv   ]
    //          [values]
    //                 [ quot ]

    ASSERT_EQ(bv.bit_offset(), 0);
    ASSERT_EQ(bv.bit_size(), 64);

    ASSERT_EQ(values.bit_offset(), 64);
    ASSERT_EQ(values.bit_size(), 3 * 15);

    ASSERT_EQ(quot_bv.bit_offset(), 64 + 3 * 15);
    ASSERT_EQ(quot_bv.bit_size(), 45);

    ASSERT_EQ(layout.get_size_in_uint64_t_units(), 3);

    // std::array<uint64_t, 3> m_data;
}

TEST(bit_layout, test4) {
    auto layout = bit_layout_t();

    auto bv = layout.aligned_elements<uint64_t>(1);

    auto values_width = typename cbp_repr_t<uint_t<15>>::width_repr_t(0);
    auto values = layout.cbp_elements<uint_t<15>>(3, values_width);

    auto quot_width = typename cbp_repr_t<dynamic_t>::width_repr_t(15);
    auto quot_bv = layout.cbp_elements<dynamic_t>(3, quot_width);

    // [        |        |        ]
    // [   bv   ]
    //          [values]
    //                 [ quot ]

    ASSERT_EQ(bv.bit_offset(), 0);
    ASSERT_EQ(bv.bit_size(), 64);

    ASSERT_EQ(values.bit_offset(), 64);
    ASSERT_EQ(values.bit_size(), 3 * 15);

    ASSERT_EQ(quot_bv.bit_offset(), 64 + 3 * 15);
    ASSERT_EQ(quot_bv.bit_size(), 45);

    ASSERT_EQ(layout.get_size_in_uint64_t_units(), 3);

    std::array<uint64_t, 3> m_data;
    auto byte_offset = [&](auto ptr) {
        return ((char*) ptr) - ((char*) m_data.data());
    };

    auto val_ptr = values.ptr_relative_to(m_data.data());
    ASSERT_EQ(byte_offset(val_ptr.internal_ptr()), 8);
    ASSERT_EQ(val_ptr.internal_bit_offset(), 0);

    auto quot_ptr = quot_bv.ptr_relative_to(m_data.data());
    ASSERT_EQ(byte_offset(quot_ptr.internal_ptr()), 8);
    ASSERT_EQ(quot_ptr.internal_bit_offset(), 45);
}

TEST(bit_layout, test5) {
    auto layout = bit_layout_t();

    auto bv = layout.aligned_elements<uint64_t>(1);

    auto values_width = typename cbp_repr_t<uint_t<15>>::width_repr_t(0);
    auto values = layout.cbp_elements<uint_t<15>>(3, values_width);

    auto quot_width = typename cbp_repr_t<dynamic_t>::width_repr_t(30);
    auto quot_bv = layout.cbp_elements<dynamic_t>(3, quot_width);

    // [        |        |        |        ]
    // [   bv   ]
    //          [values]
    //                 [      quot    ]

    ASSERT_EQ(bv.bit_offset(), 0);
    ASSERT_EQ(bv.bit_size(), 64);

    ASSERT_EQ(values.bit_offset(), 64);
    ASSERT_EQ(values.bit_size(), 3 * 15);

    ASSERT_EQ(quot_bv.bit_offset(), 64 + 3 * 15);
    ASSERT_EQ(quot_bv.bit_size(), 3 * 30);

    ASSERT_EQ(layout.get_size_in_uint64_t_units(), 4);

    std::array<uint64_t, 4> m_data;
    auto byte_offset = [&](auto ptr) {
        return ((char*) ptr) - ((char*) m_data.data());
    };

    auto val_ptr = values.ptr_relative_to(m_data.data());
    ASSERT_EQ(byte_offset(val_ptr.internal_ptr()), 8);
    ASSERT_EQ(val_ptr.internal_bit_offset(), 0);

    auto quot_ptr = quot_bv.ptr_relative_to(m_data.data());
    ASSERT_EQ(byte_offset(quot_ptr.internal_ptr()), 8);
    ASSERT_EQ(quot_ptr.internal_bit_offset(), 45);
}

TEST(bit_layout, test6) {
    auto layout = bit_layout_t();

    auto bv = layout.aligned_elements<uint64_t>(1);

    auto values = layout.aligned_elements<uint32_t>(3);
    layout.aligned_elements<uint64_t>(0);
    auto quot_width = typename cbp_repr_t<dynamic_t>::width_repr_t(30);
    auto quot_bv = layout.cbp_elements<dynamic_t>(3, quot_width);

    // [        |        |        |        |        ]
    // [   bv   ]
    //          [   values    ]
    //                            [    quot   ]

    ASSERT_EQ(bv.bit_offset(), 0);
    ASSERT_EQ(bv.bit_size(), 64);

    ASSERT_EQ(values.bit_offset(), 64);
    ASSERT_EQ(values.bit_size(), 3 * 32);

    ASSERT_EQ(quot_bv.bit_offset(), 64 * 3);
    ASSERT_EQ(quot_bv.bit_size(), 90);

    ASSERT_EQ(layout.get_size_in_uint64_t_units(), 5);

    std::array<uint64_t, 5> m_data;
    auto byte_offset = [&](auto ptr) {
        return ((char*) ptr) - ((char*) m_data.data());
    };

    auto val_ptr = values.ptr_relative_to(m_data.data());
    ASSERT_EQ(byte_offset(val_ptr), 8);

    auto quot_ptr = quot_bv.ptr_relative_to(m_data.data());
    ASSERT_EQ(byte_offset(quot_ptr.internal_ptr()), 24);
    ASSERT_EQ(quot_ptr.internal_bit_offset(), 0);
}

TEST(bit_layout, test7) {
    auto layout = bit_layout_t();

    auto bv = layout.aligned_elements<uint64_t>(1);

    auto values = layout.aligned_elements<uint32_t>(3);
    auto quot_width = typename cbp_repr_t<dynamic_t>::width_repr_t(30);
    auto quot_bv = layout.cbp_elements<dynamic_t>(3, quot_width);


    // [        |        |        |        ]
    // [   bv   ]
    //          [   values    ]
    //                        [    quot   ]

    ASSERT_EQ(bv.bit_offset(), 0);
    ASSERT_EQ(bv.bit_size(), 64);

    ASSERT_EQ(values.bit_offset(), 64);
    ASSERT_EQ(values.bit_size(), 3 * 32);

    ASSERT_EQ(quot_bv.bit_offset(), 64 + 3 * 32);
    ASSERT_EQ(quot_bv.bit_size(), 90);

    ASSERT_EQ(layout.get_size_in_uint64_t_units(), 4);

    std::array<uint64_t, 4> m_data;
    auto byte_offset = [&](auto ptr) {
        return ((char*) ptr) - ((char*) m_data.data());
    };

    auto val_ptr = values.ptr_relative_to(m_data.data());
    ASSERT_EQ(byte_offset(val_ptr), 8);

    auto quot_ptr = quot_bv.ptr_relative_to(m_data.data());
    ASSERT_EQ(byte_offset(quot_ptr.internal_ptr()), 16);
    ASSERT_EQ(quot_ptr.internal_bit_offset(), 32);
}

template<typename T>
struct Foo {};

template<>
struct Foo<uint32_t> {
    using Ref = uint32_t&;
};

struct RefLike {
    uint16_t* p;

    RefLike operator=(uint16_t val) {
        std::cout << "wrapper\n";
        *p = val;

        return *this;
    }

    RefLike(uint16_t& real_ref): p(&real_ref) {}
};

template<>
struct Foo<uint16_t> {
    using Ref = RefLike;
};

template<typename T>
void test() {
    T val = 42;

    typename Foo<T>::Ref r = val;

    r = 123;

    ASSERT_EQ(val, 123);
}


TEST(Sandbox, typedef) {
    test<uint32_t>();
    test<uint16_t>();
}
