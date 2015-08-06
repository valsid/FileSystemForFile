#include <iostream>

using namespace std;

#include "filesystemblock.h"
#include "filesystem.h"
#include "console.h"

// https://ru.wikipedia.org/wiki/DEC_Alpha

static_assert(Constants::HEADER_ADDRESS() == 0, "Need project refactor, if you change this value.");
static_assert(sizeof(FSDescriptorsContainerBlock) % sizeof(FSDescriptor) == 0, "You use not optimal sizes, sorry.");
static_assert(sizeof(blockAddress_tp) == 8, "Project by testing only for its settings.");

static_assert(std::is_pod<FSBitMapBlock>::value, "Used not 'plain of data' (pod) structures");
static_assert(std::is_pod<FSDescriptor>::value, "Used not 'plain of data' (pod) structures");
static_assert(std::is_pod<FSDescriptorsContainerBlock>::value, "Used not 'plain of data' (pod) structures");
static_assert(std::is_pod<FSDescriptorDataPart>::value, "Used not 'plain of data' (pod) structures");
static_assert(std::is_pod<FSDataBlock>::value, "Used not 'plain of data' (pod) structures");
static_assert(std::is_pod<FSHeader>::value, "Used not 'plain of data' (pod) structures");

// Source code is available on the repository github.com/valsid

int main()
{
    std::cout << "Header: " << sizeof(FSHeader) << "\n"
              << "Mark: "   << sizeof(Signature) << "\n"
              << "BitMap: " << sizeof(FSBitMapBlock) << "\n"
              << "Descriptor: " << sizeof(FSDescriptor) << "\n"
              << ("lol, its work"[2147483647]);

    shared_ptr<FormatedFileAccessor> a(new FormatedFileAccessor());
    FileSystem fs(a);

    Console console;
    fs.registerCommands(&console);

    auto testCase = [&]() {
        consoleCommand com;
        com.command = "mount";
        com.arguments.push_back("fs");
        console.runCommand(com);

        com.command = "format";
        console.runCommand(com);

        com.command = "create";
        for(int i = 0; i < 3; i++) {
            com.arguments.clear();
            com.arguments.push_back("f" + std::to_string(i));
            console.runCommand(com);
        }
    };
    console.addCommand("q_testCase", new FunctionConsoleOperation(testCase));

    console.run();

    return 0;
}

