#include "constants.h"

#include "filesystemblock.h"

#include <stdexcept>

//blockAddress_tp DescriptorAddress::toBlockAddress(uint64_t descriptorsInBlock, const FSHeader &header)
//{

//}


//_BlockAddress::_BlockAddress(uint64_t address) :
//    _address(address)
//{ }

//std::pair<_BlockAddress, uint64_t> _BlockAddress::bitMapPosition(const FSHeader &header)
//{
//    if(_address < header.descriptorsBegin()) {
//        throw std::invalid_argument("Bad argument in FileSystem::_BlockAddressToBitsetAddress: " + std::to_string(_address));
//    }
//    _address -= header.descriptorsBegin();

//    int64_t indexInBlock = _address % header.bitsInBitMapBlock();
//    blockAddress_tp bitBlock = header.bitMapBegin() + _address / header.bitsInBitMapBlock();
//    return {bitBlock, indexInBlock};
//}
