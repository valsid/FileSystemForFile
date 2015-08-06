#include "filesystemarea.h"

#include "fileaccessor.h"

#include <cassert>

FileSystemArea::FileSystemArea(shared_ptr<FormatedFileAccessor> fsFile) :
    _fsFile(fsFile)
{
}

const FSHeader &FileSystemArea::header() const
{
    return _fsFile->headerExcept();
}

std::shared_ptr<FormatedFileAccessor> FileSystemArea::file() const
{
    return _fsFile;
}

bool FileSystemArea::inRange(const blockAddress_tp &address) const
{
    assert(areaBegin() <= areaEnd());
    return address >= areaBegin() && address <= areaEnd();
}

void FileSystemArea::initBlocks()
{

}

//std::pair<blockAddress_tp, uint64_t> FileSystemArea::globalPosFromIndex(blockAddress_tp index, blockAddress_tp areaBeginBlock, uint64_t elementsInBlock)
//{
//    // Constants::INVALID_DESCRIPTOR
//    if(index == Constants::HEADER_ADDRESS() || !inRange(index)) {
//        throw std::invalid_argument("Bad argument in BitMapArea::bitMapPosFromBlock: " + std::to_string(index));
//    }

//    int64_t indexInBlock = index % elementsInBlock;
//    blockAddress_tp bitMapBlock = areaBeginBlock + index / elementsInBlock;
//    return {bitMapBlock, indexInBlock};
//}

//blockAddress_tp FileSystemArea::addressFromGlobalPos(blockAddress_tp position, uint64_t offset, blockAddress_tp areaBeginBlock, uint64_t elementsInBlock)
//{
//    // TODO: check
//    if(inRange(position) && offset < header().bitsInBitMapBlock()) {
//        return position * elementsInBlock + offset;
//    } else {
//        throw std::invalid_argument("Bad argument in BitMapArea::blockPosFromBitMapPos: " + std::to_string(position) + ":" + std::to_string(offset));
//    }
//}

DescriptorsArea::DescriptorsArea(shared_ptr<FormatedFileAccessor> fsFile) :
    FileSystemArea(fsFile)
{

}

FSDescriptor DescriptorsArea::getDescriptor(descriptorIndex_tp descriptorIndex, DescriptorVariant type) const
{
    auto descriptorBlockPos = descriptorPosFromBlock(descriptorIndex);

    TypedBufferLocker<FSDescriptorsContainerBlock> block =
            file()->read<FSDescriptorsContainerBlock>(descriptorBlockPos.first, SyncType::ReadOnly);

    DescriptorVariant dType = block->descriptors[descriptorBlockPos.second].type;
    if(type != DescriptorVariant::Any && (dType & type) != 0) {
        throw file_system_exception("Bad descriptor type. " + std::to_string(type) + " is not a " + std::to_string(dType) + ".");
    }

    return block->descriptors[descriptorBlockPos.second];
}

void DescriptorsArea::updateDescriptor(descriptorIndex_tp descriptorIndex, const FSDescriptor &descriptor)
{
    auto descriptorBlockPos = descriptorPosFromBlock(descriptorIndex);

    TypedBufferLocker<FSDescriptorsContainerBlock> block =
            file()->read<FSDescriptorsContainerBlock>(descriptorBlockPos.first, SyncType::ReadWrite);

    block->descriptors[descriptorBlockPos.second] = descriptor;
}

descriptorIndex_tp DescriptorsArea::appendDescriptor(blockAddress_tp freePlaceAddress, const FSDescriptor &descriptor, bool *filled)
{
    if(!inRange(freePlaceAddress)) {
        throw std::invalid_argument("Bad address in DescriptorsArea::appendDescriptor: " + std::to_string(freePlaceAddress));
    }

    TypedBufferLocker<FSDescriptorsContainerBlock> block =
            file()->read<FSDescriptorsContainerBlock>(freePlaceAddress, SyncType::ReadWrite);

    uint64_t freeDescriptor = 0;

    assert(Constants::INVALID_DESCRIPTOR_ID() == 0);
    if(freePlaceAddress == header().descriptorsBegin()) {
        freeDescriptor = 1;
    }

    for(uint64_t i = freeDescriptor; i < header().descriptorsInBlock(); i++) {
        if(block->descriptors[i].type == DescriptorVariant::None) {
            freeDescriptor = i;
            break;
        }
    }

    block->descriptors[freeDescriptor] = descriptor;

    if((freeDescriptor + 1) == header().descriptorsInBlock()) {
        *filled = true;
    }

    return freeDescriptor;
}

