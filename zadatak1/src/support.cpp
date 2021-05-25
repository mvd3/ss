/*
 *   _____________    ______________        __________        ___________    __            __    __      __    ____________             ___________    __________
 *  |  _________  |  |  __________  |      |   ____   |      |  _______  |  |  |          |  |  |  |    |  |  |   ______   |           |  ______   |  |          |
 *  |  |       |  |  |  |        |  |      |  |    |  |      |  |     |  |  |  |    __    |  |  |  |   |  |   |  |      |  |           |  |     |  |  |  ________|
 *  |  |       |  |  |  |        |  |      |  |    |  |      |  |     |  |  |  |   |  |   |  |  |  |  |  |    |  |      |  |           |  |     |  |  |  |
 *  |  |       |  |  |  |        |  |      |  |    |  |      |  |_____|  |  |  |   |  |   |  |  |  | |  |     |  |      |  |           |  |     |  |  |  |____
 *  |  |       |  |  |  |        |  |      |  |____|  |      |  _________|  |  |   |  |   |  |  |  ||  |      |  |______|  |           |  |     |  |  |       |
 *  |  |       |  |  |  |        |  |   ___|__________|___   |  |           |  |   |  |   |  |  |  ||  |      |   ______   |           |  |     |  |  |  _____|
 *  |  |       |  |  |  |        |  |  |  _____________   |  |  |           |  |   |  |   |  |  |  | |  |     |  |      |  |           |  |     |  |  |  |
 *  |  |       |  |  |  |        |  |  |  |            |  |  |  |           |  |   |  |   |  |  |  |  |  |    |  |      |  |           |  |     |  |  |  |_______
 *  |  |       |  |  |  |________|  |  |  |            |  |  |  |           |  |___|  |___|  |  |  |   |  |   |  |      |  |         __|  |     |  |  |          |
 *  |__|       |__|  |______________|  |__|            |__|  |__|           |________________|  |__|    |__|  |__|      |__|        |_____|     |__|  |__________|
 *
 */
#include <string>
#include <regex>
#include <iostream>
#include <fstream>
#include <sstream>

#include "../inc/support.hpp"

//Structures
int SymbolTableEntry::nextID = 0;
SymbolTableEntry::SymbolTableEntry(std::string name, int section, int offset, bool global) {
  this->id = nextID++;
  this->name = name;
  this->section = section;
  this->offset = offset;
  this->global = global;
}
RelocationTableEntry::RelocationTableEntry(int id, int offset, int size, int type) {
  this->id = id;
  this->offset = offset;
  this->size = size;
  this->type = type;
}
EquEntry::EquEntry(int id, std::string name, int value) {
  this->id = id;
  this->name = name;
  this->value = value;
}

//Local variables
std::string currentLine;
int currentLineNumber;
std::vector<std::string> currentLineFetched;
std::vector<std::string> sections;
std::vector<std::ostringstream*> sectionContent;
int currentSection;
int currentSectionSize;
std::vector<SymbolTableEntry*> symbolTable;
std::vector<std::vector<RelocationTableEntry*>> relocationTables;
std::vector<EquEntry*> equTable;

//Regex for arguments
std::regex inputFile("^[a-zA-z0-9\\-\\_\\/]+(\\.)s$");
std::regex outputFile("^[a-zA-z0-9\\-\\_\\/]+(\\.)o$");
std::regex outputMarker("^\\-o$");

//Regex for code recognition
std::regex labelRecognition(startMarker + symbolName + "\\:" + endMarker);
std::regex directiveRecognition(startMarker + "\\." + directiveList + endMarker);
std::regex commandRecognition(startMarker + commandList + endMarker);
std::regex nameRecognition(startMarker + symbolName + endMarker);
std::regex nameRecognitionWithCommaOnEnd(startMarker + symbolName + "\\," + endMarker);
std::regex literalRecognition(startMarker + literalValue + endMarker);
//Jump regex
std::regex jumpAddressLiteralRecognition(startMarker + literalValue + endMarker);
std::regex jumpAddressSymbolRecognition(startMarker + symbolName + endMarker);
std::regex jumpAddressSymbolPCRelativeRecognition(startMarker + "\\%" + symbolName + endMarker);
std::regex jumpAddressLiteralMemoryRecognition(startMarker + "\\*" + literalValue + endMarker);
std::regex jumpAddressSymbolMemoryRecognition(startMarker + "\\*" + symbolName + endMarker);
std::regex jumpAddressRegisterRecognition(startMarker + "\\*" + registerName + endMarker);
std::regex jumpAddressRegisterMemoryRecognition(startMarker + "\\*\\[" + registerName + "\\]" + endMarker);
std::regex jumpAddressRegisterMemoryLiteralOffsetRecognition(startMarker + "\\*\\[" + registerName + "(\\s)?\\+(\\s)?" + literalValue + "\\]" + endMarker);
std::regex jumpAddressRegisterMemorySymbolOffestRecognition(startMarker + "\\*\\[" + registerName + "(\\s)?\\+(\\s)?" + symbolName + "\\]" + endMarker);
//Data regex
std::regex dataLiteralRecognition(startMarker + "\\$" + literalValue + endMarker);
std::regex dataSymbolRecognition(startMarker + "\\$" + symbolName + endMarker);
std::regex dataLiteralMemoryRecognition(startMarker + literalValue + endMarker);
std::regex dataSymbolMemoryRecognition(startMarker + symbolName + endMarker);
std::regex dataSymbolPCRelativeRecognition(startMarker + "\\%" + symbolName + endMarker);
std::regex dataRegisterRecognition(startMarker + registerName + endMarker);
std::regex dataRegisterMemoryRecognition(startMarker + "\\[" + registerName + "\\]" + endMarker);
std::regex dataRegisterMemoryLiteralOffsetRecognition(startMarker + "\\[" + registerName + "(\\s)?\\+(\\s)?" + literalValue + "\\]" + endMarker);
std::regex dataRegisterMemorySymbolOffestRecognition(startMarker + "\\[" + registerName + "(\\s)?\\+(\\s)?" + symbolName + "\\]" + endMarker);
//Other regex
std::regex availableRegisterRecognition(startMarker + registerName + endMarker);
std::regex commaOnEndRecognition(startMarker + ".*\\," + endMarker);

