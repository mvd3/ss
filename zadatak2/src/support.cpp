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

//Structure constructors
int SymbolTableEntry::nextID = 0;
SymbolTableEntry::SymbolTableEntry(std::string name, int section, int offset, bool global) {
  this->id = nextID++;
  this->name = name;
  this->section = section;
  this->offset = offset;
  this->global = global;
}
RelocationTableEntry::RelocationTableEntry(int id, int section, int offset, int size, int type) {
  this->id = id;
  this->section = section;
  this->offset = offset;
  this->size = size;
  this->type = type;
}
TranslationTableEntry::TranslationTableEntry(int originalID, int newID) {
  this->originalID = originalID;
  this->newID = newID;
}
PlaceEntry::PlaceEntry(std::string section, int position) {
  this->section = section;
  this->position = position;
  this->ready = false;
}
MemoryBlock::MemoryBlock(int address) {
  this->address = address;
  this->content[0] = "00";
  this->content[1] = "00";
  this->content[2] = "00";
  this->content[3] = "00";
  this->content[4] = "00";
  this->content[5] = "00";
  this->content[6] = "00";
  this->content[7] = "00";
}

//Variables for processing
int sectionIndex; //Tracks the next section index
std::vector<std::string> sections;
std::vector<int> sectionSize;
std::vector<std::string> sectionContent;
std::vector<PlaceEntry*> sectionPositions;
std::vector<SymbolTableEntry*> definedSymbols;
std::vector<SymbolTableEntry*> undefinedSymbols;
std::vector<RelocationTableEntry*> relocationTable;
std::vector<std::string> currentLine;
std::vector<TranslationTableEntry*> translationTableSymbol;
std::vector<int> translationTableSection;
std::vector<MemoryBlock*> memory;

//Regex
std::regex outputFileMarker(startMarker + "\\-o" + endMarker);
std::regex outputFileName(startMarker + fileName + "\\.(hex|o)" + endMarker);
std::regex processOptionRegex(startMarker + "\\-(hex|linkable)" + endMarker);
std::regex placeArgumentRecogntion(startMarker + "\\-place\\=" + symbolName + "@" + hexValue + endMarker);
std::regex inputFileNameRecognition(startMarker + fileName + "\\.o" + endMarker);
std::regex inputFileSymbolTableElementRecognition(startMarker + "\\d+\\|" + symbolName + "\\|(\\-)?\\d+\\|\\d+\\|(0|1)" + endMarker);
std::regex inputFileSectionContentRecognition(startMarker + "[0-9a-f]+" + endMarker);
std::regex inputFileRelocationTableElementRecognition(startMarker + "\\d+\\|\\d+\\|\\d+\\|\\d+" + endMarker);

int readArguments(int argc, char** argv, std::string* arguments) {

  bool outputFile = false; //indicator for output file
  bool operation = false; //indicator for operation (hex or linkable)
  arguments[1] = ""; //Check if there are input files
  arguments[3] = ""; //If none are provided
  int i = 1;

  while (i<argc) { //Read command line aeguments

    if (regex_match(argv[i], outputFileMarker)) { //Recognizing output file

      if (outputFile) {
        std::cout << "Error!\nOutput file defined multiple times on command line." << std::endl;
        return -1;
      }

      outputFile = true;

      if (++i>=argc) { //Check out next element
        std::cout << "Error!\nOutput file name not defined." << std::endl;
        return -1;
      }

      if (!regex_match(argv[i], outputFileName)) {
        std::cout << "Error!\nInvalid output file name." << std::endl;
        return -1;
      }

      arguments[2] = argv[i++]; // move to next element

      continue; // to check the loop condition

    }

    if (regex_match(argv[i], processOptionRegex)) { //Recognizing process mode

      if (operation) {
        std::cout << "Error!\nProcessing mode already defined." << std::endl;
        return -1;
      }

      operation = true;

      arguments[0] = argv[i++]; // move to next element ++
      arguments[0] = arguments[0].substr(1); // Remove - from beginning

      continue;

    }

    if (regex_match(argv[i], placeArgumentRecogntion)) { //Recognizing section location

      std::string s(argv[i++]); //Next element

      arguments[3] += s.substr(7) + " "; // Removes "-place=" from beginning and adds a separation space

      continue;

    }

    if (regex_match(argv[i], inputFileNameRecognition)) { //Recognizing input file(s)

      arguments[1] += argv[i++]; //move to next element
      arguments[1] += " ";

      continue;

    }

    std::cout << "Error!\nInvalid command line argument " << argv[i] << "." << std::endl;
    return -1;

  }

  if (!operation) { //No operation mode specified
    std::cout << "Error!\nNo processing mode has been specifed (hex or linkable)." << std::endl;
    return -1;
  }

  if (!outputFile) { //Set name to default
    if (arguments[0]=="hex")
      arguments[2] = "default.hex";
    else
      arguments[2] = "default.o";
  }

  if (arguments[1]=="") { //Check if there are input files
    std::cout << "Error!\nNo input file(s) specified." << std::endl;
    return -1;
  }

  return 0;

}

