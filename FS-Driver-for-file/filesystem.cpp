#include "filesystem.h"

#include "console.h"
#include "project_exceptions.h"
using namespace fs_excetion;
#include "fsdescriptoriterator.h"
#include "path.h"

#include <iostream>
#include <cassert>
#include <limits>

FileSystem::FileSystem(std::shared_ptr<FormatedFileAccessor> fsFile) :
    _fsFile(fsFile),
    _bitMap(fsFile),
    _descriptors(fsFile),
    _dataBlocks(fsFile)
{
    descriptorAlgorithmCallbacks callbacks;
    callbacks.readExtendedSegment = [this](blockAddress_tp block) -> TypedBufferLocker<FSDescriptorDataPart> {
        return _dataBlocks.readData<FSDescriptorDataPart>(block, SyncType::ReadWrite);
    };
    callbacks.allocNewFreeBlock = [this]() -> blockAddress_tp {
        return findAndAllocateFreeDataBlock();
    };
    callbacks.deallocBlock = [this](blockAddress_tp block) {
        _bitMap.set(block, false);
    };
    callbacks.updateDescriptor = [this](descriptorIndex_tp descriptorIndex, const FSDescriptor& descriptor) {
        _descriptors.updateDescriptor(descriptorIndex, descriptor);
    };
    _descriptorAlgo.setCallbacks(callbacks);
}

FileSystem::~FileSystem()
{

}

void FileSystem::registerCommands(Console *console)
{
    console->addCommand("mount", new ClassCommandWrapper<FileSystem>(this, &FileSystem::mount));
    console->addCommand("umount", new ClassCommandWrapper<FileSystem>(this, &FileSystem::umount));

    console->addCommand("filestat", new ClassCommandWrapper<FileSystem>(this, &FileSystem::filestat));
    console->addCommand("ls", new ClassCommandWrapper<FileSystem>(this, &FileSystem::ls));

    console->addCommand("create", new ClassCommandWrapper<FileSystem>(this, &FileSystem::create));
    console->addCommand("open", new ClassCommandWrapper<FileSystem>(this, &FileSystem::open));
    console->addCommand("close", new ClassCommandWrapper<FileSystem>(this, &FileSystem::close));

    console->addCommand("read", new ClassCommandWrapper<FileSystem>(this, &FileSystem::read));
    console->addCommand("write", new ClassCommandWrapper<FileSystem>(this, &FileSystem::write));

    console->addCommand("link", new ClassCommandWrapper<FileSystem>(this, &FileSystem::link));
    console->addCommand("unlink", new ClassCommandWrapper<FileSystem>(this, &FileSystem::unlink));

    console->addCommand("truncate", new ClassCommandWrapper<FileSystem>(this, &FileSystem::truncate));

    console->addCommand("mkdir", new ClassCommandWrapper<FileSystem>(this, &FileSystem::mkdir));
    console->addCommand("rmdir", new ClassCommandWrapper<FileSystem>(this, &FileSystem::rmdir));

    console->addCommand("cd", new ClassCommandWrapper<FileSystem>(this, &FileSystem::cd));
    console->addCommand("pwd", new ClassCommandWrapper<FileSystem>(this, &FileSystem::pwd));

    console->addCommand("symlink", new ClassCommandWrapper<FileSystem>(this, &FileSystem::symlink));

    console->addCommand("createFile", new ClassCommandWrapper<FileSystem>(this, &FileSystem::createFile));

    console->addCommand("format", new ClassCommandWrapper<FileSystem>(this, &FileSystem::formatFile));
}

void FileSystem::mount(arguments str)
{
    checkArgumentsCount(str, 1);
    _fsFile->open(str.at(0));
    if(_fsFile->isFormatedFS()) {
        fileFormatChanged();
    } else {
    }
}

void FileSystem::umount()
{
    _fsFile->close();
}

