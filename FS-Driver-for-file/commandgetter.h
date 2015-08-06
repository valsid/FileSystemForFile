#ifndef COMMANDGETTER_H
#define COMMANDGETTER_H

#include <string>
#include <vector>
#include <list>

struct consoleCommand {
    std::string command;
    std::list<std::string> arguments;
};

consoleCommand getCommand(std::vector<std::string> helpCommands);

#endif // COMMANDGETTER_H
