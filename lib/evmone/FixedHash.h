// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

/// @file
/// The FixedHash fixed-size "hash" container type.
#pragma once

#define BOOST_NO_EXCEPTIONS

#include <array>
#include <cstdint>
#include <algorithm>
#include <random>
#include <set>
#include <unordered_set>
#include <sstream>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/throw_exception.hpp>

void boost::throw_exception(std::exception const & /*e*/, boost::source_location const& /*l*/){
    std::exit(EXIT_FAILURE);
}

namespace evmone
{
using u256 =  boost::multiprecision::number<boost::multiprecision::cpp_int_backend<256, 256, boost::multiprecision::unsigned_magnitude, boost::multiprecision::unchecked, void>>;
using bigint = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<>>;

using bytes = bytevec;

/// Converts a templated integer value to the big-endian byte-stream represented on a templated collection.
/// The size of the collection object will be unchanged. If it is too small, it will not represent the
/// value properly, if too big then the additional elements will be zeroed out.
/// @a Out will typically be either std::string or bytes.
/// @a T will typically by unsigned, u160, u256 or bigint.
template <class T, class Out>
inline void toBigEndian(T _val, Out& o_out)
{
	static_assert(std::is_same<bigint, T>::value || !std::numeric_limits<T>::is_signed, "only unsigned types or bigint supported"); //bigint does not carry sign bit on shift
	for (auto i = o_out.size(); i != 0; _val >>= 8, i--)
	{
		T v = _val & (T)0xff;
		o_out[i - 1] = (typename Out::value_type)(uint8_t)v;
	}
}

/// Converts a big-endian byte-stream represented on a templated collection to a templated integer value.
/// @a _In will typically be either std::string or bytes.
/// @a T will typically by unsigned, u160, u256 or bigint.
template <class T, class _In>
inline T fromBigEndian(_In const& _bytes)
{
	T ret = (T)0;
	for (auto i: _bytes)
		ret = (T)((ret << 8) | (byte)(typename std::make_unsigned<decltype(i)>::type)i);
	return ret;
}

/// Compile-time calculation of Log2 of constant values.
template <unsigned N> struct StaticLog2 { enum { result = 1 + StaticLog2<N/2>::result }; };
template <> struct StaticLog2<1> { enum { result = 0 }; };

extern std::random_device s_fixedHashEngine;

/// Fixed-size raw-byte array container type, with an API optimised for storing hashes.
/// Transparently converts to/from the corresponding arithmetic type; this will
/// assume the data contained in the hash is big-endian.
template <unsigned N>
class FixedHash
{
public:
    /// The corresponding arithmetic type.
    using Arith = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<N * 8, N * 8, boost::multiprecision::unsigned_magnitude, boost::multiprecision::unchecked, void>>;

    /// Convert to arithmetic type.
    operator Arith() const { return fromBigEndian<Arith>(m_data); }

    /// The size of the container.
    enum { size = N };

    /// A dummy flag to avoid accidental construction from pointer.
    enum ConstructFromPointerType { ConstructFromPointer };

    /// Method to convert from a string.
    enum ConstructFromStringType { FromHex, FromBinary };

    /// Method to convert from a string.
    enum ConstructFromHashType { AlignLeft, AlignRight, FailIfDifferent };

    /// Construct an empty hash.
    FixedHash() { m_data.fill(0); }

    /// Construct from another hash, filling with zeroes or cropping as necessary.
    template <unsigned M> explicit FixedHash(FixedHash<M> const& _h, ConstructFromHashType _t = AlignLeft) { m_data.fill(0); unsigned c = std::min(M, N); for (unsigned i = 0; i < c; ++i) m_data[_t == AlignRight ? N - 1 - i : i] = _h[_t == AlignRight ? M - 1 - i : i]; }

    /// Convert from unsigned
    explicit FixedHash(unsigned _u) { toBigEndian(_u, m_data); }

    /// Explicitly construct, copying from a byte array.
    explicit FixedHash(bytes const& _b, ConstructFromHashType _t = FailIfDifferent) { if (_b.size() == N) memcpy(m_data.data(), _b.data(), std::min<unsigned>(_b.size(), N)); else { m_data.fill(0); if (_t != FailIfDifferent) { auto c = std::min<unsigned>(_b.size(), N); for (unsigned i = 0; i < c; ++i) m_data[_t == AlignRight ? N - 1 - i : i] = _b[_t == AlignRight ? _b.size() - 1 - i : i]; } } }