blockAddress_tp DescriptorsArea::areaBegin() const
{
    return header().descriptorsBegin();
}

blockAddress_tp DescriptorsArea::areaEnd() const
{
    return header().descriptorsEnd();
}

void DescriptorsArea::initBlocks()
{
    file()->clearBlocks(areaBegin(), areaEnd());
//    std::unique_ptr<FSDescriptorsContainerBlock> pureDescriptorBlock(
//                reinterpret_cast<FSDescriptorsContainerBlock*>(new char[header().blockByteSize]));

//    for(size_t i = 0; i < header().descriptorsInBlock(); i++) {
//        pureDescriptorBlock->descriptors[i].init();
//    }

//    for(auto i = areaBegin(); i <= areaEnd(); ++i) {
//        file()->write(i, pureDescriptorBlock.get(), header().blockByteSize);
//    }
}

void DescriptorsArea::incrementReference(descriptorIndex_tp index)
{
    FSDescriptor descriptor = getDescriptor(index);
    descriptor.referencesCount++;
    updateDescriptor(index, descriptor);
}

void DescriptorsArea::decrementReference(descriptorIndex_tp index)
{
    FSDescriptor descriptor = getDescriptor(index);
    if(descriptor.type == DescriptorVariant::Directory && descriptor.firstFreeElementIndex != 0) {
        throw file_system_exception("Unable to remove directory. Directory not empty.");
    }
    descriptor.referencesCount--;
    updateDescriptor(index, descriptor);
}

std::pair<blockAddress_tp, uint64_t> DescriptorsArea::descriptorPosFromBlock(descriptorIndex_tp descriptorIndex) const
{
    if(descriptorIndex == Constants::INVALID_DESCRIPTOR_ID()) {
        throw std::invalid_argument("Bad argument in DescriptorsArea::descriptorPosFromBlock: " + std::to_string(descriptorIndex));
    }

    int64_t indexInBlock = descriptorIndex % header().descriptorsInBlock();
    blockAddress_tp descriptorBlock = header().descriptorsBegin()  + descriptorIndex / header().descriptorsInBlock();

    if(!inRange(descriptorBlock)) {
        throw std::invalid_argument("Bad argument in DescriptorsArea::descriptorPosFromBlock: " + std::to_string(descriptorIndex));
    }

    return {descriptorBlock, indexInBlock};
}

descriptorIndex_tp DescriptorsArea::blockPosToDescriptorIndex(blockAddress_tp bitMapBlockAddress, uint64_t indexInBlock)
{
    if(inRange(bitMapBlockAddress) && indexInBlock < header().descriptorsInBlock()) {
        return bitMapBlockAddress * header().descriptorsInBlock() + indexInBlock;
    }
    throw std::invalid_argument("Bad argument in DescriptorsArea::blockPosToDescriptorIndex: " + std::to_string(bitMapBlockAddress) + ":" + std::to_string(indexInBlock));
}

BitMapArea::BitMapArea(shared_ptr<FormatedFileAccessor> fsFile) :
    FileSystemArea(fsFile)
{
}

bool BitMapArea::get(blockAddress_tp blockAddr)
{
    auto localPos = bitMapPosFromBlock(blockAddr);

    TypedBufferLocker<FSBitMapBlock> bitBlock = file()->read<FSBitMapBlock>(localPos.first, SyncType::ReadOnly);
    return bitBlock->get(localPos.second);
}

void BitMapArea::set(blockAddress_tp blockAddr, const bool &value)
{
    auto localPos = bitMapPosFromBlock(blockAddr);

    TypedBufferLocker<FSBitMapBlock> bitBlock = file()->read<FSBitMapBlock>(localPos.first, SyncType::ReadWrite);

    bitBlock->set(localPos.second, value);
}

