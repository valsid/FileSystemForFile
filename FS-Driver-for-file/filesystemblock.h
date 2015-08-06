#ifndef FILESYSTEMBLOCK_H
#define FILESYSTEMBLOCK_H

#include "constants.h"
#include <cstring>
#include <string>

using descriptorEnum_tp = uint8_t;  // must be unsigned integer. Produce undefined behavior for signed integers
enum class DescriptorVariant : descriptorEnum_tp {None = 0x00, File = 0x01, SymLink = 0x02, Directory = 0x04, Any = 0x0F};

DescriptorVariant operator | (const DescriptorVariant &first, const DescriptorVariant &second);
descriptorEnum_tp operator & (const DescriptorVariant &first, const DescriptorVariant &second);

namespace std {
std::string to_string(DescriptorVariant var);
}

struct Signature {
    Constants::mark_t _mark;

    bool operator==(const Signature &other) const {
        for(size_t i = 0; i < Constants::magicMarkSize; i++) {
            if(_mark[i] != other._mark[i]) {
                return false;
            }
        }
        return true;
    }

    bool isValidSignature() const {
        return *this == getSignature();
    }

    static Signature getSignature() {
        Signature m;
        memcpy(&m._mark, Constants::mMark, Constants::magicMarkSize);
        return m;
    }
};

//class FSBitMapBlock;

class FileSystemBlock
{
public:
    const char *blockData() const {
        return reinterpret_cast<const char*>(this);
    }

    std::string toString(const FSHeader &) const;

//    FSBitMapBlock *atBitMap() {
//        return reinterpret_cast<FSBitMapBlock*>(this);
//    }
};

class FSBitMapBlock : public FileSystemBlock
{
public:
    uint64_t findFirstZero(uint64_t begin, uint64_t maxBits) const;
    void set(uint64_t bit, bool data);
    bool get(uint64_t bit) const;
    void reset();

    std::string toString(const FSHeader &data) const;

private:
    byte_tp bits[Constants::blockByteSize()];
};


struct directoryEntry {  // 22 bytes
    descriptorIndex_tp descriptor;

    void set(descriptorIndex_tp descriptor, std::string name, filenameLength_tp length);
    std::string name(filenameLength_tp maxFilename) const;

    std::string toString(uint64_t maxLength) const;
    bool isEmpty() const;
    void clear();

// private:
    char __name[14];
};

// TODO: make larger descriptor size
class FSDescriptor // 128 bytes
{
public:
    bool isHaveExtendedSegment();
    std::string toString(const FSHeader &data) const;

    void init() {
        type = DescriptorVariant::None;
        referencesCount = 0;
        nextDataSegment = Constants::HEADER_ADDRESS();
        firstFreeElementIndex = 0;
    }

    void initFile() {
        init();
        type = DescriptorVariant::File;
        fileSize = 0;
    }

    void initDirectory(blockAddress_tp parent) {
//        uint64_t directoryElementsCount = sizeof(directoryElements) / sizeof(directoryElements[0]);
        init();
        type = DescriptorVariant::Directory;
        this->parent = parent;
    }

    DescriptorVariant type;
    int64_t referencesCount;    // 8 - bits in byte
    blockAddress_tp nextDataSegment;
    uint64_t firstFreeElementIndex;

    static constexpr int fullDescriptorSize = 128;
    static constexpr int otherDataSize = 8 * 4;

    union {
        struct {
            byte_tp byteSizeReserve[fullDescriptorSize - otherDataSize];
        };
        struct { // for file
            uint64_t fileSize;
            blockAddress_tp dataSegments[(sizeof(byteSizeReserve) - sizeof(fileSize) - sizeof(firstFreeElementIndex)) / sizeof(blockAddress_tp)];
        };
        struct {    // for directory
            blockAddress_tp parent;
            directoryEntry directoryEntries[(sizeof(byteSizeReserve) - sizeof(parent) - sizeof(firstFreeElementIndex)) / sizeof(directoryEntry)];
        };
        struct {    // for symlink
            char symlinkPath[sizeof(byteSizeReserve)];
        };
    };
};

class FSDescriptorsContainerBlock : public FileSystemBlock
{
public:
    std::string toString(const FSHeader &data) const;

    FSDescriptor descriptors[Constants::blockByteSize() / sizeof(FSDescriptor)];
};

