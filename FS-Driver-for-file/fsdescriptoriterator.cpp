#include "fsdescriptoriterator.h"

#include <stdexcept>
#include <cassert>
#include <limits>

DirectoryDescriptorIterator::DirectoryDescriptorIterator(descriptorIndex_tp descriptorIndex, FSDescriptor descriptorBlockData, const directoryEntryIndex_tp entriesInDescriptor, const directoryEntryIndex_tp entriesInBlock, readSegmentFunction_tp readFunction) :
    DirectoryDescriptorIterator(descriptorIndex, descriptorBlockData, entriesInDescriptor, entriesInBlock, readFunction, [](const FSDescriptor&){})
{
    _syncDescriptor = false;
}

DirectoryDescriptorIterator::DirectoryDescriptorIterator(descriptorIndex_tp descriptorIndex, FSDescriptor descriptorBlockData, const directoryEntryIndex_tp entriesInDescriptor, const directoryEntryIndex_tp entriesInBlock, readSegmentFunction_tp readFunction, updateDescriptorFunction_tp updateFumtion) :
    _syncDescriptor(true),
    _descriptorHandle(descriptorIndex),
    _descriptorData(descriptorBlockData),
    _entriesInDescriptor(entriesInDescriptor),
    _entriesInBlock(entriesInBlock),
    _readSegment(readFunction),
    _updateDescriptor(updateFumtion)
{
    static_assert(sizeof(directoryEntry::__name) >= 3, "filename too small for '..' element.");
    _dotEntry.set(_descriptorHandle, ".", 2);
    _dotDotEntry.set(_descriptorData.parent, "..", 3);
}


// becose TypedBufferLocker not have copy constructor
// TODO: fix it
DirectoryDescriptorIterator::DirectoryDescriptorIterator(const DirectoryDescriptorIterator &other) :
    _currentIndex(other._descriptorHandle),
    _currentOffsetInBlock(other._currentOffsetInBlock),
    _currentBlockAddress(other._currentBlockAddress),
    _prevBlockAddress(other._prevBlockAddress),
    _syncDescriptor(other._syncDescriptor),
    _descriptorHandle(other._descriptorHandle),
    _descriptorData(other._descriptorData),
    _entriesInDescriptor(other._entriesInDescriptor),
    _entriesInBlock(other._entriesInBlock),
    _readSegment(other._readSegment),
    _updateDescriptor(other._updateDescriptor)
{
    if(_currentBlockAddress != Constants::HEADER_ADDRESS()) {
        _currentBlock = _readSegment(_currentBlockAddress);
    }
}

DirectoryDescriptorIterator::DirectoryDescriptorIterator(const DirectoryDescriptorIterator &other, bool syncDescriptor) :
    DirectoryDescriptorIterator(other)
{
    _syncDescriptor = syncDescriptor;
}


DirectoryDescriptorIterator::~DirectoryDescriptorIterator()
{
//    if(_syncDescriptor) {
//        _updateDescriptor(_descriptorData);
//    }
}

bool DirectoryDescriptorIterator::hasNext()
{
    //    if(_currentOffsetInBlock + 1 == currentBlockEntriesLimit()) {
    //        return currentBlockNextSegmentAddress() != Constants::HEADER_ADDRESS();
    //    } else {
    return _currentIndex + 1 < _descriptorData.firstFreeElementIndex || _currentIndex + 1 == _dotPosition || _currentIndex + 1 == _dotDotPosition;
    //    }
}

bool DirectoryDescriptorIterator::operator ==(const DirectoryDescriptorIterator &other)
{
    return std::tie(_currentIndex, _descriptorHandle) == std::tie(other._currentIndex, other._descriptorHandle);
}

bool DirectoryDescriptorIterator::operator !=(const DirectoryDescriptorIterator &other)
{
    return !operator ==(other);
}

std::iterator<std::forward_iterator_tag, directoryEntry>::reference DirectoryDescriptorIterator::operator *()
{
    switch (_currentOffsetInBlock) {
    case _badPosition:
        throw bad_state_exception("Invalid iterator state.");
    case _dotPosition:
        return _dotEntry;
    case _dotDotPosition:
        return _dotDotEntry;
    default:
        return currentEntryArray()[_currentOffsetInBlock];
    }
}

std::iterator<std::forward_iterator_tag, directoryEntry>::pointer DirectoryDescriptorIterator::operator ->()
{
    return &(this->operator *());
}

DirectoryDescriptorIterator &DirectoryDescriptorIterator::operator++()
{
    assert(_currentIndex + 1 != _badPosition);

    if(!hasNext()) {
        throw std::out_of_range("No such element.");
    }

    if(_currentIndex + 1 == currentBlockEntriesLimit()) {
        _currentOffsetInBlock = 0;
        if(!weInDescriptor()) {
            _prevBlockAddress = _currentBlockAddress;
        }
        _currentBlockAddress = currentBlockNextSegmentAddress();
        _currentBlock = _readSegment(_currentBlockAddress);
    } else {
        _currentOffsetInBlock++;
    }
    _currentIndex++;

    return *this;
}

DirectoryDescriptorIterator DirectoryDescriptorIterator::operator++(int)
{
    DirectoryDescriptorIterator tmp = *this;
    this->operator ++();
    return tmp;
}

void DirectoryDescriptorIterator::flush()
{
    flushDesciptor();
    _currentBlock.flush();
}

