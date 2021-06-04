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

//Start addresses for memory mapped registers (their size is 2 bytes)
const int term_out = 65280;
const int term_in = 65282;
const int tim_cfg = 65296;
const int sp = 6;
const int pc = 7;
const int psw = 8;

//PSW bit positions
const int psw_z = 0;
const int psw_o = 1;
const int psw_c = 2;
const int psw_n = 3;
const int psw_tr = 13;
const int psw_tl = 14;
const int psw_i = 15;

//Timer
const int timerValues[] = {1, 2, 3, 4, 10, 20, 60, 120};

//Terminal initialization
const char terminalName[] = "/dev/tty";

/*
Prepares all the things for emulation.
*/
void prepareEmulation();

/*
Loads the file content into memory.
Returns 0 on correct execution, else a negative integer value.
*/
int loadFile(std::string file);

/*
Function for timer thread.
*/
void timer();

/*
Function for terminal thread.
*/
void terminal();

/*
Sets up jump address.
On correct execution returns, else a negative integer value.
*/
int setUpJumpAddress(unsigned short int s, unsigned short int u, unsigned short int a, signed short int d);

int loadData(unsigned short int dest, unsigned short int s, unsigned short int u, unsigned short int a, signed short int d);

int storeData(unsigned short int dest, unsigned short int s, unsigned short int u, unsigned short int a, signed short int d);

void jumpToInterrupt(int interruptNum);

/*
Function for cpu thread.
*/
void cpu();

/*
Emulates the file execution.
In case of any error returns a negative integer value.
*/
int emulate(std::string file);
