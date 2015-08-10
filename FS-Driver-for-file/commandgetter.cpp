#include "commandgetter.h"

#include <iostream>
using std::cout;

using std::string;


typedef std::list<string> stringList;

void deleteLastSymbol(string *str)
{
    str->erase(str->length() - 1);
}

stringList getU(string commandPrefix, std::vector<string> commands)
{
    stringList result;
    for(string c : commands) {
        string sub = c.substr(0, commandPrefix.size());
        if(commandPrefix == sub) {
            result.push_back(c);
        }
    }
    return result;
}

void printCommands(stringList commands)
{
    short i = 0;
    for(string s : commands) {
        i++;
        cout << s;
        if(i == 4) {
            cout << "\n";
            i = 0;
        } else {
            cout << (s.size() > 7 ? "\t" : "\t\t");
        }
    }
}

string findMaxPrefixAfterPos(stringList list, size_t pos) {
    if(list.size() == 0) {
        return "";
    }

    size_t prefixLength = pos;
    for( ; ; prefixLength++) {
        if(prefixLength >= list.back().size()) {
            break;
        }
        char prefixSymbol = list.back().at(prefixLength);
        for(string s : list) {
            if(prefixLength >= s.size() || s.at(prefixLength) != prefixSymbol) {
                return list.back().substr(pos, prefixLength - pos);
            }
        }
    }
    return list.back().substr(pos, prefixLength - pos);
}

#ifdef WIN32
#include <conio.h>
#else
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

int getch_forUnixLike( ) {
  struct termios oldt,
                 newt;
  int            ch;
  tcgetattr( STDIN_FILENO, &oldt );
  newt = oldt;
  newt.c_lflag &= ~( ICANON | ECHO );
  tcsetattr( STDIN_FILENO, TCSANOW, &newt );
  ch = getchar();
  tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
  return ch;
}
#endif

consoleCommand getCommand(std::vector<string> helpCommands)
{
    char userInput = ' ';

    std::list<string> arguments;

    string currentText;

    int skipNChar = 0;

    while(userInput != '\n' || (arguments.empty() && currentText.empty())) {
#ifdef WIN32
        userInput = getch();
#else
        input = getch_forUnixLike();
#endif
        if(skipNChar > 0) {
            skipNChar--;
            continue;
        }
        switch(userInput) {
        case '\b':  // backspace, Windows
        case 127:   // ASCII,     Unix-like
            if(currentText.empty() && !arguments.empty()) {
                currentText = arguments.back();
                arguments.pop_back();
                cout << "\b \b";
                continue;
            }

            if(!currentText.empty())
            {
                deleteLastSymbol(&currentText);
                cout << "\b \b";
            }
            continue; // replaced break
        case ' ':
            arguments.push_back(currentText);
            currentText = "";
            break;
        case '\r':
        case '\n':
            userInput = '\n';
            break;
        case '\t':
            if(arguments.empty()) {
               auto okCommands = getU(currentText, helpCommands);
               string maxPrefix = findMaxPrefixAfterPos(okCommands, currentText.size());
               if(!maxPrefix.empty()) {
                   std::cout << maxPrefix;
                   currentText += maxPrefix;
               }
               switch (okCommands.size()) {
               case 0: case 1:
                   continue;
               default:
                   cout << "\n";
                   printCommands(okCommands);
                   cout << "\n";
                   cout << currentText;
               }
            }
            continue;
        case 27:    // Escape, next character is not echoed
            skipNChar = 2;  // TODO: make test. Its work for me
            continue;
        default:
            currentText += userInput;
            break;
        }

        cout << userInput;
    }

    arguments.push_back(currentText);

    string commandd = arguments.front();
    arguments.pop_front();
    return {commandd, arguments};
}
