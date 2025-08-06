# abi_encoder
Simple Abi Encoder for encoding Ethereum calls.
It still miss some features but should largely work.

Include the abi_encoder.hpp to use the lib. Depends on intx lib (the intx.hpp).

The intx.hpp needs ```-std=c++20``` to compile using gcc. But it will work using cdt with default settings.

Some example:

This code should generate the results in https://docs.soliditylang.org/en/latest/abi-spec.html
```
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
```

