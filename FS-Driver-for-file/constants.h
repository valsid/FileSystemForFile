#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cstdint>
#include <cstddef>

class FSHeader;

#include <utility>

constexpr int headerAddress() {
    return 0;
}

//template <typename numberType, typename ResultType>
//class NumberWrapper
//{
//protected:
//    numberType _number;
//public:
//    NumberWrapper() = default;

//    static const ResultType fromNumber(numberType u) {
//        ResultType b;
//        b._number = u;
//        return b;
//    }

//    operator numberType() const { return _number; }

//    ResultType operator +(const int &other) const {return fromNumber(_number + other); }
//    ResultType operator +(const numberType &other) const {return fromNumber(_number + other); }

//    ResultType &operator ++() { ++_number; return *static_cast<ResultType*>(this); }

//    ResultType &operator =(const numberType &other) { _number = other; }
//    ResultType &operator =(const ResultType &other) { _number = other._number; }

//    const ResultType operator += (const numberType &other) {return fromNumber(_number += other); }
//    const ResultType operator -= (const numberType &other) {return fromNumber(_number -= other); }

//    bool operator <(const ResultType &other) const { return _number < other._number; }
//    bool operator <=(const ResultType &other) const { return _number <= other._number; }
//};

//class BlockAddress : public NumberWrapper<uint64_t, BlockAddress>
//{
//public:
//    BlockAddress nextBlock() const {return *this + 1; }
//    // isValid
//};

using filenameLength_tp = uint32_t;
using blockAddress_tp = uint64_t;
using offsetInBlock_tp = uint64_t;
using directoryEntryIndex_tp = uint64_t;

using byte_tp = uint8_t;
using descriptorIndex_tp = uint64_t;

class Constants
{
public:
    constexpr static uint64_t blockByteSize() {
        return 1024;
    }

    constexpr static blockAddress_tp HEADER_ADDRESS() {     // always equals 0
        return 0;
    }

    constexpr static descriptorIndex_tp INVALID_DESCRIPTOR_ID() {     // always equals 0
        return 0;
    }

    constexpr static int maxFilename() {
        return 16;
    }

    constexpr static size_t magicMarkSize = 33;    // 64 symbols and '\n'
    using mark_t = char[magicMarkSize];

    constexpr static mark_t mMark = "WK8sTuQpu_[MgkRwRL)vgY8G53w}gqoz";     // its two UUIDs
};

#endif // CONSTANTS_H
