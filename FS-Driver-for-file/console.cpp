#include "console.h"

#include "project_exceptions.h"
using namespace fs_excetion;

#include <iostream>
#include <exception>

Console::Console()
{
    registerCommands(this);
}

Console::~Console()
{
    for(auto &e : _commands) {
        delete e.second;
    }
}

void Console::addCommand(std::string command, ConsoleOperation *callback)
{
    _commands[command] = callback;
}

void Console::runCommand(consoleCommand command)
{
    auto it = _commands.find(command.command);
    if (it == _commands.end())
    {
        std::cout << "Command '" << command.command << "' not found.\n";
        return;
    }

    std::vector<string> arg{ std::make_move_iterator(std::begin(command.arguments)),
                             std::make_move_iterator(std::end(command.arguments)) };
    try {
        (*it->second)(arg, std::cout);
    } catch(const file_system_exception &e) {
        std::cerr << "FS error: " << e.what() << "\n";
    } catch (const std::exception &e) {
        std::cerr << "Sys error: " << e.what() << "\n";
    } catch (...) {
        std::cerr << "Unknow error! What are you doing!??";
    }
}

std::vector<std::string> Console::getCommands() const
{
    std::vector<std::string> result;
    for(auto b : _commands) {
        result.push_back(b.first);
    }

    return result;
}

void Console::run()
{
    _exit = false;
    consoleCommand command;
    while(!_exit) {
        std::cout << "Command: ";
        command = getCommand(getCommands());
        runCommand(command);
    }
}

void Console::registerCommands(Console *console)
{
    addCommand("help", new ClassCommandWrapper<Console>(console, &Console::help));
    addCommand("exit", new ClassCommandWrapper<Console>(console, &Console::closeConsole));
}

void Console::closeConsole()
{
    _exit = true;
}

void Console::help(outputStream out)
{
    const auto &commands = getCommands();
    auto it = commands.cbegin();
    if(it == commands.cend()) {
        return;
    }

    while(true) {
        out << *it;
        if(++it == commands.cend()) {
            out << "\n";
            return;
        }
        out << ", ";
    }
}
