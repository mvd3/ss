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
#include <vector>
#include <sstream>

/*
Structures for further use
*/
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
  int id; //Symbol id from symbol table
  int offset;
  int size;
  int type;
  RelocationTableEntry(int id, int offset, int size, int type);
};

struct EquEntry {
  int id;
  std::string name;
  int value;
  EquEntry(int id, std::string name, int value);
};

/*
Variables used by functions.
*/
extern std::string currentLine;
extern int currentLineNumber;
extern std::vector<std::string> currentLineFetched;
extern std::vector<std::string> sections;
extern std::vector<std::ostringstream*> sectionContent;
extern int currentSection;
extern int currentSectionSize;
extern std::vector<SymbolTableEntry*> symbolTable;
extern std::vector<std::vector<RelocationTableEntry*>> relocationTables;
extern std::vector<EquEntry*> equTable;

/*
String for constructing regex
*/
const std::string startMarker = "^";
const std::string endMarker = "$";
const std::string symbolName = "[a-zA-Z\\_\\.][0-9a-zA-Z\\_\\.]*";
const std::string literalValue = "([0-9]|[1-9][0-9]+|0(b|B)[0-1]+|0(x|X)[0-9a-fA-F]+|0[0-7]+)";
const std::string directiveList = "(global|extern|section|word|skip|equ|end)";
const std::string commandList = "(halt|int|iret|call|ret|jmp|jeq|jne|jgt|push|pop|xchg|add|sub|mul|div|cmp|not|and|or|xor|test|shl|shr|ldr|str)";
const std::string registerName = "(r[0-7]|sp|pc|psw)";

/*
Regex for processing arguments
*/
extern std::regex inputFile;
extern std::regex outputFile;
extern std::regex outputMarker;

/*
Regex for code recognition
*/
extern std::regex labelRecognition;
extern std::regex directiveRecognition;
extern std::regex commandRecognition;
extern std::regex nameRecognition;
extern std::regex nameRecognitionWithCommaOnEnd;
extern std::regex literalRecognition;
//Jump regex
extern std::regex jumpAddressLiteralRecognition;
extern std::regex jumpAddressSymbolRecognition;
extern std::regex jumpAddressSymbolPCRelativeRecognition;
extern std::regex jumpAddressLiteralMemoryRecognition;
extern std::regex jumpAddressSymbolMemoryRecognition;
extern std::regex jumpAddressRegisterRecognition;
extern std::regex jumpAddressRegisterMemoryRecognition;
extern std::regex jumpAddressRegisterMemoryLiteralOffsetRecognition;
extern std::regex jumpAddressRegisterMemorySymbolOffestRecognition;
//Data regex
extern std::regex dataLiteralRecognition;
extern std::regex dataSymbolRecognition;
extern std::regex dataLiteralMemoryRecognition;
extern std::regex dataSymbolMemoryRecognition;
extern std::regex dataSymbolPCRelativeRecognition;
extern std::regex dataRegisterRecognition;
extern std::regex dataRegisterMemoryRecognition;
extern std::regex dataRegisterMemoryLiteralOffsetRecognition;
extern std::regex dataRegisterMemorySymbolOffestRecognition;
//Other
extern std::regex availableRegisterRecognition;
extern std::regex commaOnEndRecognition;

/*
Arrays for directives and commans
*/
const std::string directives[] = {"global", "extern", "section", "word", "skip", "equ", "end"};
const std::string commands[] = {"halt", "int", "iret", "call", "ret", "jmp", "jeq", "jne", "jgt", "push", "pop", "xchg", "add", "sub", "mul", "div", "cmp", "not", "and", "or", "xor", "test", "shl", "shr", "ldr", "str"};
const std::string relocationTypes[] = {"PC relative", "Absolute"};

/*
Enumerations for positions
*/
enum DIRECTIVES {GLOBAL, EXTERN, SECTION, WORD, SKIP, EQU, END};
enum COMMANDS {HALT, INT, IRET, CALL, RET, JMP, JEQ, JNE, JGT, PUSH, POP, XCHG, ADD, SUB, MUL, DIV, CMP, NOT, AND, OR, XOR, TEST, SHL, SHR, LDR, STR};
enum RELOCATION_TYPES {PC_REL, ABS};


/*
Reading arguments from the command line, saves the input file name on position 0 in arrayToSave and the output file name to position 1
Returns 0 if executed correctly, else a negative integer
*/
int readArguments(int argc, char** argv, std::string* arrayToSave);

/*
Initializes all local variables used here.
*/
void initializeLocalVariables();

/*
Fetches all the important elemenst from the current line, also removes comments.
Puts all the data inside currentLineFetched and updates indicator currentLineEmpty.
In case of an error returns a negative value, else 0.
*/
int fetchCurrentLine();

/*
Processes section directive during the first pass.
The argument is the name of the section you want to add.
On correct execution returns zero, else a negative integer (this method writes the error message).
*/
int sectionFirstPass(std::string sectionName);

/*
For now this function is used only for development
*/
void printOutputFiles(std::string name);

/*
Adds elements to symbol table during the first pass, argument is the name of the symbol.
Returns 0 on correct execution, else a negative integer value.
*/
int addSymbolFirstPass(std::string symbolName, int section, int offset, bool global);

/*
Converts literal value in string format to integer value.
*/
int convertToInt(std::string value);

/*
Determines jump address type.
Returns a negative integer value if address incorret.
*/
int typeOfJumpAddress(std::string address);

/*
Determines data type.
Returns a negative integer value if address incorret.
*/
int typeOfData(std::string data);

/*
Sets the global indicator to true in symbol table.
On correct execution returns the id of the symbol, else a negative integer value.
*/
int makeSymbolGlobal(std::string symbolName);

/*
Converts the integer value to a hexadecimal value in string representation.
length determines the max length (in bytes).
*/
std::string integerToHexString(int value, int length);

/*
Searches the symbol table and returns the id, if the symbol is not found returns a negative integer value.
*/
int findSymbol(std::string name);

/*
Returns the value of equ symbol if found, else a negative integer.
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
IMPORTANT: EQU TABLE CONTAIONS ONLY POSITIVE INTEGER VALUES.
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1
*/
int findEquSymbol(int id);

/*
Returns the register numeration.
*/
int getRegisterNumber(std::string reg);

/*
Returns second to fifth byte of processor instruction.
Negative value on incorrect execution, else zero.
*/
int createJumpAddress(int jumpType, std::string* returnString);

/*
Returns second to fifth byte of processor instruction.
Negative value on incorrect execution, else zero.
*/
int createDataAddress(int dataType, std::string* returnString);

/*
Assemble input file.
On correct execution returns 0, else a negative integer value.
*/
int assemble(std::string inputFile, std::string outputFile);
