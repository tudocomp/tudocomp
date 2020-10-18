@page rankselect Rank & Select

# Rank & Select
The classes @ref tdc::Rank and @ref tdc::Select implement data
structures for these widely used operations on bit vectors. Both are succinct,
meaning that they allow constant-time queries with relatively low memory
overhead. While rank queries may be answered for 1-bits as well as 0-bits using
the same data structure, select has the variants @ref tdc::Select1 and
@ref tdc::Select0. The following example shows the usage of these data
structures:

@code{.cpp}
// Construct a bit vector where every second bit is set
BitVector bv(128);
for(len_t i = 1; i < 128; i += 2) bv[i] = 1;

// Construct Rank and Select1 data structures
Rank    rank(bv);
Select1 select1(bv);

// Query the amount of 1-bits in the whole bit vector
ASSERT_EQ(64, rank.rank1(127));

// Query the amount of 0-bits in the whole bit vector
ASSERT_EQ(64, rank.rank0(127));

// Query the amount of 1-bits in the second half of the bit vector
ASSERT_EQ(32, rank.rank0(64, 127));

// Find the position of the first 1-bit
ASSERT_EQ(1,  select1(1));

// Find the position of the 32nd 1-bit
ASSERT_EQ(63, select1(32));

// Find the position of the 1000th 1-bit (which does not exist)
ASSERT_EQ(bv.size(), select1(1000));

// rank1(select1(i)) = i holds
for(len_t i = 1; i <= 64; i++) {
    ASSERT_EQ(i, rank(select1(i)));
}
@endcode

The functions @ref tdc::Rank::rank1 and @ref tdc::Rank::rank0 invoke the
respective queries, while for select, the operator `()` can be used as a
shortcut. Note that rank also provides the `()`, which is equal to calling
`rank1`.

All operations expect zero-based indices, i.e., the position of the first bit
in a bit vector is 0. Orders, on the other hand, are intuitively one-based
i.e. the order of the 1^st^ occurence of a bit is 1 (e.g., as the return value
of rank or the input parameter of select). That said, select does not accept an
input order of 0.

If for a parameter `k`, the `k`^th^ occurence of a bit does not exist in the
bit vector, select will return the bit vector's length instead, as a means
of saying "out of scope".
