#include "filesystemblock.h"
#include "project_exceptions.h"

#include <cassert>
#include <type_traits>

DescriptorVariant operator |(const DescriptorVariant &first, const DescriptorVariant &second)
{
    return static_cast<DescriptorVariant>(static_cast<descriptorEnum_tp>(first) | static_cast<descriptorEnum_tp>(second));
}

descriptorEnum_tp operator &(const DescriptorVariant &first, const DescriptorVariant &second)
{
    return static_cast<descriptorEnum_tp>(first) & static_cast<descriptorEnum_tp>(second);
}

uint64_t FSBitMapBlock::findFirstZero(uint64_t begin, uint64_t maxBits) const
{
//    auto bitsInOneElement = 8;
    for(std::uint32_t i = begin; i < maxBits; i++ /*+= bitsInOneElement*/) {
//        if(bits[i] != 0) {
//            for(auto j = bitsInOneElement - 1; j >= 0; j--) {
//                if(bits[i - j] == 1) {
//                    return i - j;
//                }
//            }
//        }
        if(!get(i)) {
            return i;
        }
    }
    return maxBits;
}

void FSBitMapBlock::set(uint64_t bit, bool data)
{
    if(data) {
        bits[bit / 8] |= 1 << (bit & 7);
    } else {
        bits[bit / 8] &= ~(1 << (bit & 7));
    }
}

bool FSBitMapBlock::get(uint64_t bit) const
{
    return (bits[bit / 8] & (1 << (bit & 7))) != 0;
}

void FSBitMapBlock::reset()
{
    for(auto &b : bits) {
        b = 0;
    }
}

std::string FSBitMapBlock::toString(const FSHeader &data) const
{
    std::string result;
    for(uint64_t i = 0; i < data.bitsInBitMapBlock(); i++) {
        result += std::to_string(get(i));
    }
    return result;
}

std::string std::to_string(DescriptorVariant var)
{
    switch(var){
    case DescriptorVariant::None:
        return "none";
        break;
    case DescriptorVariant::File:
        return "file";
        break;
    case DescriptorVariant::SymLink:
        return "symlink";
        break;
    case DescriptorVariant::Directory:
        return "directory";
        break;
    case DescriptorVariant::Any:
        return "Any";
        break;
    }

    if((var & DescriptorVariant::Any) == 0) {
        return "bad variant";
    } else {
        // "mixed variant";
        auto checkValues = {DescriptorVariant::File, DescriptorVariant::SymLink, DescriptorVariant::Directory};
        string result;
        for(DescriptorVariant checkVariant : checkValues) {
            if((checkVariant & var) != 0) {
                if(!result.empty()) {
                    result += " or ";
                }
                result += to_string(checkVariant);
            }
        }
        return result;
    }
}

void directoryEntry::set(descriptorIndex_tp descriptor, std::string name, filenameLength_tp maxLength)
{
    assert(name.size() <= maxLength);    // for debug

    if(name.empty()) {
        throw std::invalid_argument("Empty name");
    }

    if(name.size() >= maxLength) {
        name = name.substr(0, maxLength);
    }

    uint64_t i = 0;
    for(; i < name.size(); i++) {
        this->__name[i] = name[i];
    }
    if(i != maxLength) {
        this->__name[i] = '\0';
    }

    this->descriptor = descriptor;
}

std::string directoryEntry::name(filenameLength_tp maxFilename) const
{
    std::string result;
    char iSymbol = __name[0];

    if(iSymbol == '\0') {
        return "~empty~";
    }

    for(uint64_t i = 0; __name[i] != '\0' && i < maxFilename; i++) {
        result += __name[i];
    }

    return result;
}

std::string directoryEntry::toString(uint64_t maxFilename) const
{
    std::string s;
    s.reserve(sizeof(descriptor) + maxFilename);

    s += name(maxFilename);
    s += "\t";
    s += std::to_string(descriptor);
//    s += "\n";

    return s;
}

bool directoryEntry::isEmpty() const
{
    return __name[0] == '\0';
}

void directoryEntry::clear()
{
    __name[0] = '\0';
}


bool FSDescriptor::isHaveExtendedSegment()
{
    return nextDataSegment != Constants::HEADER_ADDRESS();
}

std::string FSDescriptor::toString(const FSHeader &data) const
{
    if(type == DescriptorVariant::None) {
        return "Empty descriptor\n";
    }
    using std::to_string;

    std::string res;
    res += "Desciptor block:";
    res += "\nType: "       + to_string(type);
    res += "\nReferences: " + to_string(referencesCount);
    res += "\nNext: " + to_string(nextDataSegment);
    res += "\nFirst free elem: " + to_string(firstFreeElementIndex);

    switch(type) {
    case DescriptorVariant::None:
        break;
    case DescriptorVariant::Any:
        res += "Any descriptor variant... Error!?";
        break;
    case DescriptorVariant::File:
        res += "\nFile size: " + to_string(fileSize);
        res += "\nData blocks:";
        for(uint64_t i = 0; i < firstFreeElementIndex && i < data.blockLinkCountInFileDescriptor; i++) {
            res += "\n  " + std::to_string(dataSegments[i]);
        }
        break;
    case DescriptorVariant::Directory:
        res += "\nParent: " + to_string(parent);
        for(uint64_t i = 0; i < firstFreeElementIndex && i < data.entriesInDirectoryDescriptor; i++) {
            res += "\n  " + directoryEntries[i].toString(data.filenameLength);
        }
        break;
    case DescriptorVariant::SymLink:
        res += "\n Symlink: ";
        break;
    }

    return res + "\n";
}

std::string FSDescriptorDataPart::toString(const FSHeader &data) const
{
    std::string res;
    res += "'FSDescriptorDataPart' block";
    res += "\nNext segment: " + std::to_string(nextSegment);

    res += "/*TODO: other data;*/";
    res += "Folder data: \n";

    for(uint64_t i = 0; i < data.entriesInDirectoryBlock; i++) {
        res += directoryEntries[i].toString(data.filenameLength);
        res += "\n";
    }

    return res + "\n";
}

bool FSDescriptorDataPart::isHaveExtendedSegment()
{
    return nextSegment != Constants::HEADER_ADDRESS();
}


std::string FileSystemBlock::toString(const FSHeader &) const
{
    return "Unknow file System object\n";
}


#include <QDebug>
std::string FSDescriptorsContainerBlock::toString(const FSHeader &data) const
{
    std::string res;
    res += "FSDescriptorsContainerBlock:\n";
    for(uint64_t i = 0; i < data.descriptorsInBlock(); i++) {
        res += "\n";
        res += descriptors[i].toString(data);
    }
    return res + "\n";
}


std::string FSDataBlock::toString(const FSHeader &) const
{
    return "'FSDataBlock'\n";
}