void DirectoryDescriptorIterator::flushDesciptor()
{
    if(_dotEntry.name(2) != "." && _dotEntry.descriptor != _descriptorHandle) {
        throw bad_state_exception("'Dot entry' has been changed.");
    }
    if(_dotDotEntry.name(3) != ".." && _dotDotEntry.descriptor != _descriptorData.parent) {
        throw bad_state_exception("'DotDot entry' has been changed.");
    }
    _updateDescriptor(_descriptorData);
}

void DirectoryDescriptorIterator::toLast()
{
    // TODO: use next address

    while(hasNext()) {
        this->operator ++();
    }
}

FSDescriptor DirectoryDescriptorIterator::descriptor() const
{
    return _descriptorData;
}

descriptorIndex_tp DirectoryDescriptorIterator::descriptorHandle() const
{
    return _descriptorHandle;
}

bool DirectoryDescriptorIterator::weInDescriptor() const
{
    return _currentIndex < _entriesInDescriptor || _currentIndex == directoryEntryIndex_tp(-1);
}

directoryEntryIndex_tp DirectoryDescriptorIterator::currentBlockEntriesLimit() const
{
    return weInDescriptor() ? _entriesInDescriptor : _entriesInBlock;
}

blockAddress_tp DirectoryDescriptorIterator::currentBlockNextSegmentAddress() const
{
    return weInDescriptor() ? _descriptorData.nextDataSegment : _currentBlock->nextSegment;
}

directoryEntry *DirectoryDescriptorIterator::currentEntryArray()
{
    return weInDescriptor() ? _descriptorData.directoryEntries : _currentBlock->directoryEntries;
}

void DirectoryDescriptorIterator::setNextAddressInCurrentContainer(blockAddress_tp nextBlock)
{
    if(weInDescriptor()) {
        _descriptorData.nextDataSegment = nextBlock;
    } else {
        _currentBlock->nextSegment = nextBlock;
    }
}

void DescriptorAlgorithms::deleteEntry(DirectoryDescriptorIterator &it)
{
    DirectoryDescriptorIterator itInEnd(it, false);
    itInEnd.toLast();
    *it = *itInEnd;
//        it.currentEntryArray()[it._currentOffsetInBlock] = itInEnd.currentEntryArray()[itInEnd._currentOffsetInBlock];

    if(itInEnd._currentOffsetInBlock == 0 && !it.weInDescriptor()) {
        if(itInEnd._prevBlockAddress == Constants::HEADER_ADDRESS()) {
            assert(itInEnd._descriptorData.nextDataSegment == itInEnd._currentBlockAddress);
            _callbacks.deallocBlock(it._descriptorData.nextDataSegment);
            it._descriptorData.nextDataSegment = Constants::HEADER_ADDRESS();
        } else {
            auto prevBlock = _callbacks.readExtendedSegment(itInEnd._prevBlockAddress);

            assert(prevBlock->nextSegment == itInEnd._currentBlockAddress);
            _callbacks.deallocBlock(prevBlock->nextSegment);
            prevBlock->nextSegment = Constants::HEADER_ADDRESS();
        }
    }
    it._descriptorData.firstFreeElementIndex--;
    it.flush();
}

void DescriptorAlgorithms::appendToEnd(descriptorIndex_tp directoryDescriptorIndex, FSDescriptor directoryDescriptorData, directoryEntry entry)
{
    auto it = iterator(directoryDescriptorIndex, directoryDescriptorData);
    it.toLast();
    if(it._currentOffsetInBlock + 1 != it.currentBlockEntriesLimit()) {
        it.currentEntryArray()[it._currentOffsetInBlock + 1] = entry;
        it._updateDescriptor(it._descriptorData);
    } else {
        blockAddress_tp newBlockAddress = _callbacks.allocNewFreeBlock();
        auto newBlock = _callbacks.readExtendedSegment(newBlockAddress);
        newBlock->init();
        newBlock->directoryEntries[0] = entry;
        it.setNextAddressInCurrentContainer(newBlockAddress);
    }
    it._descriptorData.firstFreeElementIndex++;
    it.flush();
}

#include <iostream>
DirectoryDescriptorIterator DescriptorAlgorithms::iterator(descriptorIndex_tp descriptorIndex, const FSDescriptor &descriptorBlockData, bool syncDescriptor) const
{
    if(syncDescriptor) {
        auto syncFunction = [this, descriptorIndex](const FSDescriptor& desc) {
            _callbacks.updateDescriptor(descriptorIndex, desc);
        };
        return DirectoryDescriptorIterator(descriptorIndex,
                                           descriptorBlockData,
                                           _entriesInDescriptor,
                                           _entriesInBlock,
                                           _callbacks.readExtendedSegment,
                                           syncFunction);
    } else {
        return DirectoryDescriptorIterator(descriptorIndex,
                                           descriptorBlockData,
                                           _entriesInDescriptor,
                                           _entriesInBlock,
                                           _callbacks.readExtendedSegment);
    }
}

void DescriptorAlgorithms::setEntriesInfo(directoryEntryIndex_tp entriesInDescriptor, directoryEntryIndex_tp entrienInBlock)
{
    _entriesInDescriptor = entriesInDescriptor;
    _entriesInBlock = entrienInBlock;
}

void DescriptorAlgorithms::setCallbacks(descriptorAlgorithmCallbacks callbacks)
{
    _callbacks = callbacks;
}