int readArguments(int argc, char** argv, std::string* arrayToSave) {
  if (argc==2) {

    if (regex_match(argv[1], inputFile)) {
      arrayToSave[0] = argv[1];
      arrayToSave[1] = arrayToSave[0].substr(0, arrayToSave[0].length()-2) + ".o";
    }
    else {
      std::cout << "Error!\nInvalid input file name: " << argv[1] << std::endl;
      return -1;
    }

  } else if (argc==4) {

    if (!regex_match(argv[1], outputMarker)) {
      std::cout << "Error!\nInvalid argument: " << argv[1] << std::endl;
      return -1;
    } else if (!regex_match(argv[2], outputFile)) {
      std::cout << "Error!\nInvalid output file name: " << argv[2] << std::endl;
      return -1;
    } else if (!regex_match(argv[3], inputFile)) {
      std::cout << "Error!\nInvalid input file name: " << argv[3] << std::endl;
      return -1;
    } else {
      arrayToSave[0] = argv[3];
      arrayToSave[1] = argv[2];
    }

  } else {

    std::cout << "Error!\nInvalid arguments." << std::endl;
    return -1;

  }

  return 0;
}

void initializeLocalVariables() {
  currentLine = "";
  currentLineNumber = 1;
  currentLineFetched.clear();
  sections.clear();
  currentSection = -1;
  // currentSectionSize = 0; Already doing this when adding new section. No need to invalidate cache twice.
  symbolTable.clear();
  relocationTables.clear();
  equTable.clear();
}

int fetchCurrentLine() {
  // Removing comments
  std::string line = currentLine.substr(0, currentLine.find("#"));

  std::string word;
  std::istringstream lineStream(line);

  while (lineStream >> word)
    if (regex_match(word, commaOnEndRecognition))
      currentLineFetched.push_back(word.substr(0, word.size()-1));
    else
      currentLineFetched.push_back(word);

    return 0;
}

int sectionFirstPass(std::string sectionName) {
  //Check if symbol exists in symbol table
  for (auto s : symbolTable)
    if (s->name==sectionName) {
      std::cout << "Error!\nThe section name " << sectionName << " on line " << currentLineNumber << " is already defined." << std::endl;
      return -1;
    }

  //Set section parameters and add to symbol table
  currentSection = sections.size();
  currentSectionSize = 0;
  sections.push_back(sectionName);
  symbolTable.push_back(new SymbolTableEntry(sectionName, currentSection, 0, false));

  //Add relocation table
  std::vector<RelocationTableEntry*> temp;
  temp.push_back(new RelocationTableEntry(-1, -1, -1, -1));
  relocationTables.push_back(temp);
  relocationTables.back().clear();

  return 0;
}

void printOutputFiles(std::string name) {
  std::ofstream testFile;
  testFile.open(name + ".txt");

  //Symbol table
  testFile << "Symbol table\n";
  testFile << "ID\tname\tsection\toffset\tglobal\n";
  for (auto s : symbolTable)
    testFile << s->id << " | " << s->name << " | " << (s->section>=0 ? sections[s->section] : (s->section==-1 ? "EQU" : "UNDEFINED")) << " | " << s->offset << " | " << s->global << "\n";

  //Sections
  testFile << "\nSections:\n";
  for (int i=0;i<sections.size();i++) {
    testFile << sections[i] << "\n";
    testFile << sectionContent[i]->str() << "\n";
    testFile << "ID\toffset\tsize\ttype\n";
    for (auto r : relocationTables[i])
      testFile << r->id << " | " << r->offset << " | " << r->size << " | " << relocationTypes[r->type] << "\n";
    testFile << "\n";
  }

  //EQU table
  testFile << "\nEQU\n";
  testFile << "ID\tname\tvalue\n";
  for (auto e : equTable) {
    testFile << e->id << " | " << e->name << " | " << e->value << "\n";
    symbolTable.erase(symbolTable.begin()+e->id);
  }

  testFile.close();

  testFile.open(name);

  testFile << ".symbol_table\n";

  for (auto s : symbolTable)
    testFile << s->id << "|" << s->name << "|" << s->section << "|" << s->offset << "|" << s->global << "\n";

  for(int i=0;i<sections.size();i++) {
    testFile << ".section\n";
    testFile << sectionContent[i]->str() << "\n";
    for (auto r : relocationTables[i])
      testFile << r->id << "|" << r->offset << "|" << r->size << "|" << r->type << "\n";
  }

  testFile << "\end";

  testFile.close();

}

