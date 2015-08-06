#ifndef FSDESCRIPTORITERATOR_H
#define FSDESCRIPTORITERATOR_H

#include "filesystemblock.h"
#include "blockbuffer.h"
#include "constants.h"

#include <iterator>
#include <functional>

class DescriptorAlgorithms;

// TODO: Create iterator for all descriptors types
/**
 * You need flush all changes in iterator element data
 */
class DirectoryDescriptorIterator : public std::iterator<std::forward_iterator_tag, directoryEntry>
{
    friend class DescriptorAlgorithms;

    using readSegmentFunction_tp = std::function<TypedBufferLocker<FSDescriptorDataPart>(blockAddress_tp)>;
    using updateDescriptorFunction_tp = const std::function<void(const FSDescriptor&)>;

    using reference = directoryEntry&;
    using pointer   = directoryEntry*;

public:
    DirectoryDescriptorIterator(descriptorIndex_tp descriptorIndex,
                                FSDescriptor descriptorBlockData,
                                const directoryEntryIndex_tp entriesInDescriptor,
                                const directoryEntryIndex_tp entriesInBlock,
                                readSegmentFunction_tp readFunction);

    DirectoryDescriptorIterator(descriptorIndex_tp descriptorIndex,
                                FSDescriptor descriptorBlockData,
                                const directoryEntryIndex_tp entriesInDescriptor,
                                const directoryEntryIndex_tp entriesInBlock,
                                readSegmentFunction_tp readFunction,
                                updateDescriptorFunction_tp updateFumtion);

    DirectoryDescriptorIterator(const DirectoryDescriptorIterator &other);

    DirectoryDescriptorIterator(const DirectoryDescriptorIterator &other, bool syncDescriptor);

    ~DirectoryDescriptorIterator();

    bool hasNext();

    bool operator == (const DirectoryDescriptorIterator &other);
    bool operator != (const DirectoryDescriptorIterator &other);

    /**
     * Be careful, synchronization does not happen immediately
     * Use flush, if you need
     * Change "." and ".." element data will lead to fail
     * @brief operator *
     * @return
     */
    reference operator *();

    /**
     * Be careful, synchronization does not happen immediately
     * Use flush, if you need
     * @brief operator ->
     * @return
     */
    pointer operator ->();

    DirectoryDescriptorIterator &operator++();
    DirectoryDescriptorIterator operator++(int);

    void flush();

    void flushDesciptor();

    void toLast();

    FSDescriptor descriptor() const;
    descriptorIndex_tp descriptorHandle() const;

private:
    static constexpr directoryEntryIndex_tp _badPosition = -3;
    static constexpr directoryEntryIndex_tp _dotPosition = -2;
    static constexpr directoryEntryIndex_tp _dotDotPosition = -1;
    directoryEntry _dotDotEntry;
    directoryEntry _dotEntry;

    bool weInDescriptor() const;

    directoryEntryIndex_tp currentBlockEntriesLimit() const;
    blockAddress_tp currentBlockNextSegmentAddress() const;
    directoryEntry *currentEntryArray();
    void setNextAddressInCurrentContainer(blockAddress_tp nextBlock);

    directoryEntryIndex_tp _currentIndex = _badPosition;
    directoryEntryIndex_tp _currentOffsetInBlock = _badPosition;
    mutable TypedBufferLocker<FSDescriptorDataPart> _currentBlock;
    blockAddress_tp _currentBlockAddress = Constants::HEADER_ADDRESS();
    blockAddress_tp _prevBlockAddress = Constants::HEADER_ADDRESS();

    bool _syncDescriptor;

    descriptorIndex_tp _descriptorHandle;
    FSDescriptor _descriptorData;
    const directoryEntryIndex_tp _entriesInDescriptor;
    const directoryEntryIndex_tp _entriesInBlock;
    const readSegmentFunction_tp _readSegment;
    const updateDescriptorFunction_tp _updateDescriptor;
};

struct descriptorAlgorithmCallbacks {
    std::function<TypedBufferLocker<FSDescriptorDataPart>(blockAddress_tp)> readExtendedSegment;
    std::function<blockAddress_tp()> allocNewFreeBlock;
    std::function<void(blockAddress_tp)> deallocBlock;
    std::function<void(descriptorIndex_tp descriptorIndex, const FSDescriptor&)> updateDescriptor;
};

class DescriptorAlgorithms {
public:
    void deleteEntry(DirectoryDescriptorIterator &it);
    void appendToEnd(descriptorIndex_tp descriptorIndex, FSDescriptor descriptor, directoryEntry entry);

    DirectoryDescriptorIterator iterator(descriptorIndex_tp descriptorIndex, const FSDescriptor &descriptorBlockData, bool syncDescriptor = true) const;

    void setEntriesInfo(directoryEntryIndex_tp entriesInDescriptor, directoryEntryIndex_tp entrienInBlock);
    void setCallbacks(descriptorAlgorithmCallbacks callbacks);

    directoryEntryIndex_tp _entriesInDescriptor;
    directoryEntryIndex_tp _entriesInBlock;
    descriptorAlgorithmCallbacks _callbacks;
};

#endif // FSDESCRIPTORITERATOR_H