void FileSystem::filestat(arguments arg, outputStream out)
{
    checkArgumentsCount(arg, 1);
    descriptorIndex_tp descriptorId = static_cast<descriptorIndex_tp>(std::stoll(arg.at(0)));

    FSDescriptor descriptor;
    try {
        descriptor = _descriptors.getDescriptor(descriptorId);
    } catch(const std::invalid_argument &) {
        out << "Bad descriptor\n";
        return;
    }

    if(descriptor.type == DescriptorVariant::None) {
        out << "Descriptor does not exist\n";
        return;
    }

    using std::to_string;

    out << "File statistic: \n\tid: " << descriptorId;
    if(descriptor.type == DescriptorVariant::File) {
        out << "\n\tSize: " << descriptor.fileSize;
    }
    out << "\n\tType: "     << to_string(descriptor.type) <<
           "\n\tReferences: " << descriptor.referencesCount <<
           "\n\tHas extended blocks: " << (descriptor.nextDataSegment == Constants::HEADER_ADDRESS() ? "false" : "true") << "\n";
}

void FileSystem::ls(outputStream out)
{
    auto curFolder = currentDirectory();
    auto maxFilename = header().filenameLength;
    FSDescriptor descriptorData = _descriptors.getDescriptor(curFolder);

    assert(descriptorData.type == DescriptorVariant::Directory);

    out << "name\tdescriptor\n";

    DirectoryDescriptorIterator it = getDirectoryDescriptorIterator(curFolder);

    while(it.hasNext()) {
        ++it;
        out << it->toString(maxFilename) << "\n";
    }
}

void FileSystem::create(arguments arg)
{
    checkArgumentsCount(arg, 1);
    FSDescriptor fileDescriptor;
    fileDescriptor.initFile();
    allocAndAppendDescriptorToCurrentFolder(fileDescriptor, arg.at(0));
}

void FileSystem::open(arguments arg, outputStream out)
{
    checkArgumentsCount(arg, 1);

    if(_lastFreeFileDescriptor == std::numeric_limits<openedFileDescriptor_tp>::max() ) {
        // TODO: reimplement descriptor id select, old unused descriptors can be reused,
        throw file_system_exception("You opened too many files.");
    }

    auto descriptor = getLastPathElementDescriptor(arg.at(0));

    auto openedDescriptor = _lastFreeFileDescriptor++;
    _opennedFiles[openedDescriptor] = openedFileStream{descriptor};
    out << "Opened file descriptor: " << openedDescriptor << "\n";
}

void FileSystem::close(arguments arg, outputStream out)
{
    checkArgumentsCount(arg, 1);
    uint64_t descriptorToClose = std::stoull(arg.at(0));

    auto findResult = _opennedFiles.find(descriptorToClose);
    if(findResult  == _opennedFiles.end()) {
        throw file_system_exception("Descriptor currently not open.");
    } else {
        out << "Descriptor: " << descriptorToClose << " closed.\n";
        _opennedFiles.erase(findResult);
    }
}

void FileSystem::read(arguments, outputStream)
{
}

void FileSystem::write(arguments)
{

}

void FileSystem::link(arguments arg)
{
    checkArgumentsCount(arg, 2);
    string srcName  = arg.at(0);
    string destName = arg.at(1);

    descriptorIndex_tp destDir = currentDirectory();

    descriptorIndex_tp srcDir = currentDirectory();
    auto it = getDirectoryDescriptorIterator(srcDir);
    while(it.hasNext()) {
        ++it;
        if(it->name(header().filenameLength) == srcName) {
            addDescriptorToDirectory(destDir, it->descriptor, destName);
        }
    }
}

void FileSystem::unlink(arguments arg, outputStream out)
{
    checkArgumentsCount(arg, 1);
    string name = arg.at(0);
    descriptorIndex_tp dir = currentDirectory();
    if(removeDescriptorFromDirectory(dir, DescriptorVariant::File | DescriptorVariant::SymLink, name)) {
        out << "Descriptor '" << name << "' deleted from current directory.\n";
    } else {
        out << "Descriptor" << name << "is not found\n";
    }
}

void FileSystem::truncate(arguments)
{

}

void FileSystem::mkdir(arguments arg)
{
    checkArgumentsCount(arg, 1);
    FSDescriptor fileDescriptor;
    fileDescriptor.initDirectory(currentDirectory());
    string name = arg.at(0);
    allocAndAppendDescriptorToCurrentFolder(fileDescriptor, name);
}

