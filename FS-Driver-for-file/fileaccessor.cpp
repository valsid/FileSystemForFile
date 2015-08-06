#include "fileaccessor.h"

#include "project_exceptions.h"
using namespace fs_excetion;

#include <cstring>
#include <stdexcept>

BlockFileAccessor::~BlockFileAccessor()
{
    if(_file.is_open()) {
        close();
    }
}

bool BlockFileAccessor::create(std::string path, std::ofstream::pos_type size)
{
    std::ofstream out(path, std::ios_base::out | std::ios_base::binary);
    if(!out) {
        return false;
    }
    out.seekp(size - std::ofstream::pos_type(1));
    out << '\0';
    out.close();
    return true;
}

void BlockFileAccessor::open(std::string path, uint64_t blockSize)
{
    if(_file.is_open()) {
        close();
    }
    _file.open(path, std::ios_base::out | std::ios_base::in | std::ios_base::binary);
    if(!_file) {
        throw file_open_exception("File open error.");
    }
    updateFileSize();
    setBlockSize(blockSize);
}

void BlockFileAccessor::close()
{
    if(!_file.is_open()) {
        return;
    }
    _file.close();
}

void BlockFileAccessor::clearBlock(blockAddress_tp block)
{
    clearBlocks(block, block);
}

void BlockFileAccessor::clearBlocks(blockAddress_tp begin, blockAddress_tp end)
{
    char *zeroBuffer = new char[getBlockSize()];
    memset(zeroBuffer, 0, getBlockSize());

    for(blockAddress_tp i = begin; i <= end; i++) {
        _write(i, zeroBuffer, getBlockSize());
    }
    delete zeroBuffer;
}

void BlockFileAccessor::write(blockAddress_tp offset, const FileSystemBlock *block, uint64_t size)
{
    if(block == nullptr) {
        throw std::invalid_argument("Nullpointer to block.");   // or write zero to file?
    }
    if(block->blockData() == nullptr) {
        throw std::invalid_argument("Nullpointer block data.");
    }
    checkOpen();

    if(size == 0) {
        size = getBlockSize();
    } else {
        checkBlockSize(size);
    }
    _write(offset, block->blockData(), size);
}

void BlockFileAccessor::write(blockAddress_tp offset, std::vector<const FileSystemBlock *> data, uint64_t blockSize)
{
    for(size_t i = 0; i < data.size(); i++) {
        write(offset + i, data.at(i), blockSize);
    }
}

blockAddress_tp BlockFileAccessor::lastBlockAddress() const
{
    checkOpen();
    checkValidBlockSize();
    return _lastBlockAddress;
}

//void FormatedFileAccessor::formatFS(uint64_t blockSize)
//{
//    FSHeader header;
//    header.init();

//    header.blockByteSize  // TODO: todo

//    auto blockSize = headerExcept().blockByteSize;
//    if((_fileSize / blockSize) == 0) {
//        throw bad_state_exception("File too small.");
//    }
//    _lastBlockAddress = (_fileSize / blockSize) - 1;   // block index
//}

void BlockFileAccessor::_write(blockAddress_tp block, const char *buffer, uint64_t size)
{
    _file.seekp(blockToPosType(block));
    _file.write(buffer, size);
    _file.flush();
}

void BlockFileAccessor::_read(blockAddress_tp blockAddr, char *buffer, uint64_t size) const
{
    _file.seekg(blockToPosType(blockAddr));
    _file.read(buffer, size);
}

void BlockFileAccessor::updateBlockConfiguration()
{
    checkOpen();

    if(_blockSize == 0) {
        _lastBlockAddress = 0;
    } else {
        _lastBlockAddress = fileSize() / _blockSize;
    }
}

void BlockFileAccessor::checkOpen(std::string errMessage) const
{
    if(!_file.is_open()) {
        if(errMessage.empty()) {
            errMessage = "File system not mounted";
        }
        throw bad_state_exception(errMessage);
    }
}

void BlockFileAccessor::checkBlockSize(unsigned int size) const
{
    checkValidBlockSize();
    if(size > _blockSize) {
        throw std::invalid_argument("Bad size: " + std::to_string(size) + " > " + std::to_string(_blockSize));
    }
}

void BlockFileAccessor::checkValidBlockSize() const
{
    if(_blockSize == 0) {
        throw bad_state_exception("Block size not installed");
    }
}

std::ofstream::pos_type BlockFileAccessor::blockToPosType(blockAddress_tp blockAddress) const
{
    if(blockAddress == Constants::HEADER_ADDRESS()) {
        return Constants::HEADER_ADDRESS();
    }
    return blockAddress * getBlockSize();
}

void BlockFileAccessor::setBlockSize(const uint64_t &blockSize)
{
    _blockSize = blockSize;
    _bufferPool.setDefaultBuffersSize(blockSize);
    if(_blockSize != 0) {
        updateBlockConfiguration();
    }
}

std::ofstream::pos_type BlockFileAccessor::fileSize() const
{
    checkOpen();
    return _fileSize;
}

void BlockFileAccessor::updateFileSize()
{
    _file.seekg (0, _file.end);
    _fileSize = _file.tellg();
}

uint64_t BlockFileAccessor::getBlockSize() const
{
    checkValidBlockSize();
    return _blockSize;
}

const FSHeader &FormatedFileAccessor::headerExcept() const
{
    checkOpen();
    ensureValidFS();
    return _header;
}

void FormatedFileAccessor::open(std::string path, uint64_t blockSize)
{
    BlockFileAccessor::open(path, blockSize);
    syncHeaderFromFile();
}

const FSHeader *FormatedFileAccessor::header() const
{
    try {
        return &headerExcept();
    } catch(...) {
        return nullptr;
    }
}

void FormatedFileAccessor::syncHeaderToFile()
{
    checkOpen();
    BlockFileAccessor::_write(Constants::HEADER_ADDRESS(), _header.blockData(), sizeof(FSHeader));
}

void FormatedFileAccessor::syncHeaderFromFile()
{
    BlockFileAccessor::_read(Constants::HEADER_ADDRESS(), reinterpret_cast<char *>(&_header), sizeof(FSHeader));
    if(isFormatedFS()){
        setBlockSize(header()->blockByteSize);
    }
}

void FormatedFileAccessor::ensureValidFS() const
{
    if(!isFormatedFS()) {
        throw file_not_formated_exception("This file system is not valid.");
    } else {
        if(_header.version != 1) {
            throw file_not_formated_exception("Unsupported filesystem version.");
        }
    }
}

bool FormatedFileAccessor::isFormatedFS() const
{
    return _header.signature.isValidSignature();
}

void FormatedFileAccessor::formatFS(const FSHeader &header)
{
    if(!header.signature.isValidSignature()) {
        throw bad_state_exception("Bad header");
    }
    _header = header;
    syncHeaderToFile();
    setBlockSize(_header.blockByteSize);
}

void FormatedFileAccessor::_write(blockAddress_tp block, const char *buffer, uint64_t size)
{
    if(block == Constants::HEADER_ADDRESS()) {
        throw std::invalid_argument("Bad block address. Attempt read from header block.");
    }
    ensureValidFS();
    BlockFileAccessor::_write(block, buffer, size);
}

void FormatedFileAccessor::_read(blockAddress_tp offset, char *buffer, uint64_t size) const
{
    if(offset == Constants::HEADER_ADDRESS()) {
        throw std::invalid_argument("Bad block address. Attempt read from header block.");
    }
    ensureValidFS();
    return BlockFileAccessor::_read(offset, buffer, size);
}
