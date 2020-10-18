@page bv Bit Vectors

# Bit Vectors
Bit vectors are provided by the @ref tdc::BitVector class. Technically, it is
an alias for `IntVector<uint_t<1>>` and thus BitVector can be used like any
vector type.

The following data structures can be built on top of a BitVector:
* @subpage rankselect