void initializeData() {
  // sectionIndex = -1;
}

void addMemoryBlock(int startAddress, int length) {

  int floor = startAddress-startAddress%8;
  int ceiling = startAddress+length+8-(startAddress+length)%8;
  bool found = false;

  if (memory.size()>0) {
    if (memory.back()->address<floor)
      memory.push_back(new MemoryBlock(floor));
    floor += 8;
  }

  for (int i=floor;i<ceiling;i+=8)
    memory.push_back(new MemoryBlock(i));

}

void writeToMemory(int startAddress, int length, std::string data) {
  int counter = 0;
  int block = 0;
  int baseAddress = startAddress-startAddress%8;
  int diff = startAddress-baseAddress;

  while (memory[block]->address!=baseAddress) //Find the block
    block++;

  while (counter<length) {
    memory[block]->content[diff++] = data.substr(2*counter++, 2); //Write

    if (diff==8) { //Move to next memory block
      block++;
      diff = 0;
    }
  }

}

void printExecutablefile(std::string name) {
  std::ofstream outFile;
  /*
  outFile.open(name + ".txt");

  outFile << "Defined symbols\n";
  outFile << "ID | name | section | offset | global\n";

  for (auto s : definedSymbols)
    outFile << s->id << "|" << s->name << "|" << s->section << "|" << s->offset << "|" << s->global << "\n";

  outFile << "Undefined symbols\n";
  outFile << "ID | name | section | offset | global\n";

  for (auto s : undefinedSymbols)
    outFile << s->id << "|" << s->name << "|" << s->section << "|" << s->offset << "|" << s->global << "\n";

  outFile << "\nRelocation table\n";
  outFile << "ID | section | offset | size | type\n";

  for (auto r : relocationTable)
    outFile << r->id << "|" << r->section << "|" << r->offset << "|" << r->size << "|" << (r->type==0 ? "PC_REL" : "ABS") << "\n";

  outFile << "\nSection positions\n";

  for (auto p : sectionPositions)
    outFile << p->section << ":" << integerToHexString(p->position, 2) << "\n";

  for (int i=0;i<sections.size();i++) {

    outFile << sections[i] << "\n";

    outFile << sectionContent[i] << "\n";

  }

  outFile.close();
*/
  outFile.open(name);

  for (int i=0;i<sectionPositions.size();i++)
    writeToMemory(sectionPositions[i]->position, sectionSize[i], sectionContent[i]);

  for (int i=0;i<memory.size();i++)
    outFile << integerToHexString(memory[i]->address, 2) << ":" << memory[i]->content[0] << " " << memory[i]->content[1] << " " << memory[i]->content[2] << " " << memory[i]->content[3] << " " << memory[i]->content[4] << " " << memory[i]->content[5] << " " << memory[i]->content[6] << " " << memory[i]->content[7] << "\n";

  outFile.close();

}

