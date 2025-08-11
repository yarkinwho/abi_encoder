#pragma once

#include <vector>
#include <string>
#include <type_traits>
#include "evm_types.hpp"

/*
Need -std=c++20 when compiling using g++. But the code can compile using cdt with default settings.

Usage:

This code should generate the results in https://docs.soliditylang.org/en/latest/abi-spec.html

struct test1 {
    uint64_t a;
    std::vector<uint32_t> b;
    uint8_t c[10];
    std::string d;

    DEF_ENCODER(a, b, c, d);
};

struct test2 {
    std::vector<std::vector<intx::uint256>> a;
    std::vector<std::string> b;

    DEF_ENCODER(a, b);
};

test1 t1 = {0x123, {0x456, 0x789}, {'1','2','3','4','5','6','7','8','9','0'}, "Hello, world!"};

test2 t2 = {{ {1,2}, {3}}, {"one","two","three"}};

std::vector<uint8_t> output1;
std::vector<uint8_t> output2;

AbiEncoder enc1(t1);
enc1.encode(output1);

AbiEncoder enc2(t2);
enc2.encode(output2);


*/
namespace utils {
// Local helpers

using byte = uint8_t;
using bytes = std::vector<byte>;

void appendNumber(std::vector<uint8_t>& buffer, intx::uint256 input) {
    uint8_t val_[32] = {};
    intx::be::store(val_, input);
    buffer.insert(buffer.end(), val_, val_ + sizeof(val_));
}

template<unsigned N, std::enable_if_t<(N <= 32), bool> = true>
void appendFixedBytes(std::vector<uint8_t>& buffer, const byte(&input)[N]) {
    uint8_t buf[32] = {};
    memcpy(buf, input, 32 > sizeof(input) ? sizeof(input) : 32 );
    buffer.insert(buffer.end(), buf, buf + sizeof(buf));
}

void appendBytes(std::vector<uint8_t>& buffer, const bytes& input) {
    appendNumber(buffer, intx::uint256(input.size()));

    for (size_t i = 0; i < (input.size() + 31) / 32 * 32; i += 32) {
        uint8_t buf[32] = {};
        memcpy(buf, input.data() + i, i + 32 > input.size() ? input.size() - i : 32);
        buffer.insert(buffer.end(), buf, buf + sizeof(buf));
    }
}

void appendString(std::vector<uint8_t>& buffer, const std::string& input) {
    appendNumber(buffer, intx::uint256(input.size()));

    for (size_t i = 0; i < (input.size() + 31) / 32 * 32; i += 32) {
        uint8_t buf[32] = {};
        memcpy(buf, input.data() + i, i + 32 > input.size() ? input.size() - i : 32);
        buffer.insert(buffer.end(), buf, buf + sizeof(buf));
    }
}

// Define a template class 'check' to test for the existence
// of 'func'
template <typename T> class check_encoder_struct {
    // Define two types of character arrays, 'yes' and 'no',
    // for SFINAE test
    typedef char yes[1];
    typedef char no[2];

    // Test if class T has a member function named 'encodeStruct'
    // If T has 'func', this version of test() is chosen
    template <typename C>
    static yes& test(decltype(&C::encodeStruct));

    // Fallback test() function used if T does not have
    // 'func'
    template <typename> static no& test(...);

public:
    // Static constant 'value' becomes true if T has 'func',
    // false otherwise The comparison is based on the size
    // of the return type from test()
    static const bool value
        = sizeof(test<T>(0)) == sizeof(yes);
};
template< class T >
constexpr bool check_encoder_struct_v = check_encoder_struct<T>::value;

class AbiEncoder {
public:
    enum DataType {
      kTuple, // Tuple, fixed size array of types. Dynamic if any element of the tuple is dynamic.
      kUint, // bool, all numbers that can be converted to uint256. Static.
      kByteN, // Fixed size byte array, N <= 32. Static.
      kBytes, // variable length byte array, string. Dynamic.
      kVLA, // variable length array of types. Dynamic.
    };

    // Empty Tuple for appending children.
    AbiEncoder() : m_type(kTuple) {
        // m_dynamic flag is not used. default to false;
    }

    // Uint
    // Need manually convert to uint to use this lib as intx we used only support uint...
    template<class T, 
        std::enable_if_t<std::is_convertible_v<T, intx::uint256>, bool> = true> 
    AbiEncoder( T input) : m_type(kUint) {
        appendNumber(m_data, intx::uint256(input));
    }

    // boolean is uint
    AbiEncoder(bool input) : 
        AbiEncoder(intx::uint256(byte(input))) {
    }

    // Fix size bytes. 
    // Capture N > 32 cases in the ctor, then fail it in appendFixedBytes.
    template<unsigned N>
    AbiEncoder(const byte (&input)[N]) : m_type(kByteN) {
        // will fail to compile if N > 32
        appendFixedBytes(m_data, input);
    }

