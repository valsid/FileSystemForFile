#include <iostream>

using namespace std;

#include "filesystemblock.h"
#include "filesystem.h"
#include "console.h"

// warning asserts
static_assert(Constants::HEADER_ADDRESS() == 0, "Need project refactor, if you change this value.");
static_assert(sizeof(FSDescriptorsContainerBlock) % sizeof(FSDescriptor) == 0, "You are used not optimal constants.");
static_assert(sizeof(blockAddress_tp) == 8, "Project was tested only with 8byte block address.");
static_assert(sizeof(FSDescriptor) == FSDescriptor::fullDescriptorSize, "Wrong FSDescriptor constants.");

static_assert(std::is_pod<FSBitMapBlock>::value, "Used not 'plain of data' (pod) structures");
static_assert(std::is_pod<FSDescriptor>::value, "Used not 'plain of data' (pod) structures");
static_assert(std::is_pod<FSDescriptorsContainerBlock>::value, "Used not 'plain of data' (pod) structures");
static_assert(std::is_pod<FSDescriptorDataPart>::value, "Used not 'plain of data' (pod) structures");
static_assert(std::is_pod<FSDataBlock>::value, "Used not 'plain of data' (pod) structures");
static_assert(std::is_pod<FSHeader>::value, "Used not 'plain of data' (pod) structures");

int main()
{
    std::cout << "Header: " << sizeof(FSHeader) << "\n"
              << "Mark: "   << sizeof(Signature) << "\n"
              << "BitMap: " << sizeof(FSBitMapBlock) << "\n"
              << "Descriptor: " << sizeof(FSDescriptor) << "\n";

    shared_ptr<FormatedFileAccessor> a(new FormatedFileAccessor());
    FileSystem fs(a);

    Console console;
    fs.registerCommands(&console);

    auto testCase = [&]() {
        consoleCommand com;
        com.command = "mount";
        com.arguments.push_back("fs");

        std::cout << "Mount file 'fs' for filesystem: \n";
        console.runCommand(com);

        com.command = "format";
        std::cout << "\nFormat fs: \n";
        console.runCommand(com);

        com.command = "ls";
        std::cout << "\nls of a empty fs: \n";
        console.runCommand(com);

        std::cout << "\nCreate empty files and ls: \n";

        com.command = "create";
        com.arguments.clear();
        for(int i = 0; i < 3; i++) {
            com.arguments.push_back("f" + std::to_string(i));
            console.runCommand(com);
            com.arguments.clear();
        }

        com.command = "ls";
        console.runCommand(com);
    };
    console.addCommand("q_testCase", new FunctionConsoleOperation(testCase));

    console.run();

    return 0;
}

