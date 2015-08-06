#ifndef CONSOLEOPERATIONHANDLER_H
#define CONSOLEOPERATIONHANDLER_H

class Console;
class ConsoleOperationHandler
{
public:
    virtual ~ConsoleOperationHandler() { }
    virtual void registerCommands(Console *) { }
};

#endif // CONSOLEOPERATIONHANDLER_H