void printLinkableFile(std::string name) {
  std::ofstream outFile;

/*
  outFile.open(name + ".txt");

  outFile << "Defined symbols\n";
  outFile << "ID | name | section | offset | global\n";

  for (int i=0;i<sections.size();i++) {
    outFile << definedSymbols.size()+undefinedSymbols.size()+i << "|" << sections[i] << "|" << translationTableSection[i] << "|0|0\n";
    for (auto s : definedSymbols) {
      if (s->section==translationTableSection[i])
        outFile << s->id << "|" << s->name << "|" << s->section << "|" << s->offset << "|" << s->global << "\n";
    }
  }

  outFile << "Undefined symbols\n";
  outFile << "ID | name | section | offset | global\n";

  for (auto s : undefinedSymbols)
    outFile << s->id << "|" << s->name << "|" << s->section << "|" << s->offset << "|" << s->global << "\n";

  outFile << "\nRelocation table\n";
  outFile << "ID | section | offset | size | type\n";

  for (auto r : relocationTable)
    outFile << r->id << "|" << r->section << "|" << r->offset << "|" << r->size << "|" << (r->type==0 ? "PC_REL" : "ABS") << "\n";

  outFile << "\nSection positions\n";

  for (auto p : sectionPositions)
    outFile << p->section << ":" << integerToHexString(p->position, 2) << "\n";

  for (int i=0;i<sections.size();i++) {

    outFile << sections[i] << "\n";

    outFile << sectionContent[i] << "\n";

  }

  outFile.close();
*/
  outFile.open(name);

  outFile << ".symbol_table\n";

  for (int i = 0;i<sections.size();i++) {
    outFile << definedSymbols.size()+undefinedSymbols.size()+i << "|" << sections[i] << "|" << i << "|0|0\n";
    for (auto s : definedSymbols)
      if (s->section==i)
        outFile << s->id << "|" << s->name << "|" << s->section << "|" << s->offset << "|" << s->global << "\n";
  }

  for (auto s : undefinedSymbols)
    outFile << s->id << "|" << s->name << "|" << s->section << "|" << s->offset << "|" << s->global << "\n";

  for (int i=0;i<sections.size();i++) {

    outFile << ".section\n";

    outFile << sectionContent[i] << "\n";

    for (int j=0;j<relocationTable.size();j++) {
      if (relocationTable[j]->section==i) {
        outFile << relocationTable[j]->id << "|" << relocationTable[j]->offset << "|" << relocationTable[j]->size << "|" << relocationTable[j]->type << "\n";
        relocationTable.erase(relocationTable.begin()+j--);
      }
    }

  }

  outFile << ".end";

  outFile.close();
}

void fetchLine(std::string line) {

  currentLine.clear();

  int pos = line.find("|");

  while(pos>0) {
    currentLine.push_back(line.substr(0, pos));
    line = line.substr(pos+1);
    pos = line.find("|");
  }

  currentLine.push_back(line);

}

int addSection() {
  int i;

  for (i=0;i<sections.size();i++)
    if (sections[i]==currentLine[1])
      break;

  if (i==sections.size()) { //New section
    sections.push_back(currentLine[1]);
    sectionSize.push_back(0);
    sectionContent.push_back("");
  }

  return i;

}

