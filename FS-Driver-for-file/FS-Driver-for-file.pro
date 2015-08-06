TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
#CONFIG -= qt
CONFIG += c++11

SOURCES += main.cpp \
    fileaccessor.cpp \
    filesystem.cpp \
    filesystemblock.cpp \
    console.cpp \
    commandgetter.cpp \
    path.cpp \
    constants.cpp \
    consoleoperationhandler.cpp \
    filesystemarea.cpp \
    blockbuffer.cpp \
    fsdescriptoriterator.cpp

include(deployment.pri)
qtcAddDeployment()

HEADERS += \
    fileaccessor.h \
    filesystem.h \
    filesystemblock.h \
    console.h \
    consoleoperationwrapper.h \
    commandgetter.h \
    path.h \
    constants.h \
    consoleoperationhandler.h \
    project_exceptions.h \
    filesystemarea.h \
    blockbuffer.h \
    fsdescriptoriterator.h


win32:DEFINES += WIN32
