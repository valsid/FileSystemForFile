#ifndef CONSOLEOPERATIONWRAPPER
#define CONSOLEOPERATIONWRAPPER

#include <string>
#include <memory>
#include <vector>
#include <functional>

using std::vector;
using std::string;

using arguments = std::vector<string>;
using outputStream = std::ostream &;

class ConsoleOperation {
public:
    virtual ~ConsoleOperation() {}
    virtual void operator()(arguments, outputStream) = 0;
};

class FunctionConsoleOperation : public ConsoleOperation{
    using callback_function_tp = std::function<void(arguments, outputStream)>;
    using callback_function_no_arg_tp = std::function<void()>;
public:
    FunctionConsoleOperation(callback_function_tp callback) {
        function = callback;
    }

    FunctionConsoleOperation(callback_function_no_arg_tp callback) {
        function = [=](arguments, outputStream) {
            callback();
        };
    }

    virtual ~FunctionConsoleOperation() override {}

    virtual void operator()(arguments paramPamPam, outputStream out) override {
        function(paramPamPam, out);
    }


private:
    callback_function_tp function;
};

template<typename ClassT>
class ClassCommandWrapper: public ConsoleOperation {
public:
     // return type _ argument type
    using function_out_arg_tp = void (ClassT::*)(arguments arg, outputStream out);
    using function_emp_arg_tp = void (ClassT::*)(arguments arg);
    using function_out_emp_tp = void (ClassT::*)(outputStream out);
    using function_emp_emp_tp = void (ClassT::*)(void);

    ClassCommandWrapper(ClassT *classLink, function_out_arg_tp func) :
        _link(classLink)
    {
        function = [=](arguments arg, outputStream out) {
            (classLink->*func)(arg, out);
        };
    }

    ClassCommandWrapper(ClassT *classLink, function_emp_arg_tp func) :
        _link(classLink)
    {
        function = [=](arguments arg, outputStream) {
            (classLink->*func)(arg);
        };
    }

    ClassCommandWrapper(ClassT *classLink, function_out_emp_tp func) :
        _link(classLink)
    {
        function = [=](arguments, outputStream out) {
            (classLink->*func)(out);
        };
    }

    ClassCommandWrapper(ClassT *classLink, function_emp_emp_tp func) :
        _link(classLink)
    {
        function = [=](arguments, outputStream) {
            (classLink->*func)();
        };
    }

    virtual void operator()(arguments paramPamPam, outputStream out) override {
        function(paramPamPam, out);
    }

private:
    std::function<void(arguments, outputStream)> function;
    ClassT *_link;
};

#endif // CONSOLEOPERATIONWRAPPER