int addSymbol(bool defined, int section) {

  if (defined) { //Known location

    for (auto s : definedSymbols) //Already defined
      if (s->name==currentLine[1]) {
        std::cout << "Error!\nMultiple definitions of symbol " << currentLine[1] << "." << std::endl;
        return -1;
      }

    for (int i=0;i<undefinedSymbols.size();i++) //Check if already in undefined
      if (undefinedSymbols[i]->name==currentLine[1] && currentLine[4]=="1") { //Only a global symbol can replace an undefined
        definedSymbols.push_back(undefinedSymbols[i]); //Move to defined symbols
        definedSymbols.back()->section = section; //Write the section
        definedSymbols.back()->offset = std::stoi(currentLine[3], nullptr, 10) + sectionSize[section]; //Write offset
        definedSymbols.back()->global = currentLine[4]=="1" ? true : false; //Write global
        translationTableSymbol.push_back(new TranslationTableEntry(std::stoi(currentLine[0], nullptr, 10), undefinedSymbols[i]->id)); //Add translation table entry
        undefinedSymbols.erase(undefinedSymbols.begin()+i); //Remove from undefined symbols
        return definedSymbols.back()->id;
      }

      //New symbol
      definedSymbols.push_back(new SymbolTableEntry(currentLine[1], section, std::stoi(currentLine[3], nullptr, 10)+sectionSize[section], (currentLine[4]=="1" ? true : false)));
      translationTableSymbol.push_back(new TranslationTableEntry(std::stoi(currentLine[0], nullptr, 10), definedSymbols.back()->id));
      return definedSymbols.back()->id;

  } else { //Unknown location

    for (auto s : definedSymbols) //Check if symbol already exists in defined table
      if (s->name==currentLine[1]) {
        translationTableSymbol.push_back(new TranslationTableEntry(std::stoi(currentLine[0], nullptr, 10), s->id)) ;//Add the existing id
        return s->id;
      }

    for (auto s : undefinedSymbols) // Check if the symbol already exists in undefined table (possibilty of multiple definitions)
      if (s->name==currentLine[1]) {
        translationTableSymbol.push_back(new TranslationTableEntry(std::stoi(currentLine[0], nullptr, 10), s->id)) ;//Add the existing id
        return s->id;
      }

      // First appearance of the undefined symbol
      undefinedSymbols.push_back(new SymbolTableEntry(currentLine[1], -2, 0, (currentLine[4]=="1" ? true : false))); //Add undefined symbol
      translationTableSymbol.push_back(new TranslationTableEntry(std::stoi(currentLine[0], nullptr, 10), undefinedSymbols.back()->id)); //Add translation table entry
      return undefinedSymbols.back()->id;

  }

}

int getNewSymbolID(int id) {
  for (auto t : translationTableSymbol)
    if (t->originalID==id)
      return t->newID;

  return -1;
}

