#include "path.h"

#include <list>

Path::Path(std::string path) :
    _path(path)
{

}

Path::~Path()
{

}

std::string Path::toString()
{
    return _path;
}

bool Path::isAbsolute() const
{
    return _path.front() == delimeter;
}

std::vector<std::string> Path::getParsedPath() const
{
    if(_parsedPath.empty()) {
        std::list<size_t> delimeterPositions;
        delimeterPositions.push_back(-1);
        for(size_t i = 0; i < _path.size(); i++) {
            if(_path.at(i) == delimeter) {
                delimeterPositions.push_back(i);
            }
        }
        if(!_path.empty() && _path.back() != delimeter) {
            delimeterPositions.push_back(_path.size());
        }
        if(_path.size() - 1 < _parsedPath.capacity()) {
            _parsedPath.reserve(_path.size() - 1);
        }

        auto it = delimeterPositions.begin();

        while(true) {
            size_t beginPos = *it;
            if(++it == delimeterPositions.end()) {
                break;
            }
            size_t endPos = *it;
            beginPos++;
            if(beginPos != endPos) {
                _parsedPath.push_back(_path.substr(beginPos, endPos - beginPos));
            }
        }

    }
    return _parsedPath;
}

