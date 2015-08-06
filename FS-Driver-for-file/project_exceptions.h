#ifndef PROJECT_EXCEPTIONS
#define PROJECT_EXCEPTIONS

#include <stdexcept>

// TODO: more exceptions!!!

namespace fs_excetion {

class bad_state_exception : public std::logic_error
{
public:
    explicit
    bad_state_exception(const std::string &message) :
        logic_error(message)
    { }
};

class file_open_exception : public std::logic_error
{
public:
    explicit
    file_open_exception(const std::string &message) :
        logic_error(message)
    { }
};

class file_system_exception : public std::logic_error
{
public:
    explicit
    file_system_exception(const std::string &message) :
        logic_error(message)
    { }
};

class file_not_formated_exception : public file_system_exception
{
public:
    explicit
    file_not_formated_exception (const std::string &message) :
        file_system_exception(message)
    { }
};

class not_enough_arguments_exception : public file_system_exception
{
public:
    explicit
    not_enough_arguments_exception (size_t avalArguments, size_t minArguments) :
        file_system_exception("Arguments: " + std::to_string(avalArguments) + "/" + std::to_string(minArguments) + ".")
    { }
};

class filename_error : public file_system_exception
{
public:
    explicit
    filename_error (const std::string &message) :
        file_system_exception(message)
    { }
};


class no_enough_fs_entry : public file_system_exception
{
public:
    explicit
    no_enough_fs_entry (const std::string &message) :
        file_system_exception(message)
    { }
};
}

#endif // PROJECT_EXCEPTIONS

