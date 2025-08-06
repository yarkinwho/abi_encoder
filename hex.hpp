
#pragma once

#include <optional>
#include <string>
#include <vector>

// Hex helper functions:
// Copied from EVMC: Ethereum Client-VM Connector API.
// No functionality modification expected.
// Copyright 2021 The EVMC Authors.
// Licensed under the Apache License, Version 2.0.

namespace utils {

/// Extracts the nibble value out of a hex digit.
/// Returns -1 in case of invalid hex digit.
inline constexpr int from_hex_digit(char h) noexcept {
    if (h >= '0' && h <= '9')
        return h - '0';
    else if (h >= 'a' && h <= 'f')
        return h - 'a' + 10;
    else if (h >= 'A' && h <= 'F')
        return h - 'A' + 10;
    else
        return -1;
}

/// Decodes hex-encoded sequence of characters.
///
/// It is guaranteed that the output will not be longer than half of the input length.
///
/// @param begin  The input begin iterator. It only must satisfy input iterator concept.
/// @param end    The input end iterator. It only must satisfy input iterator concept.
/// @param out    The output iterator. It must satisfy output iterator concept.
/// @return       True if successful, false if input is invalid hex.
template <typename InputIt, typename OutputIt>
inline constexpr bool from_hex(InputIt begin, InputIt end, OutputIt out) noexcept {
    int hi_nibble = -1;  // Init with invalid value, should never be used.
    size_t i = 0;
    for (auto it = begin; it != end; ++it, ++i) {
        const auto h = *it;
        const int v = from_hex_digit(h);
        if (v < 0) {
            if (i == 1 && hi_nibble == 0 && h == 'x')  // 0x prefix
                continue;
            return false;
        }

        if (i % 2 == 0)
            hi_nibble = v << 4;
        else
            *out++ = static_cast<uint8_t>(hi_nibble | v);
    }

    return i % 2 == 0;
}

/// Decodes hex encoded string to bytes.
///
/// In case the input is invalid the returned value is std::nullopt.
/// This can happen if a non-hex digit or odd number of digits is encountered.
inline std::optional<std::vector<char>> from_hex(std::string_view hex) {
    std::vector<char> bs;
    bs.reserve(hex.size() / 2);
    if (!from_hex(hex.begin(), hex.end(), std::back_inserter(bs)))
        return {};
    return bs;
}


inline std::string vec_to_hex(std::vector<char> byte_array, bool with_prefix) {
    static const char* kHexDigits{"0123456789abcdef"};
    std::string out(byte_array.size() * 2 + (with_prefix ? 2 : 0), '\0');
    char* dest{&out[0]};
    if (with_prefix) {
        *dest++ = '0';
        *dest++ = 'x';
    }
    for (const auto& b : byte_array) {
        *dest++ = kHexDigits[(uint8_t)b >> 4];    // Hi
        *dest++ = kHexDigits[(uint8_t)b & 0x0f];  // Lo
    }
    return out;
}

inline std::string vec_to_hex(std::vector<uint8_t> byte_array, bool with_prefix) {
    static const char* kHexDigits{"0123456789abcdef"};
    std::string out(byte_array.size() * 2 + (with_prefix ? 2 : 0), '\0');
    char* dest{&out[0]};
    if (with_prefix) {
        *dest++ = '0';
        *dest++ = 'x';
    }
    for (const auto& b : byte_array) {
        *dest++ = kHexDigits[b >> 4];    // Hi
        *dest++ = kHexDigits[b & 0x0f];  // Lo
    }
    return out;
}

}  // namespace utils