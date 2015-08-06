#ifndef BLOCKBUFFER_H
#define BLOCKBUFFER_H

#include "project_exceptions.h"
using namespace fs_excetion;

#include "filesystemblock.h"
#include "constants.h"

#include <memory>
#include <list>
#include <unordered_set>

class BlockFileAccessor;
class BlockBufferPool;

enum class SyncType {None = 0, ReadOnly = 1, WriteOnly = 2, ReadWrite = 3};
/**
 * @brief The BlockBufferLocker class
 * Writes changes to file before destruct
 */
class BlockBufferLocker
{
    BlockBufferLocker(BlockBufferLocker &other) = delete;
    const BlockBufferLocker &operator=(const BlockBufferLocker &other) const = delete;
    BlockBufferLocker operator=(const BlockBufferLocker &other) = delete;

protected:
    BlockBufferLocker(BlockBufferPool *bufferPool, BlockFileAccessor *file, blockAddress_tp block,
                      SyncType syncType);
public:
    BlockBufferLocker();
    ~BlockBufferLocker();
    BlockBufferLocker(BlockBufferLocker &&other);
    BlockBufferLocker &operator =(BlockBufferLocker &&other);

    byte_tp *data() const;
    uint64_t length() const;

    bool isValid() const;
    SyncType syncType() const;
    void setSyncType(const SyncType &syncType);

    void flush();

    template<typename T = FileSystemBlock>
    T *atBlock() const {
        static_assert(std::is_base_of<FileSystemBlock, T>::value, "T must be derived from FileSystemBlock");
        return reinterpret_cast<T*>(data());
    }

private:
    void ensureValid() const;

    // mutable for lazy initialize
    mutable byte_tp *_blockBuffer = nullptr;
    BlockBufferPool *_bufferPool;

    blockAddress_tp _blockAddress;
    BlockFileAccessor *_fsFile;
    SyncType _type;
    bool _isValid = true;
};

template<typename T>
class TypedBufferLocker : public BlockBufferLocker
{
    friend class BlockBufferPool;

    TypedBufferLocker(BlockBufferPool *pool, BlockFileAccessor *file, blockAddress_tp block, SyncType type) :
        BlockBufferLocker(pool, file, block, type)
    {
        static_assert(std::is_base_of<FileSystemBlock, T>::value, "T must be derived from FileSystemBlock");
    }

public:
    TypedBufferLocker()
    { }

    T &operator *() {
        return *block();
    }

    T *operator &() {
        return block();
    }

    T *operator ->() {
        return block();
    }

private:
    T *block() const
    {
        return atBlock<T>();
    }
};

class BlockBufferPool {
public:
    void setDefaultBuffersSize(uint64_t size);
    uint64_t bufferSize() const;

    template<typename T>
    TypedBufferLocker<T> getLock(blockAddress_tp block, BlockFileAccessor *file, SyncType type)
    {
        return TypedBufferLocker<T>(this, file, block, type);
    }

private:
    friend class BlockBufferLocker;
        byte_tp *getAnyFreeBuffer();
        void unlockBuffer(byte_tp *buffer);

    byte_tp *lockFreeBuffer();
    byte_tp *lockNewBuffer();

    uint64_t _bufferSize = 0;
    mutable std::list<byte_tp *> _free;
    mutable std::unordered_set<byte_tp *> _occupied;    // debug
};


#endif // BLOCKBUFFER_H