void FileSystem::rmdir(arguments arg, outputStream out)
{
    checkArgumentsCount(arg, 1);
    string name = arg.at(0);
    if(removeDescriptorFromDirectory(currentDirectory(), DescriptorVariant::Directory, name)) {
        out << "Directory deleted\n";
    } else {
        out << "Directory not found\n";
    }
}

void FileSystem::cd(arguments arg)
{
    checkArgumentsCount(arg, 1);
    _currentFolder = getLastPathElementDescriptor(arg.at(0));
//    auto v = p.getParsedPath();
}

void FileSystem::pwd(arguments, outputStream out)
{
    auto paths = getDirectoryPathFromDescriptor(currentDirectory());
    out << "/";
    for(string directory : paths) {
        out << directory << "/";
    }
    out << "\n";
}

void FileSystem::symlink(arguments)
{

}

void FileSystem::createFile(arguments str, outputStream out)
{
    checkArgumentsCount(str, 2);
    std::ofstream::pos_type  size = std::stoll(str.at(1));     // 1234asd321 -> 1234, its ok?
    if(_fsFile->create(str.at(0), size)) {
        out << "File created";
    } else {
        out << "file not created";
    }
    out << "\n";
}

void FileSystem::fileFormatChanged()
{
    assert(_fsFile->isFormatedFS());
    _descriptorAlgo.setEntriesInfo(header().entriesInDirectoryDescriptor, header().entriesInDirectoryBlock);

    std::clog << "Install root directory: " << header().rootDirectoryDescriptor << "\n";
    _currentFolder = header().rootDirectoryDescriptor;
    std::clog << header().toString();

}

void FileSystem::checkArgumentsCount(arguments arg, size_t minArgumentsCount) const
{
    if(arg.size() < minArgumentsCount) {
        throw not_enough_arguments_exception(arg.size(), minArgumentsCount);
    }
}

void FileSystem::checkFilename(const std::string &filename) const
{
    if(filename.length() > header().filenameLength) {
        throw filename_error("File name is too long.");
    }
}

descriptorIndex_tp FileSystem::currentDirectory() const
{
    return _currentFolder;
}

const FSHeader &FileSystem::header() const
{
    return _fsFile->headerExcept();
}

void FileSystem::allocAndAppendDescriptorToCurrentFolder(const FSDescriptor &descriptor, const std::string &name)
{
    // TODO: check conflicts

    blockAddress_tp notFullDescriptorBlock = _bitMap.findFirstFreeBlock(
                                                    _descriptors.areaBegin(),
                                                    _descriptors.areaEnd() );

    if(notFullDescriptorBlock == Constants::HEADER_ADDRESS()) {
        throw no_enough_fs_entry("No enough descriptors");
    }

    bool blockBeenFilled = false;
    auto newFileDescriptorIndex = _descriptors.appendDescriptor(notFullDescriptorBlock, descriptor, &blockBeenFilled);
    if(blockBeenFilled) {
        _bitMap.set(notFullDescriptorBlock, true);
    }

    addDescriptorToDirectory(currentDirectory(), newFileDescriptorIndex, name);
}

void FileSystem::addDescriptorToDirectory(descriptorIndex_tp directoryDescriptorIndex, descriptorIndex_tp folderElementDescriptor, const string &name)
{
//    // TODO: parse name path

    checkFilename(name);

    directoryEntry newEntry;
    newEntry.set(folderElementDescriptor, name, header().filenameLength);

    FSDescriptor dirDescriptor = _descriptors.getDescriptor(directoryDescriptorIndex);

    assert(dirDescriptor.type == DescriptorVariant::Directory);

    _descriptorAlgo.appendToEnd(directoryDescriptorIndex, dirDescriptor, newEntry);
}

bool FileSystem::removeDescriptorFromDirectory(descriptorIndex_tp directoryDescriptorIndex, DescriptorVariant type, const std::string &name)
{
    checkFilename(name);

    auto it = getDirectoryDescriptorIterator(directoryDescriptorIndex);

    while(it.hasNext()) {
        ++it;
        if(it->name(header().filenameLength) == name) {
            FSDescriptor dirDescriptor = _descriptors.getDescriptor(it->descriptor);
            if((dirDescriptor.type & type) != 0) {
                _descriptors.decrementReference(it->descriptor);
                _descriptorAlgo.deleteEntry(it);
                return true;
            } else {
                throw file_system_exception("Unable to remove '" + name + "'. " + std::to_string(dirDescriptor.type) + " is not " + std::to_string(type) + ".");
            }
        }
    }
    return false;
}

