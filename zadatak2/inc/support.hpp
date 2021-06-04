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

//Strings for constructing regex
const std::string startMarker = "^";
const std::string endMarker = "$";
const std::string fileName = "[a-zA-Z\\_\\.\\-\\/][0-9a-zA-Z\\_\\.\\-\\/]*";
const std::string symbolName = "[a-zA-Z\\_\\.][0-9a-zA-Z\\_\\.]*";
const std::string hexValue = "0[x|X][0-9a-f]+";

//Structures
struct SymbolTableEntry {
  int id;
  std::string name;
  int section;
  int offset;
  bool global;
  SymbolTableEntry(std::string name, int section, int offset, bool global);
private:
  static int nextID;
};

struct RelocationTableEntry {
  int id;
  int section;
  int offset;
  int size;
  int type;
  RelocationTableEntry(int id, int section, int offset, int size, int type);
};

struct TranslationTableEntry {
  int originalID;
  int newID;
  TranslationTableEntry(int originalID, int newID);
};

struct PlaceEntry {
  std::string section;
  int position;
  bool ready;
  PlaceEntry(std::string section, int position);
};

struct MemoryBlock {
  int address;
  std::string content[8];
  MemoryBlock(int address);
};

/*
Reads arguments and stores them insde argmunents array
Returns 0 on correct execution, else a negative integer value
*/
int readArguments(int argc, char** argv, std::string* arguments);

/*
Initializes the data.
*/
void initializeData();

/*
Adds memory block(s).
*/
void addMemoryBlock(int startAddress, int length);

/*
Write to memory block(s)
*/
void writeToMemory(int startAddress, int length, std::string data);

/*
Prints the .hex file
*/
void printExecutablefile(std::string name);

/*
Prints the .o file
*/
void printLinkableFile(std::string name);

/*
Removes '|' from the line and puts wordy by word in currentLine.
*/
void fetchLine(std::string line);

/*
Adds new section (or checks if it already exists).
Returns the position of the section inside vectors.
*/
int addSection();

/*
Adds new symbol to symbol table.
Returns a non negative integer ID on correct execution, else a negative integer value.
The input argument tells if the symbol should be added to the defined table or undefined.
*/
int addSymbol(bool defined, int section);

/*
Returns the new ID of the symbol from the aggregated table.
Returns a negative integer value if not found (which shouldn't happen).
*/
int getNewSymbolID(int id);

/*
Read all input files for hex.
undefinedCheck checks if there are any undefined symbols.
Returns 0 on correct execution, else a nefative integer value.
*/
int readInputFiles(std::string files, bool undefinedCheck);

/*
Returns the position of section inside the vectors.
If section doesn't exists returns a negative integer value.
*/
int getSectionPosition(std::string sectionName);

/*
Manages the -place arguments.
Returns 0 on correct exectuon, else a negative integer value.
*/
int managePositions(std::string positions);

/*
Does what it says.
Length is in bytes.
*/
std::string integerToHexString(int value, int length);

/*
Returns symbol.
*/
SymbolTableEntry* getSymbol(int id);

/*
Resolves relocations.
*/
int resolveRelocations();

/*
Resolves relocations for linkable processing.
*/
int resolveRelocationsLinkable();

/*
Does the magic for -hex.
Return 0 on correct execution, else a negative integer.
*/
int linkExecutable(std::string* arguments);

/*
Does the magic for -linkable.
Returns 0 on correct execution, else a negative integer value.
*/
int linkLinkable(std::string* arguments);