    // evm address
    AbiEncoder(const evm_address& input) : m_type(kByteN) {
        byte buf[32] = {};
        memcpy(buf + 32 - kAddressLength, input.data, kAddressLength);
        appendFixedBytes(m_data, buf);
    }

    // bytes
    AbiEncoder(const bytes& input) : m_type(kBytes) {
        appendBytes(m_data, input);
    }

    // string is bytes
    AbiEncoder(const std::string& input) : m_type(kBytes)  {
        appendString(m_data, input);
    }

    // explict ctor for char* string to remove ambiguity.
    AbiEncoder(const char* input) : 
        AbiEncoder(std::string(input)) {
    }

    // Struct is tuple
    template<class T, 
        std::enable_if_t<check_encoder_struct_v<T>, bool> = true> 
    AbiEncoder(const T& input) : m_type(kTuple) {
        input.encodeStruct(*this);
    }

    // Fix size array of types is tuple
    template<class T, unsigned N>
    AbiEncoder(const T (&input)[N]) : m_type(kTuple) {
        for (const auto& i : input) {
            this->append(i);
        }
    }
    
    // Variable length array of types
    template<class T> 
    AbiEncoder(const std::vector<T>& input) : m_type(kVLA) {
        for (const auto& i : input) {
            this->append(i);
        }
    }

    // append a new element for tuple
    void append(AbiEncoder&& input) {
        if (m_type == kTuple || m_type == kVLA) {
            m_children.push_back(std::move(input));
        }
    }

    // generate encoded bytes
    void encode(std::vector<uint8_t>& buffer) const {
        if (m_type == kUint || m_type == kBytes || m_type == kByteN ) {
            // m_data should contain encoded data for non tuple case
            buffer.insert(buffer.end(), m_data.data(), m_data.data() + m_data.size());
        } 
        else {
            // VLA or tuple
            if (m_type == kVLA) {
                appendNumber(buffer, intx::uint256(m_children.size()));
            }
            size_t headsize = 0;
            std::vector<uint8_t> tails;
            for (const auto& c : m_children) {
                headsize += c.getHeadSize();
            }
            for (const auto& c : m_children) {
                c.encodeHead(buffer, headsize + tails.size());
                c.encodeTail(tails);
            }
            buffer.insert(buffer.end(), tails.data(), tails.data() + tails.size());
        }        
    }

private:
    DataType m_type;
    std::vector<uint8_t> m_data;
    std::vector<AbiEncoder> m_children;

    inline bool isDynamic() const { 
        if (m_type == kVLA || m_type == kBytes) {
            return true;
        }
        else if (m_type == kByteN || m_type == kUint) {
            return false;
        }
        else {
            // Must be tuple
            for (const auto& c : m_children) {
                if (c.isDynamic()) {
                    return true;
                }
            }
            return false;
        }
    }

    inline size_t getHeadSize() const {
        if (isDynamic()) {
            return 32;
        }
        else {
            if (m_type == kTuple) {
                size_t sum = 0;
                for (const auto& c : m_children) {
                    // It's known they are static
                    sum += c.getHeadSize();
                }
                return sum;
            }
            else {
                // non tuple static types should always has the same size.
                return 32;
            }
        }
    }

    void encodeHead(std::vector<uint8_t>& buffer, size_t offset) const {
        if (isDynamic()) {
            appendNumber(buffer, intx::uint256(offset));
        }
        else {
            encode(buffer);
        }
    }

    void encodeTail(std::vector<uint8_t>& buffer) const {
        if (isDynamic()) {
            encode(buffer);
        }
        else {
            return;
        }
    }
};


} // utils


// Macro for DEF_ENCODER(...)
// tricks from
// https://www.scs.stanford.edu/~dm/blog/va-opt.html#the-for_each-macro

#define PARENS ()

//#define EXPAND(...) EXPAND4(EXPAND4(EXPAND4(EXPAND4(__VA_ARGS__))))
//#define EXPAND4(...) EXPAND3(EXPAND3(EXPAND3(EXPAND3(__VA_ARGS__))))
#define EXPAND(...) EXPAND2(EXPAND2(EXPAND2(EXPAND2(__VA_ARGS__))))
#define EXPAND2(...) EXPAND1(EXPAND1(EXPAND1(EXPAND1(__VA_ARGS__))))
#define EXPAND1(...) __VA_ARGS__

#define FOR_EACH_HELPER(macro, a1, ...)                           \
    macro(a1);                                                    \
    __VA_OPT__(FOR_EACH_AGAIN PARENS (macro, __VA_ARGS__))
#define FOR_EACH_AGAIN() FOR_EACH_HELPER



#define DEF_ENCODER(...)                                                 \
    void encodeStruct(utils::AbiEncoder& encoder) const {                       \
        EXPAND(FOR_EACH_HELPER((encoder.append), __VA_ARGS__))           \
    }                                                   