int readInputFiles(std::string files, bool undefinedCheck) {

  std::string file;
  std::istringstream fileStream(files);
  std::ifstream inFile;
  std::string line;
  int mode; //Which section and what it's doing
  int curSection;
  int id; //Used later for relocations
  int curSectionSize;

  while(fileStream >> file) { //Loop through file list

    translationTableSymbol.clear(); //Clear for this cycle
    translationTableSection.clear(); //Clear for this cycle
    mode = -1;
    sectionIndex = -1; //Section check
    curSection = -1; //This value should not appear

    inFile.open(file);

    while(getline(inFile, line)) {

      if (mode<0) { //First line

        if (line==".symbol_table")
          mode = 0;
        else {
          std::cout << "Error!\nFile " << file << " is corrupted.a" << std::endl;
          return -1;
        }

      } else if (mode==0) { //Read symbol table

        if (regex_match(line, inputFileSymbolTableElementRecognition)) {

          fetchLine(line);

          if (std::stoi(currentLine[2], nullptr, 10)>=0) { //Defined symbol

            if (sectionIndex<std::stoi(currentLine[2], nullptr, 10)) { //New section encountered

              sectionIndex = std::stoi(currentLine[2], nullptr, 10); //Update section counter

              curSection = addSection(); //New key for section

              translationTableSection.push_back(curSection); //position inside the vector represents the old key

            } else { //Symbol encountered

              if (addSymbol(true, curSection)<0)
                return -1;

            }

          } else { //Undefined symbol

            //First check if symbol is already defined


            if (addSymbol(false, -2)<0)
              return -1;

          }

        } else if (line==".section") {
          mode = 1;
          curSection = 0; //Start from the first section position
        } else {
          std::cout << "Error!\nFile " << file << " is corrupted.b" << std::endl;
          return -1;
        }

      } else if (mode==1) { //Read section content

        if (regex_match(line, inputFileSectionContentRecognition)) { //Add content and update size

          sectionContent[translationTableSection[curSection]] += line; //Add content from this section to aggregated section
          curSectionSize = line.size()/2; //Later used for relocation
          sectionSize[translationTableSection[curSection]] += curSectionSize; //Length of this section

        } else {
          std::cout << "Error!\nFile " << file << " is corrupted.c" << std::endl;
          return -1;
        }

        mode = 2;

      } else if (mode==2) { //Read relocation table entry

        if (regex_match(line, inputFileRelocationTableElementRecognition)) {

          fetchLine(line);

          id = getNewSymbolID(std::stoi(currentLine[0], nullptr, 10)); //new id

          relocationTable.push_back(new RelocationTableEntry(id, translationTableSection[curSection], std::stoi(currentLine[1], nullptr, 10)+sectionSize[translationTableSection[curSection]]-curSectionSize, std::stoi(currentLine[2], nullptr, 10), std::stoi(currentLine[3], nullptr, 10)));

          // std::cout << relocationTable.back()->id << "|" << relocationTable.back()->section << "|" << relocationTable.back()->offset << "|" << relocationTable.back()->size << "|" << (relocationTable.back()->type==0 ? "PC_REL" : "ABS") << std::endl;

        } else if (line==".section") {
          mode = 1;
          curSection++; //Increase for the next section
        } else if (line==".end")
          mode = -1;
        else {
          std::cout << "Error!\nFile " << file << " is corrupted.d" << std::endl;
          std::cout << line << std::endl; //Some d is appearing here
          return -1;
        }

      }

    }

    inFile.close();

  }

  if (undefinedSymbols.size()>0 && undefinedCheck) { //Undefined symbols found
    std::cout << "Error!\nThese symbols are referenced, but not defined:";
    for (auto s : undefinedSymbols)
      std::cout << "\n" << s->name;
    std::cout << std::endl;
    return -1;
  }

  return 0;

}

int getSectionPosition(std::string sectionName) {
  for (int i=0;i<sections.size();i++)
    if (sections[i]==sectionName)
      return i;
  return -1;
}

int managePositions(std::string positions) {

  std::istringstream line(positions);
  std::string word;
  std::vector<std::string> section;
  std::vector<int> start;
  int monkey;
  int startPosition = 0; //Start position for the rest of sections
  bool added;

  while (line >> word) { //Extract section@address

    added = false;
    monkey = word.find("@");
    if (section.size()>0) { //Sort start positions
      for (int i=0;i<section.size();i++) {
        if (start[i]>std::stoi(word.substr(monkey+3), nullptr, 16)) {
          section.insert(section.begin()+i, word.substr(0, monkey));
          start.insert(start.begin()+i, std::stoi(word.substr(monkey+3), nullptr, 16));
          added=true;
          break;
        }
      }
    }
    if (!added) {
      section.push_back(word.substr(0, monkey));
      start.push_back(std::stoi(word.substr(monkey+3), nullptr, 16));
    }
  }

  //Add all sections to position vector
  for (std::string s : sections)
    sectionPositions.push_back(new PlaceEntry(s, 0));

  for (int i=0;i<section.size();i++) { //Placing the prescribed sections
    for (int j=0;j<sectionPositions.size();j++) {
      if (section[i]==sectionPositions[j]->section && startPosition<=start[i] && !sectionPositions[j]->ready) {
        sectionPositions[j]->position = start[i];
        sectionPositions[j]->ready = true;
        startPosition = start[i] + sectionSize[j];
        addMemoryBlock(start[i], sectionSize[j]);
      } else if (section[i]==sectionPositions[j]->section && startPosition>start[i] && !sectionPositions[j]->ready) { //Source of error
        std::cout << "I'm here" << std::endl;
        std::cout << "Error!\nSection " << section[i] << " is overlaping with " << section[i-1] << " section." << std::endl;
        return -1;
      }
    }
  }

  //Place the rest of sections
  for (int i=0;i<sectionPositions.size();i++)
    if (!sectionPositions[i]->ready) {
      sectionPositions[i]->position = startPosition;
      sectionPositions[i]->ready = true;
      addMemoryBlock(startPosition, sectionSize[i]);
      startPosition += sectionSize[i];
    }

  if (startPosition>65536) {
    std::cout << "Error!\nSection(s) are out of memory bounds." << std::endl;
    return -1;;
  }

  return 0;

}

