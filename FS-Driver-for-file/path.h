#ifndef PATH_H
#define PATH_H

#include <string>
#include <vector>

class Path
{
public:
    Path(std::string _path);
    ~Path();

    std::string toString();

    bool isFolder() const;
    bool isFile() const;
    bool isAbsolute() const;

    std::vector<std::string> getParsedPath() const;

private:
    const char delimeter = '/';

    mutable std::vector<std::string> _parsedPath;
    std::string _path;
};

#endif // PATH_H