int addSymbolFirstPass(std::string symbolName, int section, int offset, bool global) {
  for (auto s : symbolTable)
    if (s->name==symbolName) {
      std::cout << "Error!\nSymbol " << symbolName << " on line " << currentLineNumber << " is already defined." << std::endl;
      return -1;
    }

  symbolTable.push_back(new SymbolTableEntry(symbolName, section, offset, global));
  return 0;
}

int convertToInt(std::string value) {
  int valueSize = value.size();
  if (valueSize<2)
    return std::stoi(value, nullptr, 10);
  else {
    std::string prefix = value.substr(0, 2);
    if (prefix=="0x" || prefix=="0X")
      return std::stoi(value.substr(2, valueSize-2), nullptr, 16);
    else if (prefix=="0b" || prefix=="0B")
      return std::stoi(value.substr(2, valueSize-2), nullptr, 2);
    else if (prefix.substr(0,1)=="0")
      return std::stoi(value.substr(1, valueSize-1), nullptr, 8);
    else
      return std::stoi(value, nullptr, 10);
  }
}

int typeOfJumpAddress(std::string address) {
  //Return values are shuffled because prioritization
  if (regex_match(address, jumpAddressRegisterRecognition))
    return 5;
  else if (regex_match(address, jumpAddressRegisterMemoryRecognition))
    return 6;
  else if (regex_match(address, jumpAddressRegisterMemoryLiteralOffsetRecognition))
    return 7;
  else if (regex_match(address, jumpAddressRegisterMemorySymbolOffestRecognition))
    return 8;
  else if (regex_match(address, jumpAddressSymbolPCRelativeRecognition))
    return 2;
  else if (regex_match(address, jumpAddressLiteralMemoryRecognition))
    return 3;
  else if (regex_match(address, jumpAddressSymbolMemoryRecognition))
    return 4;
  else if (regex_match(address, jumpAddressLiteralRecognition))
    return 0;
  else if (regex_match(address, jumpAddressSymbolRecognition))
    return 1;
  else
    return -1;
}

int typeOfData(std::string data) {
  //Return values are shuffled because prioritization
  if (regex_match(data, dataRegisterRecognition))
    return 5;
  else if (regex_match(data, dataRegisterMemoryRecognition))
    return 6;
  else if (regex_match(data, dataRegisterMemoryLiteralOffsetRecognition))
    return 7;
  else if (regex_match(data, dataRegisterMemorySymbolOffestRecognition))
    return 8;
  else if (regex_match(data, dataSymbolPCRelativeRecognition))
    return 4;
  else if (regex_match(data, dataLiteralMemoryRecognition))
    return 2;
  else if (regex_match(data, dataSymbolMemoryRecognition))
    return 3;
  else if (regex_match(data, dataLiteralRecognition))
    return 0;
  else if (regex_match(data, dataSymbolRecognition))
    return 1;
  else
    return -1;
}

int makeSymbolGlobal(std::string symbolName) {
  for (auto s : symbolTable)
    if (s->name==symbolName) {
      s->global = true;
      return s->id;
    }
  return -1;
}

std::string integerToHexString(int value, int length) {
  std::string s;
  std::stringstream ss;
  ss.width(length*2);
  ss.fill('0');
  ss << std::hex << value;
  s = ss.str();
  if (s.size()<=length*2)
    return s;
  else
    return s.substr(s.size()-length*2);
}

int findSymbol(std::string name) {
  for (auto s : symbolTable)
    if (s->name==name)
      return s->id;
  return -1;
}

int findEquSymbol(int id) {
  for (auto e : equTable)
    if (e->id==id)
      return e->value;
  return -1;
}

int getRegisterNumber(std::string reg) {
  if (reg=="sp")
    return 6;
  else if (reg=="pc")
    return 7;
  else if (reg=="psw")
    return 8;
  else
    return std::stoi(reg.substr(1));
}