blockAddress_tp BitMapArea::findFirstFreeBlock(blockAddress_tp begin, blockAddress_tp end)
{
    const auto beginBlockAddr  = bitMapPosFromBlock(begin);
    const auto endBitBlockAddr = bitMapPosFromBlock(end);

    if(beginBlockAddr.second != 0) {
        TypedBufferLocker<FSBitMapBlock> bitBlock = readToBuff<FSBitMapBlock>(beginBlockAddr.first, SyncType::ReadOnly);
        uint64_t endBit = (beginBlockAddr.first == endBitBlockAddr.first) ? endBitBlockAddr.second + 1 : header().bitsInBitMapBlock();
        auto offset = bitBlock->findFirstZero(beginBlockAddr.second, endBit);
        if(offset == endBit) {
            if(beginBlockAddr.first == endBitBlockAddr.first) {
                return Constants::HEADER_ADDRESS();
            }
        } else {
            return blockPosFromBitMapPos(beginBlockAddr.first, offset);
        }
    }


    for(auto i = beginBlockAddr.first + 1; i < endBitBlockAddr.first; i++) {
        TypedBufferLocker<FSBitMapBlock> bitBlock = readToBuff<FSBitMapBlock>(i, SyncType::ReadOnly);
        auto offset = bitBlock->findFirstZero(0, header().bitsInBitMapBlock());
        if(offset != header().bitsInBitMapBlock()) {
            return blockPosFromBitMapPos(i, offset);
        }
    }

    if(endBitBlockAddr.second != 0) {
        TypedBufferLocker<FSBitMapBlock> bitBlock = readToBuff<FSBitMapBlock>(end, SyncType::ReadOnly);
        auto offset = bitBlock->findFirstZero(0, endBitBlockAddr.second + 1);       // .second != limits.max()
        if(offset != endBitBlockAddr.second + 1) {
            return blockPosFromBitMapPos(endBitBlockAddr.first, offset);
        }
    }

    return Constants::HEADER_ADDRESS();
}

blockAddress_tp BitMapArea::areaBegin() const
{
    return header().bitMapBegin();
}

blockAddress_tp BitMapArea::areaEnd() const
{
    return header().bitMapEnd();
}

void BitMapArea::initBlocks()
{
    file()->clearBlocks(areaBegin(), areaEnd());
}

std::pair<blockAddress_tp, uint64_t> BitMapArea::bitMapPosFromBlock(blockAddress_tp blockAddress)
{
    if(blockAddress == Constants::HEADER_ADDRESS()) {
        throw std::invalid_argument("Bad argument in BitMapArea::bitMapPosFromBlock: " + std::to_string(blockAddress));
    }
    uint64_t entryInBlock = header().bitsInBitMapBlock();
    uint64_t beginBlock = header().bitMapBegin();

//    blockAddress -= header().descriptorsBegin();

    int64_t indexInBlock = blockAddress % entryInBlock;
    blockAddress_tp bitMapBlock = beginBlock + blockAddress / entryInBlock;
    return {bitMapBlock, indexInBlock};
}

blockAddress_tp BitMapArea::blockPosFromBitMapPos(blockAddress_tp bitMapBlockAddress, uint64_t offsetInBlock)
{
    if(inRange(bitMapBlockAddress) && offsetInBlock < header().bitsInBitMapBlock()) {
        return (bitMapBlockAddress - header().bitMapBegin()) * header().bitsInBitMapBlock() + offsetInBlock;
    } else {
        throw std::invalid_argument("Bad argument in BitMapArea::blockPosFromBitMapPos: " + std::to_string(bitMapBlockAddress) + ":" + std::to_string(offsetInBlock));
    }
}

DataArea::DataArea(shared_ptr<FormatedFileAccessor> fsFile) :
    FileSystemArea(fsFile)
{
}

blockAddress_tp DataArea::areaBegin() const
{
    return header().dataBlockBegin();
}

blockAddress_tp DataArea::areaEnd() const
{
    return header().dataBlockEnd();
}

void DataArea::writeDataFromBuffer(blockAddress_tp block, BlockBufferLocker &locker)
{
    file()->write(block, locker.atBlock(), header().blockByteSize);
}

