#pragma once

namespace utils {

    inline constexpr size_t kAddressLength{20};

    typedef struct {
        uint8_t data[kAddressLength];
    } evm_address;

}

