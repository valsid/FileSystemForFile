#ifndef FILEACCESSOR_H
#define FILEACCESSOR_H

#include "project_exceptions.h"
#include "filesystemblock.h"
#include "constants.h"
#include "blockbuffer.h"

#include <vector>
#include <string>
#include <fstream>

// TODO: refactor buffer

class BlockFileAccessor
{
public:
    BlockFileAccessor() = default;
    virtual ~BlockFileAccessor();

    bool create(std::string path, std::ofstream::pos_type size);
    void open(std::string path, uint64_t _blockSize = 0);
    void close();

    void clearBlock(blockAddress_tp block);
    void clearBlocks(blockAddress_tp begin, blockAddress_tp end);   // inclusive begin and end

    void write(blockAddress_tp offset, const FileSystemBlock *block, uint64_t size = 0);
    void write(blockAddress_tp offset, std::vector<const FileSystemBlock*> data, uint64_t _blockSize);    // TODO:

    template<typename T>
    TypedBufferLocker<T> read(blockAddress_tp offset, SyncType type, uint64_t size = 0) const
    {
        checkOpen();

        if(size == 0) {
            size = getBlockSize();
        } else {
            checkBlockSize(size);
        }
        TypedBufferLocker<T> res = _bufferPool.getLock<T>(offset, const_cast<BlockFileAccessor *>(this), type);
        if(type == SyncType::ReadOnly || type == SyncType::ReadWrite) {
            _read(offset, reinterpret_cast<char *>(res.data()), size);
        }
        return res;
    }

    uint64_t getBlockSize() const;
    blockAddress_tp lastBlockAddress() const;

    void setBlockSize(const uint64_t &blockSize);

    std::ofstream::pos_type fileSize() const;

    void updateFileSize();

protected:
    void updateBlockConfiguration();

    virtual void _write(blockAddress_tp block, const char *buffer, uint64_t size);

    virtual void _read(blockAddress_tp offset, char *buffer, uint64_t size = 0) const;

    void checkOpen(std::string errMessage = "") const;
    void checkValidBlockSize() const;
    void checkBlockSize(unsigned int size) const;

    std::ofstream::pos_type blockToPosType(blockAddress_tp blockAddress) const;

    mutable BlockBufferPool _bufferPool;

private:
    mutable std::fstream _file;
    uint64_t _blockSize = 0;
    blockAddress_tp _lastBlockAddress = 0;
    std::ofstream::pos_type _fileSize = 0;
};

class FormatedFileAccessor : public BlockFileAccessor
{
public:
    const FSHeader *header() const;
    const FSHeader &headerExcept() const;

    void open(std::string path, uint64_t _blockSize = 0);

    bool isFormatedFS() const;
//    void formatFS(uint64_t blockSize, uint64_t .../*other parameters*/);
    void formatFS(const FSHeader &header);

private:
    virtual void _write(blockAddress_tp block, const char *buffer, uint64_t size) final;
    virtual void _read(blockAddress_tp offset, char *buffer, uint64_t size) const final;

    void ensureValidFS() const;

    void syncHeaderToFile();
    void syncHeaderFromFile();

    FSHeader _header;
};

#endif // FILEACCESSOR_H
