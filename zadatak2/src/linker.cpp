#include <iostream>
#include <string>

#include "../inc/support.hpp"

int main(int argc, char** argv) {

  // index 0: linkable or hex; index 1: input files; index 2: output file; index 3: -place arguments
  std::string* arguments = new std::string[4];

  if (readArguments(argc, argv, arguments)<0)
    return -1;

  if (arguments[0]=="hex") {
    if (linkExecutable(arguments)<0)
      return -1;
  } else {
    if (linkLinkable(arguments)<0)
      return -1;
  }

  return 0;
}
