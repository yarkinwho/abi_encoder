#include <iostream>

#include "intx.hpp"
#include "hex.hpp"
#include "abi_encoder.hpp"


using namespace utils;

struct test {
    uint64_t a;
    std::string b;
    std::vector<std::string> c;
    std::string d[3];

    DEF_ENCODER(a, b, c, d);
};


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

struct test3 {
    utils::bytes a;
    utils::evm_address b;
    bool c;

    DEF_ENCODER(a, b, c);
};

struct test4 {
    test1 a;
    test2 b; 
    test3 c;

    DEF_ENCODER(a, b, c);
};


std::vector<std::string> splitStringBySize(const std::string& str, size_t chunkSize) {
    std::vector<std::string> result;
    for (size_t i = 0; i < str.length(); i += chunkSize) {
        result.push_back(str.substr(i, chunkSize));
    }
    return result;
}

void print(const std::vector<uint8_t>& buffer) {
    auto s = utils::vec_to_hex(buffer, false);

    auto ss = splitStringBySize(s,64);
    for (const auto& o : ss) {
        std::cout<<o<<std::endl;
    }
}


int main() {
    

    test1 t1 = {0x123, {0x456, 0x789}, {'1','2','3','4','5','6','7','8','9','0'}, "Hello, world!"};

    test2 t2 = {{ {1,2}, {3}}, {"one","two","three"}};

    test3 t3 = {{'1','2','3','4','5','6','7','8','9','0'}, {{0xbb,0xbb}}, true};

    test4 t4 = {t1,t2,t3};

    std::vector<uint8_t> buffer;
    std::vector<test4> vla = {t4, t4, t4};

    AbiEncoder enc;
    enc.append(t4);
    enc.append(vla);

    enc.encode(buffer);
    
    print(buffer);

}
