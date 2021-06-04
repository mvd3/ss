#include <iostream>
#include <string>

#include "../inc/support.hpp"

int main(int argc, char** argv) {

  if (argc!=2) {
    std::cout << "Error!\nInvalid command line argument(s)." << std::endl;
    return -1;
  }

  std::string hexFile(argv[1]);

  if (hexFile.substr(hexFile.size()-4)!=".hex") {
    std::cout << "Error!\nInvalid memory file." << std::endl;
    return -1;
  }

  if (emulate(hexFile)<0)
    return -1;

  return 0;
}