std::list<string> FileSystem::getDirectoryPathFromDescriptor(descriptorIndex_tp handle)
{
    std::list<string> resultPath;
    descriptorIndex_tp currentHandle = handle;
    while(currentHandle != header().rootDirectoryDescriptor) {
        bool found = false;
        DirectoryDescriptorIterator it = getDirectoryDescriptorIterator(currentHandle);
        while(it.hasNext()) {
            ++it;
            if(it->descriptor == currentHandle) {
                resultPath.push_front(it->name(header().filenameLength));
                found = true;
                break;
            }
        }
        if(!found) {
            throw std::runtime_error("Error in file format. Entry not found in parent directory.");
        }
        currentHandle = it.descriptor().nextDataSegment;
    }

    return resultPath;
}

blockAddress_tp FileSystem::findAndAllocateFreeDataBlock()
{
    blockAddress_tp freeBlock = _bitMap.findFirstFreeBlock(
                _dataBlocks.areaBegin(),
                _dataBlocks.areaEnd());
    if(freeBlock == Constants::HEADER_ADDRESS()) {
        throw no_enough_fs_entry("Not enough free space.");
    }
    _bitMap.set(freeBlock, true);
    return freeBlock;
}

void FileSystem::formatFile()
{
    FSHeader header;
    header.init();
    header.blockByteSize = Constants::blockByteSize();
    uint64_t blockCount = _fsFile->fileSize() / header.blockByteSize;
    if(blockCount < 4) {
        throw bad_state_exception("File is too small.");
    }

//    TODO: fix magic numbers

    header._bitMapBegin = 1;
    header._bitMapEnd = header._bitMapBegin + (blockCount / header.bitsInBitMapBlock());
    auto descriptorBlocks = (blockCount / 4 / header.descriptorsInBlock());
    header._descriptorsEnd = header._bitMapEnd + 1 + descriptorBlocks;
    header._dataEnd = blockCount - 1;

    _fsFile->formatFS(header);

    _bitMap.initBlocks();
    _descriptors.initBlocks();

    FSDescriptor rootDirectory;
    rootDirectory.initDirectory(header.rootDirectoryDescriptor);

    _descriptors.updateDescriptor(header.rootDirectoryDescriptor, rootDirectory);

    // WARNING: if one block contains one descriptor, need fix bit map
    assert(header.descriptorsInBlock() > 1);

    fileFormatChanged();
}

descriptorIndex_tp FileSystem::getLastPathElementDescriptor(const std::string path)
{
    Path p(path);
    return getLastPathElementDescriptor(p.getParsedPath(), p.isAbsolute());
}

descriptorIndex_tp FileSystem::getLastPathElementDescriptor(const std::vector<std::string> &path, bool isAbsolute)
{
    descriptorIndex_tp currentHandle = isAbsolute ? header().rootDirectoryDescriptor : currentDirectory();

    // TODO: symbolic link and check symbolic link loop

//    for(const string &currentName: path) {
    for(auto currentName = path.cbegin(); ; ++currentName) {
        if(currentName == path.cend()) {
            return currentHandle;
        }
        DirectoryDescriptorIterator dIt = getDirectoryDescriptorIterator(currentHandle);
        bool foundNext = false;
        while(dIt.hasNext()) {
            ++dIt;
            if(dIt->name(header().filenameLength) == *currentName) {
                foundNext = true;
                currentHandle = dIt->descriptor;
                break;
            }
        }
        if(!foundNext) {
            throw file_system_exception("No such file or directory.");
        }
    }
}

DirectoryDescriptorIterator FileSystem::getDirectoryDescriptorIterator(descriptorIndex_tp directoryDescriptorIndex)
{
    FSDescriptor dirDescriptor = _descriptors.getDescriptor(directoryDescriptorIndex);

    assert(dirDescriptor.type == DescriptorVariant::Directory);
    _descriptorAlgo.setEntriesInfo(header().entriesInDirectoryDescriptor,
                                   header().entriesInDirectoryBlock);
    return _descriptorAlgo.iterator(directoryDescriptorIndex, dirDescriptor);
}
