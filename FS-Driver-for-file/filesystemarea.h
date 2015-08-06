#ifndef FILESYSTEMAREA_H
#define FILESYSTEMAREA_H

#include <memory>
using std::shared_ptr;

class FileSystem;

#include "fileaccessor.h"
#include "blockbuffer.h"
#include "constants.h"

class FileSystemArea
{
public:
    FileSystemArea(shared_ptr<FormatedFileAccessor> fsFile);
    virtual ~FileSystemArea() = default;

    bool inRange(const blockAddress_tp &address) const;
    virtual blockAddress_tp areaBegin() const = 0;
    virtual blockAddress_tp areaEnd() const = 0;

    virtual void initBlocks();

protected:
    const FSHeader &header() const;
    std::shared_ptr<FormatedFileAccessor> file() const;

    std::shared_ptr<FormatedFileAccessor> _fsFile;

    template<typename T>
    TypedBufferLocker<T> readToBuff(blockAddress_tp address, SyncType type) {
        if(inRange(address)) {
            return file()->read<T>(address, type);
        } else {
            throw std::invalid_argument("Bad argument in FileSystemArea::readToBuf: " + std::to_string(address));
        }
    }
// TODO:
//    static std::pair<blockAddress_tp, uint64_t> globalPosFromIndex(blockAddress_tp index, blockAddress_tp areaBeginBlock, uint64_t elementsInBlock);
//    static blockAddress_tp addressFromGlobalPos(blockAddress_tp bitMapBlockAddress, uint64_t offsetInBlock, blockAddress_tp areaBeginBlock, uint64_t elementsInBlock);
};


class BitMapArea : public FileSystemArea {
public:
    BitMapArea(shared_ptr<FormatedFileAccessor> fsFile);

    bool get(blockAddress_tp block);
    void set(blockAddress_tp blockAddr, const bool &value);

    // inclusive
    blockAddress_tp findFirstFreeBlock(blockAddress_tp areaBegin, blockAddress_tp areaEnd);

    virtual blockAddress_tp areaBegin() const;
    virtual blockAddress_tp areaEnd() const;

    virtual void initBlocks();

protected:
    std::pair<blockAddress_tp, uint64_t> bitMapPosFromBlock(blockAddress_tp blockAddress);
    blockAddress_tp blockPosFromBitMapPos(blockAddress_tp bitMapBlockAddress, uint64_t offsetInBlock);
};

class DescriptorsArea : public FileSystemArea {
public:
    DescriptorsArea(shared_ptr<FormatedFileAccessor> fsFile);

    FSDescriptor getDescriptor(descriptorIndex_tp descriptorIndex, DescriptorVariant type = DescriptorVariant::Any) const;
    void updateDescriptor(descriptorIndex_tp descriptorIndex, const FSDescriptor &descriptor);

    descriptorIndex_tp appendDescriptor(blockAddress_tp freePlaceAddress, const FSDescriptor &descriptor, bool *filled = nullptr);

    virtual blockAddress_tp areaBegin() const;
    virtual blockAddress_tp areaEnd() const;

    virtual void initBlocks();

    void incrementReference(descriptorIndex_tp index);
    void decrementReference(descriptorIndex_tp index);

protected:
    std::pair<blockAddress_tp, uint64_t> descriptorPosFromBlock(descriptorIndex_tp descriptorIndex) const;
    descriptorIndex_tp blockPosToDescriptorIndex(blockAddress_tp bitMapBlockAddress, uint64_t indexInBlock);
};

class DataArea : public FileSystemArea {
public:
    DataArea(shared_ptr<FormatedFileAccessor> fsFile);

    virtual blockAddress_tp areaBegin() const;
    virtual blockAddress_tp areaEnd() const;

    template <typename T = FSDataBlock>
    TypedBufferLocker<T> readData(blockAddress_tp block, SyncType type) const
    {
        if(!inRange(block)) {
            throw std::invalid_argument("Bad block in DataArea::readDataToBuffer: " + std::to_string(block));
        }
        return file()->read<T>(block, type);
    }

    void writeDataFromBuffer(blockAddress_tp block, BlockBufferLocker &locker);
};


#endif // FILESYSTEMAREA_H
