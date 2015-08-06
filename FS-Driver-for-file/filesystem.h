#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "consoleoperationhandler.h"
#include "consoleoperationwrapper.h"
#include "fsdescriptoriterator.h"
#include "filesystemarea.h"
#include "fileaccessor.h"

#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <tuple>

using std::string;

// TODO: make block commodification check

class FileSystem : public ConsoleOperationHandler
{
    using openedFileDescriptor_tp = uint64_t;
    using openedFileOffset_tp     = uint64_t;  // WARNING: change to 128 bit integer, if you have filesystem which contains more 2^64 bytes

public:
    FileSystem(std::shared_ptr<FormatedFileAccessor> fsFile);
    ~FileSystem();

    void registerCommands(Console *console) override;

    void mount(arguments str);
    void umount();

    void filestat(arguments arg, outputStream out);
    void ls(outputStream out);

    void create(arguments arg);
    void open(arguments arg, outputStream out);
    void close(arguments arg);

    void read(arguments, outputStream);
    void write(arguments);

    void link(arguments arg);
    void unlink(arguments arg, outputStream out);

    void truncate(arguments str);

    void mkdir(arguments arg);
    void rmdir(arguments arg, outputStream out);

    void cd(arguments arg);

    void pwd(arguments str, outputStream out);

    void symlink(arguments str);

    // paramethers: path - size
    void createFile(arguments str, outputStream out);

    void formatFile();

private:
    void fileFormatChanged();

    std::shared_ptr<FormatedFileAccessor> _fsFile;

    BitMapArea _bitMap;
    DescriptorsArea _descriptors;
    DataArea _dataBlocks;

    descriptorIndex_tp _currentFolder;

    // TODO: arguments pattern
    void checkArgumentsCount(arguments arg, size_t minArgumentsCount) const;
    void checkFilename(const string &filename) const;

    descriptorIndex_tp currentDirectory() const;

    const FSHeader &header() const;

    void allocAndAppendDescriptorToCurrentFolder(const FSDescriptor &descriptor,const std::string &name);

    void addDescriptorToDirectory(descriptorIndex_tp folderDescriptor, descriptorIndex_tp folderElementDescriptor, const std::string &name);

    bool removeDescriptorFromDirectory(descriptorIndex_tp folderDescriptor, DescriptorVariant type, const std::string &name);

    /**
     * @brief get directory path from descriptor handle
     * @param handle
     * @return path from root directory (not inclusive root element)
     */
    std::list<string> getDirectoryPathFromDescriptor(descriptorIndex_tp handle);

    blockAddress_tp findAndAllocateFreeDataBlock();
    
    DirectoryDescriptorIterator getLastPathElementHandle(const std::string path);
    DirectoryDescriptorIterator getLastPathElementHandle(const std::vector<std::string> &path, bool isAbsolute);

    DirectoryDescriptorIterator getDirectoryDescriptorIterator(descriptorIndex_tp directoryDescriptorIndex);

    struct openedFileStream {
        FSDescriptor s;
//        openedFileOffset_tp offset;
        /* flags */
    };

    std::unordered_map<openedFileDescriptor_tp, openedFileStream> _opennedFiles;
    openedFileDescriptor_tp _lastOpennedFileDescriptor = 0;

    DescriptorAlgorithms _descriptorAlgo;
};

#endif // FILESYSTEM_H