int createJumpAddress(int jumpType, std::string* returnString) {

  int id;

  if (jumpType==0) { //Immediate literal

    *returnString += "f00";
    *returnString += integerToHexString(convertToInt(currentLineFetched[1]), 2);

  } else if (jumpType==1) { //Immediate symbol

    *returnString += "f00";
    id = findSymbol(currentLineFetched[1]);

    if (id<0) {
      std::cout << "Error!\nSymbol " << currentLineFetched[1] << " is not defined, line " << currentLineNumber << "." << std::endl;
      return -1;
    }

    if (findEquSymbol(id)<0) {
      relocationTables[currentSection].push_back(new RelocationTableEntry(id, currentSectionSize+3, 2, ABS));
      *returnString += "0000";
    } else
      *returnString += integerToHexString(findEquSymbol(id), 2);

  } else if (jumpType==2) { // PC relative symbol

    *returnString += "703";
    id = findSymbol(currentLineFetched[1].substr(1));

    if (id<0) {
      std::cout << "Error!\nSymbol " << currentLineFetched[1] << " is not defined, line " << currentLineNumber << "." << std::endl;
      return -1;
    }

    if (findEquSymbol(id)>=0) {
      std::cout << "Error!\nSymbol " << currentLineFetched[1] << " is inside .equ section, line " << currentLineNumber << "." << std::endl;
      return -1;
    }

    if (symbolTable[id]->section==currentSection) { //Same section

      short int distance = symbolTable[id]->offset-currentSectionSize-5;
      *returnString += integerToHexString(distance, 2);

    } else {

      relocationTables[currentSection].push_back(new RelocationTableEntry(id, currentSectionSize+3, 2, PC_REL));
      *returnString += "0000";

    }


  } else if (jumpType==3) { // Memory direct literal

    *returnString += "f04";
    *returnString += integerToHexString(convertToInt(currentLineFetched[1].substr(1)), 2);

  } else if (jumpType==4) { //Memory direct symbol

    *returnString += "f04";
    id = findSymbol(currentLineFetched[1].substr(1));

    if (id<0) {
      std::cout << "Error!\nSymbol " << currentLineFetched[1] << " is not defined, line " << currentLineNumber << "." << std::endl;
      return -1;
    }

    if (findEquSymbol(id)<0) {
      relocationTables[currentSection].push_back(new RelocationTableEntry(id, currentSectionSize+3, 2, ABS));
      *returnString += "0000";
    } else
      *returnString += integerToHexString(findEquSymbol(id), 2);

  } else if (jumpType==5) { //Register direct

    *returnString += std::to_string(getRegisterNumber(currentLineFetched[1].substr(1))); //Add register
    *returnString += "01";

  } else if (jumpType==6) { //Register indirect

    *returnString += std::to_string(getRegisterNumber(currentLineFetched[1].substr(2, currentLineFetched[1].size()-3)));
    *returnString += "02";

  } else if (jumpType==7) { //Register indirect with literal offset

    std::string reg = currentLineFetched[1].substr(2);

    if (reg.find("w")==2) //Case for psw
      reg = reg.substr(0, 3);
    else
      reg = reg.substr(0, 2);

    *returnString += std::to_string(getRegisterNumber(reg));
    *returnString += "03";

    reg = currentLineFetched[1]; //Load full operand
    reg = reg.substr(reg.find("+")+1); //Remove everything before plus sign, including him (+1)
    reg = reg.substr(0, reg.size()-1); //Remove bracket on back

    *returnString += integerToHexString(convertToInt(reg), 2);

  } else if (jumpType==8) { //Register indirect with symbol offset

    std::string reg = currentLineFetched[1].substr(2);

    if (reg.find("w")==2) //Case for psw
      reg = reg.substr(0, 3);
    else
      reg = reg.substr(0, 2);

    *returnString += std::to_string(getRegisterNumber(reg));
    *returnString += "03";

    reg = currentLineFetched[1]; //Load full operand
    reg = reg.substr(reg.find("+")+1); //Remove everything before plus sign, including him (+1)
    reg = reg.substr(0, reg.size()-1); //Remove bracket on back

    id = findSymbol(reg);

    if (id<0) {
      std::cout << "Error!\nSymbol " << currentLineFetched[1] << " is not defined, line " << currentLineNumber << "." << std::endl;
      return -1;
    }

    if (findEquSymbol(id)<0) {
      relocationTables[currentSection].push_back(new RelocationTableEntry(id, currentSectionSize+3, 2, ABS));
      *returnString += "0000";
    } else
      *returnString += integerToHexString(findEquSymbol(id), 2);

  }

  return 0;

}