    /// Explicitly construct, copying from a byte array.
    explicit FixedHash(bytesConstRef _b, ConstructFromHashType _t = FailIfDifferent) { if (_b.size() == N) memcpy(m_data.data(), _b.data(), std::min<size_t>(_b.size(), N)); else { m_data.fill(0); if (_t != FailIfDifferent) { auto c = std::min<size_t>(_b.size(), N); for (unsigned i = 0; i < c; ++i) m_data[_t == AlignRight ? N - 1 - i : i] = _b[_t == AlignRight ? _b.size() - 1 - i : i]; } } }

    /// Explicitly construct, copying from a bytes in memory with given pointer.
    explicit FixedHash(byte const* _bs, ConstructFromPointerType) { memcpy(m_data.data(), _bs, N); }

    /// @returns true iff this is the empty hash.
    explicit operator bool() const { return std::any_of(m_data.begin(), m_data.end(), [](byte _b) { return _b != 0; }); }

    // The obvious comparison operators.
    bool operator==(FixedHash const& _c) const { return m_data == _c.m_data; }
    bool operator!=(FixedHash const& _c) const { return m_data != _c.m_data; }
    bool operator<(FixedHash const& _c) const { for (unsigned i = 0; i < N; ++i) if (m_data[i] < _c.m_data[i]) return true; else if (m_data[i] > _c.m_data[i]) return false; return false; }
    bool operator>=(FixedHash const& _c) const { return !operator<(_c); }
    bool operator<=(FixedHash const& _c) const { return operator==(_c) || operator<(_c); }
    bool operator>(FixedHash const& _c) const { return !operator<=(_c); }

    // The obvious binary operators.
    FixedHash& operator^=(FixedHash const& _c) { for (unsigned i = 0; i < N; ++i) m_data[i] ^= _c.m_data[i]; return *this; }
    FixedHash operator^(FixedHash const& _c) const { return FixedHash(*this) ^= _c; }
    FixedHash& operator|=(FixedHash const& _c) { for (unsigned i = 0; i < N; ++i) m_data[i] |= _c.m_data[i]; return *this; }
    FixedHash operator|(FixedHash const& _c) const { return FixedHash(*this) |= _c; }
    FixedHash& operator&=(FixedHash const& _c) { for (unsigned i = 0; i < N; ++i) m_data[i] &= _c.m_data[i]; return *this; }
    FixedHash operator&(FixedHash const& _c) const { return FixedHash(*this) &= _c; }
    FixedHash operator~() const { FixedHash ret; for (unsigned i = 0; i < N; ++i) ret[i] = ~m_data[i]; return ret; }

    // Big-endian increment.
    FixedHash& operator++() { for (unsigned i = size; i > 0 && !++m_data[--i]; ) {} return *this; }

    /// @returns true if all one-bits in @a _c are set in this object.
    bool contains(FixedHash const& _c) const { return (*this & _c) == _c; }

    /// @returns a particular byte from the hash.
    byte& operator[](size_t _i) { return m_data[_i]; }
    /// @returns a particular byte from the hash.
    byte operator[](size_t _i) const { return m_data[_i]; }

    /// @returns a mutable byte vector_ref to the object's data.
    bytesRef ref() { return bytesRef(m_data.data(), N); }

    /// @returns a constant byte vector_ref to the object's data.
    bytesConstRef ref() const { return bytesConstRef(m_data.data(), N); }

    /// @returns a mutable byte pointer to the object's data.
    byte* data() { return m_data.data(); }

    /// @returns a constant byte pointer to the object's data.
    byte const* data() const { return m_data.data(); }

    /// @returns begin iterator.
    auto begin() const -> typename std::array<byte, N>::const_iterator { return m_data.begin(); }

    /// @returns end iterator.
    auto end() const -> typename std::array<byte, N>::const_iterator { return m_data.end(); }