SymbolTableEntry* getSymbol(int id) {
  for (auto s : definedSymbols)
    if (s->id==id)
      return s;

  return nullptr;
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

int resolveRelocations() {

  SymbolTableEntry* symbol;

  // for (auto r : relocationTable)
  //   std::cout << r->id << "|" << r->section << "|" << r->offset << "|" << r->size << "|" << (r->type==0 ? "PC_REL" : "ABS") << std::endl;

  // for (auto t : translationTableSymbol)
  //   std::cout << t->originalID << ":" << t->newID << std::endl;

  for (auto r : relocationTable) {

    symbol = getSymbol(r->id);

    if (symbol==nullptr) {
      std::cout << "Error!\nRelocation tables are corrupt." << std::endl;
      std::cout << r->id << "|" << r->section << "|" << r->offset << "|" << r->size << "|" << (r->type==0 ? "PC_REL" : "ABS") << std::endl;
      return -1;
    }

    if (r->type) { //Absolute

      sectionContent[r->section] = sectionContent[r->section].substr(0, r->offset*2) + integerToHexString(sectionPositions[symbol->section]->position+symbol->offset, 2) + sectionContent[r->section].substr((r->offset+r->size)*2);

    } else { //PC relative

      short int addr = sectionPositions[symbol->section]->position+symbol->offset-sectionPositions[r->section]->position-r->offset-r->size; // addr = (jump_addr_section+offset)-(section_base+offset+size)
      sectionContent[r->section] = sectionContent[r->section].substr(0, r->offset*2) + integerToHexString(addr, 2) + sectionContent[r->section].substr((r->offset+r->size)*2);

    }

  }

  return 0;

}

int resolveRelocationsLinkable() {

  SymbolTableEntry* symbol;

  for (int i=0;i<relocationTable.size();i++) {

    if (!relocationTable[i]->type) { //PC relative

      symbol = getSymbol(relocationTable[i]->id);

      if (symbol==nullptr) //If the symbol is undefined
        continue;

      if (relocationTable[i]->section==symbol->section) {

        short int addr = symbol->offset-relocationTable[i]->offset-relocationTable[i]->size;
        sectionContent[relocationTable[i]->section] = sectionContent[relocationTable[i]->section].substr(0, relocationTable[i]->offset*2) + integerToHexString(addr, 2) + sectionContent[relocationTable[i]->section].substr((relocationTable[i]->offset+relocationTable[i]->size)*2);

        //Remove from relocations
        relocationTable.erase(relocationTable.begin()+i--);

      }

    }

  }

  return 0;

}

int linkExecutable(std::string* arguments) {

  if (readInputFiles(arguments[1], true)<0)
    return -1;

  if (managePositions(arguments[3])<0)
    return -1;

  if (resolveRelocations()<0)
    return -1;

  printExecutablefile(arguments[2]);

  return 0;

}

int linkLinkable(std::string* arguments) {

  if (readInputFiles(arguments[1], false)<0)
    return -1;

  if (resolveRelocationsLinkable()<0)
    return -1;

  printLinkableFile(arguments[2]);

  return 0;
}
