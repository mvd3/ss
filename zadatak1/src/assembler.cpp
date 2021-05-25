#include <iostream>
#include <string>

#include "../inc/support.hpp"

int main(int argc, char** argv) {
    std::string* files = new std::string[2];
    if (readArguments(argc, argv, files)<0)
      return -1;
    if (assemble(files[0], files[1])<0)
      return -1;
    return 0;
}