    /// @returns a copy of the object's data as a byte vector.
    bytes asBytes() const { return bytes(data(), data() + N); }

    /// @returns a mutable reference to the object's data as an STL array.
    std::array<byte, N>& asArray() { return m_data; }

    /// @returns a constant reference to the object's data as an STL array.
    std::array<byte, N> const& asArray() const { return m_data; }

    /// Populate with random data.
    template <class Engine>
    void randomize(Engine& _eng)
    {
        for (auto& i: m_data)
            i = (uint8_t)std::uniform_int_distribution<uint16_t>(0, 255)(_eng);
    }

    /// @returns a random valued object.
    static FixedHash random() { FixedHash ret; ret.randomize(s_fixedHashEngine); return ret; }

    template <unsigned P, unsigned M> inline FixedHash& shiftBloom(FixedHash<M> const& _h)
    {
        return (*this |= _h.template bloomPart<P, N>());
    }

    template <unsigned P, unsigned M> inline bool containsBloom(FixedHash<M> const& _h)
    {
        return contains(_h.template bloomPart<P, N>());
    }

    template <unsigned P, unsigned M> inline FixedHash<M> bloomPart() const
    {
        unsigned const c_bloomBits = M * 8;
        unsigned const c_mask = c_bloomBits - 1;
        unsigned const c_bloomBytes = (StaticLog2<c_bloomBits>::result + 7) / 8;

        static_assert((M & (M - 1)) == 0, "M must be power-of-two");
        static_assert(P * c_bloomBytes <= N, "out of range");

        FixedHash<M> ret;
        byte const* p = data();
        for (unsigned i = 0; i < P; ++i)
        {
            unsigned index = 0;
            for (unsigned j = 0; j < c_bloomBytes; ++j, ++p)
                index = (index << 8) | *p;
            index &= c_mask;
            ret[M - 1 - index / 8] |= (1 << (index % 8));
        }
        return ret;
    }

    /// Returns the index of the first bit set to one, or size() * 8 if no bits are set.
    inline unsigned firstBitSet() const
    {
        unsigned ret = 0;
        for (auto d: m_data)
            if (d)
            {
                for (;; ++ret, d <<= 1)
                {
                    if (d & 0x80)
                        return ret;
                }
            }
            else
                ret += 8;
        return ret;
    }

    void clear() { m_data.fill(0); }

private:
    std::array<byte, N> m_data;        ///< The binary data.
};

/// Fast equality operator for h256.
template<> inline bool FixedHash<32>::operator==(FixedHash<32> const& _other) const
{
    const uint64_t* hash1 = (const uint64_t*)data();
    const uint64_t* hash2 = (const uint64_t*)_other.data();
    return (hash1[0] == hash2[0]) && (hash1[1] == hash2[1]) && (hash1[2] == hash2[2]) && (hash1[3] == hash2[3]);
}

/// Stream I/O for the FixedHash class.
template <unsigned N>
inline std::ostream& operator<<(std::ostream& _out, FixedHash<N> const& _h)
{
    _out << toHex(_h);
    return _out;
}

template <unsigned N>
inline std::istream& operator>>(std::istream& _in, FixedHash<N>& o_h)
{
    std::string s;
    _in >> s;
    o_h = FixedHash<N>(s, FixedHash<N>::FromHex, FixedHash<N>::AlignRight);
    return _in;
}

// Common types of FixedHash.
using h2048 = FixedHash<256>;
using h1024 = FixedHash<128>;
using h520 = FixedHash<65>;
using h512 = FixedHash<64>;
using h256 = FixedHash<32>;
using h160 = FixedHash<20>;
using h128 = FixedHash<16>;
using h64 = FixedHash<8>;
using h512s = std::vector<h512>;
using h256s = std::vector<h256>;
using h160s = std::vector<h160>;
using h256Set = std::set<h256>;
using h160Set = std::set<h160>;
using h256Hash = std::unordered_set<h256>;
using h160Hash = std::unordered_set<h160>;

/// Convert the given value into h160 (160-bit unsigned integer) using the right 20 bytes.
inline h160 right160(h256 const& _t)
{
    h160 ret;
    memcpy(ret.data(), _t.data() + 12, 20);
    return ret;
}

}