int createDataAddress(int dataType, std::string* returnString) {

  int id;

  *returnString += std::to_string(getRegisterNumber(currentLineFetched[1])); // value of regD is the same for all

  if (dataType==0) { //Immediate literal

    *returnString += "f00";
    *returnString += integerToHexString(convertToInt(currentLineFetched[2].substr(1)), 2);

  } else if (dataType==1) { //Immediate symbol

    *returnString += "f00";
    id = findSymbol(currentLineFetched[2].substr(1));

    if (id<0) {
      std::cout << "Error!\nSymbol " << currentLineFetched[2] << " is not defined, line " << currentLineNumber << "." << std::endl;
      return -1;
    }

    if (findEquSymbol(id)<0) {
      relocationTables[currentSection].push_back(new RelocationTableEntry(id, currentSectionSize+3, 2, ABS));
      *returnString += "0000";
    } else
      *returnString += integerToHexString(findEquSymbol(id), 2);

  } else if (dataType==4) { // PC relative symbol

    *returnString += "703";
    id = findSymbol(currentLineFetched[2].substr(1));

    if (id<0) {
      std::cout << "Error!\nSymbol " << currentLineFetched[2] << " is not defined, line " << currentLineNumber << "." << std::endl;
      return -1;
    }

    if (findEquSymbol(id)>=0) {
      std::cout << "Error!\nSymbol " << currentLineFetched[2] << " is inside .equ section, line " << currentLineNumber << "." << std::endl;
      return -1;
    }

    if (symbolTable[id]->section==currentSection) { //Same section

      short int distance = symbolTable[id]->offset-currentSectionSize-5;
      *returnString += integerToHexString(distance, 2);

    } else {

      relocationTables[currentSection].push_back(new RelocationTableEntry(id, currentSectionSize+3, 2, PC_REL));
      *returnString += "0000";

    }

  } else if (dataType==2) { // Memory direct literal

    *returnString += "f04";
    *returnString += integerToHexString(convertToInt(currentLineFetched[2]), 2);

  } else if (dataType==3) { //Memory direct symbol

    *returnString += "f04";
    id = findSymbol(currentLineFetched[2]);

    if (id<0) {
      std::cout << "Error!\nSymbol " << currentLineFetched[2] << " is not defined, line " << currentLineNumber << "." << std::endl;
      return -1;
    }

    if (findEquSymbol(id)<0) {
      relocationTables[currentSection].push_back(new RelocationTableEntry(id, currentSectionSize+3, 2, ABS));
      *returnString += "0000";
    } else
      *returnString += integerToHexString(findEquSymbol(id), 2);

  } else if (dataType==5) { //Register direct

    *returnString += std::to_string(getRegisterNumber(currentLineFetched[2])); //Add register
    *returnString += "01";

  } else if (dataType==6) { //Register indirect

    *returnString += std::to_string(getRegisterNumber(currentLineFetched[2].substr(1, currentLineFetched[2].size()-2)));
    *returnString += "02";

  } else if (dataType==7) { //Register indirect with literal offset

    std::string reg = currentLineFetched[2].substr(1);

    if (reg.find("w")==2) //Case for psw
      reg = reg.substr(0, 3);
    else
      reg = reg.substr(0, 2);

    *returnString += std::to_string(getRegisterNumber(reg));
    *returnString += "03";

    reg = currentLineFetched[2]; //Load full operand
    reg = reg.substr(reg.find("+")+1); //Remove everything before plus sign, including him (+1)
    reg = reg.substr(0, reg.size()-1); //Remove bracket on back

    *returnString += integerToHexString(convertToInt(reg), 2);

  } else if (dataType==8) { //Register indirect with symbol offset

    std::string reg = currentLineFetched[2].substr(1);

    if (reg.find("w")==2) //Case for psw
      reg = reg.substr(0, 3);
    else
      reg = reg.substr(0, 2);

    *returnString += std::to_string(getRegisterNumber(reg));
    *returnString += "03";

    reg = currentLineFetched[2]; //Load full operand
    reg = reg.substr(reg.find("+")+1); //Remove everything before plus sign, including him (+1)
    reg = reg.substr(0, reg.size()-1); //Remove bracket on back

    id = findSymbol(reg);

    if (id<0) {
      std::cout << "Error!\nSymbol " << currentLineFetched[2] << " is not defined, line " << currentLineNumber << "." << std::endl;
      return -1;
    }

    if (findEquSymbol(id)<0) {
      relocationTables[currentSection].push_back(new RelocationTableEntry(id, currentSectionSize+3, 2, ABS));
      *returnString += "0000";
    } else
      *returnString += integerToHexString(findEquSymbol(id), 2);

  }

  return 0;

}

