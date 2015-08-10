#include "blockbuffer.h"

#include <stdexcept>
#include "fileaccessor.h"

#include <iostream>     // for logs

constexpr bool logEnabled = false;

BlockBufferLocker::BlockBufferLocker(BlockBufferPool *bufferPool, BlockFileAccessor *file, blockAddress_tp block, SyncType type) :
    _bufferPool(bufferPool),
    _blockAddress(block),
    _fsFile(file),
    _type(type)
{
    if(bufferPool == nullptr) {
        throw std::invalid_argument("Nullpointer to buffer pool");
    }
    if(block == Constants::HEADER_ADDRESS()) {
        throw std::invalid_argument("Bad block address in BlockBufferLocker.");
    }
    if(file == nullptr && (type == SyncType::WriteOnly || type == SyncType::ReadWrite)) {
        throw std::invalid_argument("Nullpointer to file, lol.");
    }
}

BlockBufferLocker::BlockBufferLocker()
{
    _isValid = false;
}

byte_tp *BlockBufferLocker::data() const
{
    ensureValid();
    if(_blockBuffer == nullptr) {
        _blockBuffer = _bufferPool->getAnyFreeBuffer();
    }
    return _blockBuffer;
}

uint64_t BlockBufferLocker::length() const
{
    ensureValid();
    return _bufferPool->bufferSize();
}

void BlockBufferLocker::flush()
{
    if((_type == SyncType::WriteOnly || _type == SyncType::ReadWrite) && isValid()) {
        _fsFile->write(_blockAddress, atBlock());
    }
}

bool BlockBufferLocker::isValid() const
{
    return _isValid;
}

void BlockBufferLocker::ensureValid() const
{
    if(!isValid()) {
        throw std::invalid_argument("\tBuffer is not initialized.");
    }
}
SyncType BlockBufferLocker::syncType() const
{
    return _type;
}

void BlockBufferLocker::setSyncType(const SyncType &type)
{
    _type = type;
}

BlockBufferLocker::~BlockBufferLocker()
{
    flush();
    if(isValid()) {
        _bufferPool->unlockBuffer(_blockBuffer);
    }
}

BlockBufferLocker::BlockBufferLocker(BlockBufferLocker &&other)
{
    *this = std::move(other);
}

BlockBufferLocker &BlockBufferLocker::operator =(BlockBufferLocker &&other)
{
    if(&other == this) {
        return *this;
    }

    this->flush();

    _type = std::move(other._type);
    _fsFile = std::move(other._fsFile);
    _bufferPool = std::move(other._bufferPool);
    _blockBuffer = std::move(other._blockBuffer);
    _blockAddress = std::move(other._blockAddress);
    _isValid = std::move(other._isValid);

    other._isValid = false;

    return *this;
}

void BlockBufferPool::setDefaultBuffersSize(uint64_t size)
{
    if(!_occupied.empty()) {
        throw bad_state_exception("You cannot change buffer size if you have occupied buffers.");
    }
    if(logEnabled) {
        std::clog << "\tSet block size: " << _bufferSize << " -> " << size << "\n";
    }
    if(_bufferSize != size) {
        _bufferSize = size;
        while(!_free.empty()) {
            delete _free.front();
            _free.pop_front();
        }
    }
}

uint64_t BlockBufferPool::bufferSize() const
{
    return _bufferSize;
}

byte_tp *BlockBufferPool::getAnyFreeBuffer()
{
    if(logEnabled) {
        std::clog << "\tlock\n";
    }
    return _free.empty() ? lockNewBuffer() : lockFreeBuffer();
}

void BlockBufferPool::unlockBuffer(byte_tp *buffer)
{
    if(logEnabled) {
        std::clog << "\tunlock " << buffer << "\n";
    }
    _occupied.erase(buffer);
    _free.push_front(buffer);
}

byte_tp *BlockBufferPool::lockFreeBuffer()
{
    byte_tp *result = _free.front();
    if(logEnabled) {
        std::clog << "\tUse old buffer: " << result << "\n";
    }
    _free.pop_front();
    _occupied.insert(result);
    return result;
}

byte_tp *BlockBufferPool::lockNewBuffer()
{
    byte_tp *result = new byte_tp[_bufferSize];
    if(logEnabled) {
        std::clog << "\tCreate new buffer[" << _bufferSize << "] -> " << static_cast<void*>(result) << "\n";
    }
    _occupied.insert(result);
    return result;
}