class FSDescriptorDataPart : public FileSystemBlock
{
public:
    std::string toString(const FSHeader &data) const;
    bool isHaveExtendedSegment();

    void init() {
        nextSegment = Constants::HEADER_ADDRESS();
    }

    blockAddress_tp nextSegment;
    union {
        byte_tp byteSizeReserve[Constants::blockByteSize() - sizeof(nextSegment)];

        // File
        blockAddress_tp dataBlocks[sizeof(byteSizeReserve) / sizeof(blockAddress_tp)];

        // Folder
        directoryEntry  directoryEntries[sizeof(byteSizeReserve) / sizeof(directoryEntry)];
    };
};

class FSDataBlock : public FileSystemBlock
{
public:
    std::string toString(const FSHeader &) const;

    byte_tp data[Constants::blockByteSize()];
};

#define ARRAY_LENGTH(T) sizeof(T) / sizeof(T[0])

// TODO: program not fully support FS with other structures size

class FSHeader : public FileSystemBlock    // first file system block
{
public:
    void init() {
        signature = Signature::getSignature();
        version = 1;

        blockByteSize = Constants::blockByteSize();
        filenameLength = sizeof(directoryEntry::__name);
        rootDirectoryDescriptor = 1;

        descriptorSize = sizeof(FSDescriptor);

        // TODO:
        blockLinkCountInFileDescriptor = ARRAY_LENGTH(FSDescriptor::dataSegments);
        entriesInDirectoryDescriptor = ARRAY_LENGTH(FSDescriptor::directoryEntries);

        blockLinkCountInFileBlock = ARRAY_LENGTH(FSDescriptorDataPart::dataBlocks);
        entriesInDirectoryBlock = ARRAY_LENGTH(FSDescriptorDataPart::directoryEntries);

        _bitMapBegin = Constants::HEADER_ADDRESS();
        _bitMapEnd = Constants::HEADER_ADDRESS();
        _descriptorsEnd = Constants::HEADER_ADDRESS();
        _dataEnd = Constants::HEADER_ADDRESS();
    }

    Signature signature;
    std::uint32_t version;
    filenameLength_tp filenameLength;

    uint64_t blockByteSize;
    uint64_t descriptorSize;

    descriptorIndex_tp rootDirectoryDescriptor;

    uint64_t blockLinkCountInFileDescriptor;
    uint64_t blockLinkCountInFileBlock;

    uint64_t entriesInDirectoryDescriptor;
    uint64_t entriesInDirectoryBlock;

    blockAddress_tp _bitMapBegin;
    blockAddress_tp _bitMapEnd;
    blockAddress_tp _descriptorsEnd;
    blockAddress_tp _dataEnd;

    blockAddress_tp bitMapBegin() const {
        return _bitMapBegin;
    }
    blockAddress_tp bitMapEnd() const {
        return _bitMapEnd;
    }
    blockAddress_tp descriptorsBegin() const {
        return bitMapEnd() + 1;
    }
    blockAddress_tp descriptorsEnd() const {
        return _descriptorsEnd;
    }
    blockAddress_tp dataBlockBegin() const {
        return descriptorsEnd() + 1; //.nextBlock();
    }
    blockAddress_tp dataBlockEnd() const {
        return _dataEnd;
    }

    uint64_t descriptorsInBlock() const {
        return blockByteSize / descriptorSize;
    }

    uint64_t bitsInBitMapBlock() const {
        constexpr int bitsInByte = 8;
        return blockByteSize * bitsInByte;
    }

    std::string toString() const {
        std::string str;
        using std::to_string;
        str += "Version: " + to_string(version);
        str += "\nBlock size: " + to_string(blockByteSize);
        str += "\nDescriptor size: " + to_string(descriptorSize);
        str += "\nRoot directory descriptor: " + to_string(rootDirectoryDescriptor);
        str += "\nMax filename: " + to_string(filenameLength);
        str += "\nBlocks: [" + to_string(bitMapBegin()) + ":" + to_string(bitMapEnd()) + "] - ";
        str += "[" + to_string(descriptorsBegin()) + ":" + to_string(descriptorsEnd()) + "] - ";
        str += "[" + to_string(dataBlockBegin()) + ":" + to_string(dataBlockEnd()) + "]\n";

        return str;
    }
};

#endif // FILESYSTEMBLOCK_H                                         
