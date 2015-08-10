#ifndef CONSOLE_H
#define CONSOLE_H

#include "consoleoperationwrapper.h"
#include "consoleoperationhandler.h"
#include "commandgetter.h"

#include <map>
#include <string>
#include <vector>

using std::string;

class Console : public ConsoleOperationHandler
{
public:
    Console();
    ~Console() override;

    void addCommand(string command, ConsoleOperation *callback);

    void runCommand(consoleCommand command);

    std::vector<string> getCommands() const;

    void run();

private:
    void registerCommands(Console *console) override;

    void closeConsole();

    void help(outputStream out);

    bool _exit = false;
    std::map<string, ConsoleOperation *> _commands;
};

#endif // CONSOLE_H