int assemble(std::string inputFile, std::string outputFile) {
  initializeLocalVariables();

  bool endReached = false; // Indicator for existence of end directive in the input file

  std::ifstream inFile(inputFile);

  if (!inFile.good()) { //Empty or corrupt file
    std::cout << "Error!\nCan't read input file." << std::endl;
    return -1;
  }

  while(getline(inFile, currentLine)) { //First pass
    fetchCurrentLine();

    if (currentLineFetched.size()==0) { //Skip to next line
      currentLineNumber++;
      continue;
    }

    if (regex_match(currentLineFetched[0], labelRecognition)) { //Label
      if (currentSection<0) {
        std::cout << "Error!\nLabel must be defined inside a section, line " << currentLineNumber << "." << std::endl;
        return -1;
      }

      //Add symbol
      if(addSymbolFirstPass(currentLineFetched[0].substr(0, currentLineFetched[0].size()-1), currentSection, currentSectionSize, false)<0)
        return -1;

      //Check if more code exists on this line
      if (currentLineFetched.size()==1) { //Skip to next line
        currentLineFetched.clear();
        currentLineNumber++;
        continue;
      } else
        currentLineFetched.erase(currentLineFetched.begin());
    }

    if (regex_match(currentLineFetched[0], directiveRecognition)) { //Directive

      std::string curDir = currentLineFetched[0].substr(1);
      if (curDir==directives[0]) { //Global
        //Check length
        if (currentLineFetched.size()<2) {
          std::cout << "Error!\nMissing symbol(s) on line " << currentLineNumber << "." << std::endl;
          return -1;
        }

        for (int i=1;i<currentLineFetched.size();i++)
          if (!regex_match(currentLineFetched[i], nameRecognition)) {
            std::cout << "Error!\nInvalid list definition on line " << currentLineNumber << ", symbol " << currentLineFetched[i] << "." << std::endl;
            return -1;
          }

      } else if (curDir == directives[1]) { //Extern
        //Check length
        if (currentLineFetched.size()<2) {
          std::cout << "Error!\nMissing symbol(s) on line " << currentLineNumber << "." << std::endl;
          return -1;
        }

        for (int i=1;i<currentLineFetched.size();i++) {
          if (!regex_match(currentLineFetched[i], nameRecognition)) {
            std::cout << "Error!\nInvalid list definition on line " << currentLineNumber << ", symbol " << currentLineFetched[i] << "." << std::endl;
            return -1;
          } else
            if (addSymbolFirstPass(currentLineFetched[i], -2, 0, true)<0)
              return -1;
        }

      } else if (curDir == directives[2]) { //Section
        //Check length
        if (currentLineFetched.size()!=2) {
          std::cout << "Error!\nMissing section name on line " << currentLineNumber << "." << std::endl;
          return -1;
        }
        if (!regex_match(currentLineFetched[1], nameRecognition)) {
          std::cout << "Error!\nInvalid section name on line " << currentLineNumber << "." << std::endl;
          return -1;
        }
        if (sectionFirstPass(currentLineFetched[1])<0)
          return -1;
      } else if (curDir == directives[3]) { //Word
        //Check length
        if (currentLineFetched.size()<2) {
          std::cout << "Error!\nMissing value(s) on line " << currentLineNumber << "." << std::endl;
          return -1;
        }
        currentSectionSize += (currentLineFetched.size()-1)*2;
      } else if (curDir == directives[4]) { //Skip
        //Check length
        if (currentLineFetched.size()!=2) {
          std::cout << "Error!\nMissing value on line " << currentLineNumber << "." << std::endl;
          return -1;
        }
        if (!regex_match(currentLineFetched[1], literalRecognition)) {
          std::cout << "Error!\nInvalid literal value on line " << currentLineNumber << "." << std::endl;
          return -1;
        }
        currentSectionSize += convertToInt(currentLineFetched[1]);
      } else if (curDir == directives[5]) { //Equ
        //Check length
        if (currentLineFetched.size()!=3) {
          std::cout << "Error!\nMissing value(s) on line " << currentLineNumber << "." << std::endl;
          return -1;
        }
        if (!regex_match(currentLineFetched[1], nameRecognition)) {
          std::cout << "Error!\nInvalid symbol name " << currentLineFetched[1] << " on line " << currentLineNumber << "." << std::endl;
          return -1;
        }
        if (!regex_match(currentLineFetched[2], literalRecognition)) {
          std::cout << "Error!\nInvalid literal value " << currentLineFetched[2] << " on line " << currentLineNumber << "." << std::endl;
          return -1;
        }
        //Add symbol
        if (addSymbolFirstPass(currentLineFetched[1], -1, equTable.size()*2, false)<0)
          return -1;
        //Add to equ table
        equTable.push_back(new EquEntry(findSymbol(currentLineFetched[1]), currentLineFetched[1], convertToInt(currentLineFetched[2])));
      } else if (curDir == directives[6]) { //End
        endReached = true;
        break; // For now only this
      }

    } else if (regex_match(currentLineFetched[0], commandRecognition)) { //Command

      int formatType; //Int to hold the type

      if (currentLineFetched[0]==commands[0] || currentLineFetched[0]==commands[2] || currentLineFetched[0]==commands[4]) { // HALT, IRET, RET

        if (currentLineFetched.size()!=1) {
          std::cout << "Error!\nCommand on line " << currentLineNumber << " not properly written." << std::endl;
          return -1;
        }

        currentSectionSize += 1;

      } else if (currentLineFetched[0]==commands[3] || currentLineFetched[0]==commands[5] || currentLineFetched[0]==commands[6] || currentLineFetched[0]==commands[7] || currentLineFetched[0]==commands[8]) { //CALL, JMP, JEQ, JNE, JGT

        if (currentLineFetched.size()!=2) {
          std::cout << "Error!\nCommand on line " << currentLineNumber << " not properly written." << std::endl;
          return -1;
        }

        formatType = typeOfJumpAddress(currentLineFetched[1]);

        if (formatType<0) {
          std::cout << "Error!\nInvalid adrress format " << currentLineFetched[1] << " on line " << currentLineNumber << "." << std::endl;
          return -1;
        }

        if (formatType==5 || formatType==6)
          currentSectionSize += 3;
        else
          currentSectionSize += 5;

      } else if (currentLineFetched[0]==commands[1]) { //INT

        if (currentLineFetched.size()!=2) {
          std::cout << "Error!\nCommand on line " << currentLineNumber << " not properly written." << std::endl;
          return -1;
        }

        if (!regex_match(currentLineFetched[1], availableRegisterRecognition)) {
          std::cout << "Error!\nInvalid register format " << currentLineFetched[1] << " on line " << currentLineNumber << "." << std::endl;
          return -1;
        }
        currentSectionSize += 2;

      } else if (currentLineFetched[0]==commands[9] || currentLineFetched[0]==commands[10]) { //PUSH, POP

        if (currentLineFetched.size()!=2) {
          std::cout << "Error!\nCommand on line " << currentLineNumber << " not properly written." << std::endl;
          return -1;
        }

        if (!regex_match(currentLineFetched[1], availableRegisterRecognition)) {
          std::cout << "Error!\nInvalid register format " << currentLineFetched[1] << " on line " << currentLineNumber << "." << std::endl;
          return -1;
        }
        currentSectionSize += 3;

      } else if (currentLineFetched[0]!=commands[24] && currentLineFetched[0]!=commands[25]) { // XCHG, ADD, SUB, MUL, DIV, CMP, NOT, AND, OR, XOR, TEST, SHL, SHR

        if (currentLineFetched.size()!=3) {
          std::cout << "Error!\nCommand on line " << currentLineNumber << " not properly written." << std::endl;
          return -1;
        }

        if (!regex_match(currentLineFetched[1], availableRegisterRecognition)) {
          std::cout << "Error!\nInvalid register format " << currentLineFetched[1] << " on line " << currentLineNumber << "." << std::endl;
          return -1;
        }

        if (!regex_match(currentLineFetched[2], availableRegisterRecognition)) {
          std::cout << "Error!\nInvalid register format " << currentLineFetched[2] << " on line " << currentLineNumber << "." << std::endl;
          return -1;
        }

        currentSectionSize += 2;

      } else if (currentLineFetched[0]==commands[24] || currentLineFetched[0]==commands[25]) { //LDR, STR

        if (currentLineFetched.size()!=3) {
          std::cout << "Error!\nCommand on line " << currentLineNumber << " not properly written." << std::endl;
          return -1;
        }

        if (!regex_match(currentLineFetched[1], availableRegisterRecognition)) {
          std::cout << "Error!\nInvalid register format " << currentLineFetched[1] << " on line " << currentLineNumber << "." << std::endl;
          return -1;
        }

        formatType = typeOfData(currentLineFetched[2]);

        if (formatType<0) {
          std::cout << "Error!\nInvalid operand format " << currentLineFetched[2] << " on line " << currentLineNumber << "." << std::endl;
          return -1;
        }

        if (formatType==5 || formatType==6)
          currentSectionSize += 3;
        else
          currentSectionSize += 5;

      }

    } else { //Error
      std::cout << "Error!\nUnknown code on line " << currentLineNumber << "." << std::endl;
      return -1;
    }

    // Advance to next line
    currentLineNumber++;
    currentLineFetched.clear();
  }

  if (!endReached) { //No end defined
    std::cout << "Error!\nMissing .end directive." << std::endl;
    return -1;
  }

  //Return to file beginning
  inFile.clear();
  inFile.seekg(0);
  currentLineNumber = 1;
  currentSection = -1; //On first encounter with .section it will be incremented to first position (0, zero)
  endReached = false;

  while (getline(inFile, currentLine)) { //Second pass

    fetchCurrentLine();

    if (currentLineFetched.size()==0) { //Skip to next line
      currentLineNumber++;
      continue;
    }

    if (regex_match(currentLineFetched[0], directiveRecognition)) { //Directive

      std::string curDir = currentLineFetched[0].substr(1);
      if (curDir==directives[0]) { //Global

        for (int i=1;i<currentLineFetched.size();i++)
          if (makeSymbolGlobal(currentLineFetched[i])<0) {
            std::cout << "Error!\nSymbol " << currentLineFetched[i] << " doesn't exist in the symbol table, error on line " << currentLineNumber << "." << std::endl;
            return -1;
          }

      } else if (curDir==directives[1]) { //Extern

        //Nothing to do

      } else if (curDir==directives[2]) { //Section
        currentSection++;
        currentSectionSize = 0;
        sectionContent.push_back(new std::ostringstream);
      } else if (curDir==directives[3]) { //Word

        int id;

        for (int i=1;i<currentLineFetched.size();i++) {
          if (regex_match(currentLineFetched[i], literalRecognition)) { //Literal

            *(sectionContent[currentSection]) << integerToHexString(convertToInt(currentLineFetched[i]), 2);

          } else { //Symbol

            id = findSymbol(currentLineFetched[i]);

            if (id<0) {
              std::cout << "Error!\nSymbol " << currentLineFetched[i] << " isn't defined, line " << currentLineNumber << "." << std::endl;
              return -1;
            }

            if (findEquSymbol(id)<0) {
              relocationTables[currentSection].push_back(new RelocationTableEntry(id, currentSectionSize+(i-1)*2, 2, ABS));
              *(sectionContent[currentSection]) << integerToHexString(convertToInt("0"), 2);
            } else
              *(sectionContent[currentSection]) << integerToHexString(findEquSymbol(id), 2);

          }
        }

        currentSectionSize += (currentLineFetched.size()-1)*2;

      } else if (curDir==directives[4]) { //Skip

        int len = convertToInt(currentLineFetched[1]);
        *(sectionContent[currentSection]) << integerToHexString(0, len);
        currentSectionSize += len;

      } else if (curDir==directives[5]) { //Equ

        //Nothing to do here

      } else if (curDir==directives[6]) { //End - not finished
        // break;

        // printOutputFiles();
        // return 0;

        endReached = true;

      }

    } else if (regex_match(currentLineFetched[0], commandRecognition)) { //Command

      int formatType;
      std::string addressPart = "";

      if (currentLineFetched[0]==commands[0]) { //HALT

        *(sectionContent[currentSection]) << "00";
        currentSectionSize += 1;

      } else if (currentLineFetched[0]==commands[1]) { //INT

        *(sectionContent[currentSection]) << "10" << std::to_string(getRegisterNumber(currentLineFetched[1])) << "f";
        currentSectionSize += 2;

      } else if (currentLineFetched[0]==commands[2]) { //IRET

        *(sectionContent[currentSection]) << "20";
        currentSectionSize += 1;

      } else if (currentLineFetched[0]==commands[3]) { //CALL

        formatType = typeOfJumpAddress(currentLineFetched[1]);

        if (createJumpAddress(formatType, &addressPart)<0)
          return -1;

        *(sectionContent[currentSection]) << "30f" << addressPart;

        if (formatType==5 || formatType==6)
          currentSectionSize += 3;
        else
          currentSectionSize += 5;

      } else if (currentLineFetched[0]==commands[4]) { //RET

        *(sectionContent[currentSection]) << "40";
        currentSectionSize += 1;

      } else if (currentLineFetched[0]==commands[5] || currentLineFetched[0]==commands[6] || currentLineFetched[0]==commands[7] || currentLineFetched[0]==commands[8]) { //JMP, JEQ, JNE, JGT

        formatType = typeOfJumpAddress(currentLineFetched[1]);

        std::string jumpCode = "0"; //JMP

        if (currentLineFetched[0]==commands[6]) //JEQ
          jumpCode = "1";
        else if (currentLineFetched[0]==commands[7]) // JNE
          jumpCode = "2";
        else if (currentLineFetched[0]==commands[8]) //JGT
          jumpCode = "3";

        if (createJumpAddress(formatType, &addressPart)<0)
          return -1;

        *(sectionContent[currentSection]) << "5" << jumpCode << "f" << addressPart;

        if (formatType==5 || formatType==6)
          currentSectionSize += 3;
        else
          currentSectionSize += 5;

      } else if (currentLineFetched[0]==commands[9]) { //PUSH

        *(sectionContent[currentSection]) << "b0" << std::to_string(getRegisterNumber(currentLineFetched[1])) << "612";
        currentSectionSize += 3;

      } else if (currentLineFetched[0]==commands[10]) { //POP

        *(sectionContent[currentSection]) << "a0" << std::to_string(getRegisterNumber(currentLineFetched[1])) << "642";
        currentSectionSize += 3;

      } else if (currentLineFetched[0]==commands[11]) { //XCHG

        *(sectionContent[currentSection]) << "60" << std::to_string(getRegisterNumber(currentLineFetched[1])) << std::to_string(getRegisterNumber(currentLineFetched[2]));
        currentSectionSize += 2;

      } else if (currentLineFetched[0]==commands[12] || currentLineFetched[0]==commands[13] || currentLineFetched[0]==commands[14] || currentLineFetched[0]==commands[15] || currentLineFetched[0]==commands[16]) { //ADD, SUB, MUL, DIV, CMP

        std::string mode = "0"; //add

        if (currentLineFetched[0]==commands[13]) //sub
          mode = "1";
        else if (currentLineFetched[0]==commands[14]) //mul
          mode = "2";
        else if (currentLineFetched[0]==commands[15]) //div
          mode = "3";
        else if (currentLineFetched[0]==commands[16]) //cmp
          mode = "4";

        *(sectionContent[currentSection]) << "7" << mode << std::to_string(getRegisterNumber(currentLineFetched[1])) << std::to_string(getRegisterNumber(currentLineFetched[2]));

        currentSectionSize += 2;

      } else if (currentLineFetched[0]==commands[17] || currentLineFetched[0]==commands[18] || currentLineFetched[0]==commands[19] || currentLineFetched[0]==commands[20] || currentLineFetched[0]==commands[21]) { //NOT, AND, OR, XOR, TEST

        std::string mode = "0"; //not

        if (currentLineFetched[0]==commands[18]) //and
          mode = "1";
        else if (currentLineFetched[0]==commands[19]) //or
          mode = "2";
        else if (currentLineFetched[0]==commands[20]) //xor
          mode = "3";
        else if (currentLineFetched[0]==commands[21]) //test
          mode = "4";

        *(sectionContent[currentSection]) << "8" << mode << std::to_string(getRegisterNumber(currentLineFetched[1])) << std::to_string(getRegisterNumber(currentLineFetched[2]));

        currentSectionSize += 2;

      } else if (currentLineFetched[0]==commands[22] || currentLineFetched[0]==commands[23]) { //SHL, SHR

        std::string mode = "0"; //shl

        if (currentLineFetched[0]==commands[23]) //shr
          mode = "1";

        *(sectionContent[currentSection]) << "9" << mode << std::to_string(getRegisterNumber(currentLineFetched[1])) << std::to_string(getRegisterNumber(currentLineFetched[2]));

        currentSectionSize += 2;

      } else if (currentLineFetched[0]==commands[24]) { //LDR

        formatType = typeOfData(currentLineFetched[2]);

        if (createDataAddress(formatType, &addressPart)<0)
          return -1;

        *(sectionContent[currentSection]) << "a0" << addressPart;

        if (formatType==5 || formatType==6)
          currentSectionSize += 3;
        else
          currentSectionSize += 5;

      } else if (currentLineFetched[0]==commands[25]) { //STR

        formatType = typeOfData(currentLineFetched[2]);

        if (createDataAddress(formatType, &addressPart)<0)
          return -1;

        *(sectionContent[currentSection]) << "b0" << addressPart;

        if (formatType==5 || formatType==6)
          currentSectionSize += 3;
        else
          currentSectionSize += 5;

      }

    }

    // Advance to next line
    currentLineNumber++;
    currentLineFetched.clear();

  }

  inFile.close();

  printOutputFiles(outputFile);

  return 0;
}
