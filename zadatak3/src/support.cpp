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
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <regex>
#include <thread>
#include <chrono>
#include <atomic>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>

#include "../inc/support.hpp"

//Computer stuff
std::vector<unsigned char> memory;
std::vector<unsigned short int> registers;

//Regex
std::regex memoryLineRecognition("^[0-9a-f]{4}\\:([0-9a-f]{2}\\s?){8}$");

//Timer variables
std::atomic_ushort timerTicks;
std::atomic_bool timerAlive;

//Terminal variables
std::atomic_bool terminalAlive;
std::atomic_bool termOutModified;
int terminalID;
unsigned char readContent;
struct termios terminalConfiguration;
struct termios oldTerminalConfiguration;


//CPU variables
unsigned short int opCode;
unsigned short int regDest;
unsigned short int regSrc;
unsigned short int upMode;
unsigned short int addrMode;
signed short int data;
unsigned short int temp;

//Interrupt indicators
std::atomic_bool entry_0;
std::atomic_bool entry_1;
std::atomic_bool entry_2;
std::atomic_bool entry_3;
std::atomic_bool entry_4;
std::atomic_bool entry_5;
std::atomic_bool entry_6;
std::atomic_bool entry_7;

void prepareEmulation() {

  //Create memory
  for (int i=0;i<65536;i++)
    memory.push_back(0);

  //Create registers
  for (int i=0;i<9;i++)
    registers.push_back(0);

}

int loadFile(std::string file) {

  std::ifstream inFile(file);
  std::string line;
  std::string word;
  int baseAddress;

  if (!inFile.good()) {
    std::cout << "Error!\nCan't open input file." << std::endl;
    return -1;
  }

  while (getline(inFile, line)) { //Load into memory

    if (!regex_match(line, memoryLineRecognition)) { //Check if line is correct
      std::cout << "Error!\nInput file is corrupt.\n" << line << std::endl;
      return -1;
    }

    std::istringstream lineStream(line); //Load line

    lineStream >> word; //Memory address

    baseAddress = std::stoi(word.substr(0, 4), nullptr, 16); //Base address in integer format

    word = word.substr(5); //Remove addr: part

    for (int i=0;i<8;i++) {
      memory[baseAddress+i] = std::stoi(word, nullptr, 16);
      lineStream >> word;
    }

  }

  inFile.close();

  return 0;
}

void timer() {

  while (timerAlive) {

    if (++timerTicks>=timerValues[memory[tim_cfg+1]%8]) { //Signal interrupt

      entry_2 = true; //Set interrupt indicator

      timerTicks = 0; //Restart counter
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500)); //Go to sleep

  }

}

void terminal() {

  //Open terminal connection
  terminalID = open(terminalName, O_RDWR | O_NONBLOCK);

  if (terminalID<0) { //Check

    std::cout << "Error!\nCan't open terminal." << std::endl;
    terminalAlive = false;

  } else if (tcgetattr(terminalID, &terminalConfiguration)<0 || tcgetattr(terminalID, &oldTerminalConfiguration)<0) { //Check terminal configuration

    std::cout << "Error!\nCan't load terminal configuration." << std::endl;
    terminalAlive = false;

  } else { //Configure terminal


  terminalConfiguration.c_cflag = CS8 | CREAD | CLOCAL;
  terminalConfiguration.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
  terminalConfiguration.c_cc[VMIN] = 1;
  terminalConfiguration.c_cc[VTIME] = 0;

  cfsetospeed(&terminalConfiguration, B9600);
  cfsetispeed(&terminalConfiguration, B9600);

  tcsetattr(terminalID, TCSANOW, &terminalConfiguration);

  }

  while (terminalAlive) { //Terminal working


    if (read(terminalID, &readContent, 1)>0) { //Read from terminal
      //Write data
      memory[term_in+1] = readContent;

      //Send interrupt signal
      entry_3 = true;
    }

    //Print the change
    if (termOutModified) { //term_out has been changed

      //Reset indicator
      termOutModified = false;

      //Print out content
      write(terminalID, &memory[term_out+1], 1);

    }

  }

  //Finish line
  std::cout << std::endl;

  //Restore previous configuration
  tcsetattr(terminalID, TCSANOW, &oldTerminalConfiguration);

  //Close terminal connection
  close(terminalID);

}

int setUpJumpAddress(unsigned short int s, unsigned short int u, unsigned short int a, signed short int d) {

  if (a==0) { //immediate

    registers[pc] = (unsigned short int) d;

  } else if (a==1) { //register direct

    if (s>8) //invalid register value
      return -1;

    registers[pc] = registers[s];

  } else if (a==2) { //register indirect

    if (s>8) //invalid register value
      return -1;

    //Update before
    if (u==1)
      registers[s] -= 2;
    else if (u==2)
      registers[s] += 2;

    registers[pc] = memory[registers[s]] << 8;
    registers[pc] |= memory[registers[s]+1];

    //Update after
    if (u==3)
      registers[s] -= 2;
    else if (u==4)
      registers[s] += 2;

  } else if (a==3) {  //register indirect with offset

    if (s>8) //invalid register value
      return -1;

    //Update before
    if (u==1)
      registers[s] -= 2;
    else if (u==2)
      registers[s] += 2;

    registers[pc] = memory[registers[s]+d] << 8;
    registers[pc] |= memory[registers[s]+d+1];

    //Update after
    if (u==3)
      registers[s] -= 2;
    else if (u==4)
      registers[s] += 2;

  } else if (a==4) { //memory direct

    registers[pc] = memory[(unsigned short int) d] << 8;
    registers[pc] |= memory[((unsigned short int) d)+1];

  } else if (a==5) { //register direct with offset

    if (s>8) //invalid register value
      return -1;

    //Update before
    if (u==1)
      registers[s] -= 2;
    else if (u==2)
      registers[s] += 2;

    registers[pc] = registers[s]+d;

    //Update after
    if (u==3)
      registers[s] -= 2;
    else if (u==4)
      registers[s] += 2;

  } else { //Error

    return -1;

  }

  return 0;

}

int loadData(unsigned short int dest, unsigned short int s, unsigned short int u, unsigned short int a, signed short int d) {

  if (a==0) { //immediate

    registers[dest] = (unsigned short int) d;

  } else if (a==1) { //register direct

    if (s>8) //invalid register value
      return -1;

    registers[dest] = registers[s];

  } else if (a==2) { //register indirect

    if (s>8) //invalid register value
      return -1;

    //Update before
    if (u==1)
      registers[s] -= 2;
    else if (u==2)
      registers[s] += 2;

    registers[dest] = memory[registers[s]] << 8;
    registers[dest] |= memory[registers[s]+1];

    //Update after
    if (u==3)
      registers[s] -= 2;
    else if (u==4)
      registers[s] += 2;

  } else if (a==3) {  //register indirect with offset

    if (s>8) //invalid register value
      return -1;

    //Update before
    if (u==1)
      registers[s] -= 2;
    else if (u==2)
      registers[s] += 2;

    registers[dest] = memory[registers[s]+d] << 8;
    registers[dest] |= memory[registers[s]+d+1];

    //Update after
    if (u==3)
      registers[s] -= 2;
    else if (u==4)
      registers[s] += 2;

  } else if (a==4) { //memory direct

    registers[dest] = memory[(unsigned short int) d] << 8;
    registers[dest] |= memory[((unsigned short int) d)+1];

  } else if (a==5) { //register direct with offset

    if (s>8) //invalid register value
      return -1;

    //Update before
    if (u==1)
      registers[s] -= 2;
    else if (u==2)
      registers[s] += 2;

    registers[dest] = registers[s]+d;

    //Update after
    if (u==3)
      registers[s] -= 2;
    else if (u==4)
      registers[s] += 2;

  } else { //Error

    return -1;

  }

  return 0;

}

int storeData(unsigned short int dest, unsigned short int s, unsigned short int u, unsigned short int a, signed short int d) {

  if (a==0) { //immediate - error

    return -1;

  } else if (a==1) { //register direct

    if (s>8) //invalid register value
      return -1;

    registers[s] = registers[dest];

  } else if (a==2) { //register indirect

    if (s>8) //invalid register value
      return -1;

    //Update before
    if (u==1)
      registers[s] -= 2;
    else if (u==2)
      registers[s] += 2;

    memory[registers[s]] = registers[dest] >> 8;
    memory[registers[s]+1] = registers[dest];

    if (registers[s]>=term_out-1 && registers[s]<=term_out+1) //term_out has been modified
      termOutModified = true;

    //Update after
    if (u==3)
      registers[s] -= 2;
    else if (u==4)
      registers[s] += 2;

  } else if (a==3) {  //register indirect with offset

    if (s>8) //invalid register value
      return -1;

    //Update before
    if (u==1)
      registers[s] -= 2;
    else if (u==2)
      registers[s] += 2;

    memory[registers[s]+d] = registers[dest] >> 8;
    memory[registers[s]+d+1] = registers[dest];

    if (registers[s]+d>=term_out-1 && registers[s]+d<=term_out+1) //term_out has been modified
      termOutModified = true;

    //Update after
    if (u==3)
      registers[s] -= 2;
    else if (u==4)
      registers[s] += 2;

  } else if (a==4) { //memory direct

    memory[(unsigned short int) d] = registers[dest] >> 8;
    memory[((unsigned short int) d)+1] = registers[dest];

    if (((unsigned short int) d)>=term_out-1 && ((unsigned short int) d)<=term_out+1) //term_out has been modified
      termOutModified = true;

  } else if (a==5) { //register direct with offset -- makes no sense -- error

    return -1;

  } else { //Error

    return -1;

  }

  return 0;

}

void jumpToInterrupt(int interruptNum) {

  //Push PC
  memory[--registers[sp]] = registers[pc] & 0xff; //odd byte
  memory[--registers[sp]] = registers[pc] >> 8; //even byte

  //Push PSW
  memory[--registers[sp]] = registers[psw] & 0xff; //odd byte
  memory[--registers[sp]] = registers[psw] >> 8; //even byte

  //Write to PC
  registers[pc] = memory[interruptNum*2] << 8;
  registers[pc] |= memory[interruptNum*2+1];

}

void cpu() {

  //Starting the cpu (load start address)
  registers[pc] = (memory[registers[pc]] << 8) + memory[registers[pc]+1];

  registers[sp] = term_out; //Start position for stack pointer. Full descending.

  while(true) { //Executing instructions

    opCode = memory[registers[pc]]; //Read op code

    if (opCode==0) { //HALT

      //Increase PC
      registers[pc]++;

      break;

    } else if (opCode==0x10) { //INT

      //Load data
      regDest = memory[registers[pc]+1] >> 4; //Set destination register

      //Increase PC
      registers[pc] += 2;

      if (regDest>8) { //Invalid insctruction

        //Set up interrupt indicator
        entry_1 = true;

      } else { //Continue instruction

        //Go to interrupt
        jumpToInterrupt(registers[regDest]%8);

        //Mask all other interrupts
        if (registers[regDest]%8<2)
        registers[psw] |= 1 << psw_tr; // Mask timer interrupts
        if (registers[regDest]%8<3)
        registers[psw] |= 1 << psw_tl; // Mask terminal interrupts
        registers[psw] |= 1 << psw_i; // Mask all other interrupts

      }

    } else if (opCode==0x20) { //IRET

      //Increase PC
      registers[pc]++;

      //Pop PSW
      registers[psw] = memory[registers[sp]++] << 8; //even byte
      registers[psw] |= memory[registers[sp]++];  //odd byte

      //Pop PC
      registers[pc] = memory[registers[sp]++] << 8; //even byte
      registers[pc] |= memory[registers[sp]++];  //odd byte

    } else if (opCode==0x30) { //CALL

      //Read instruction
      regSrc = memory[registers[pc]+1] & 0xf;
      upMode = memory[registers[pc]+2] >> 4;
      addrMode = memory[registers[pc]+2] & 0xf;

      //Load data if needed and increase PC
      if (addrMode==1 || addrMode==2) { //Without data
        registers[pc] += 3;
      } else { //With data
        data = memory[registers[pc]+3] << 8;
        data |= memory[registers[pc]+4];
        registers[pc] += 5;
      }

      //Push PC
      memory[--registers[sp]] = registers[pc] & 0xff; //odd byte
      memory[--registers[sp]] = registers[pc] >> 8; //even byte

      //Write operand to PC
      if (setUpJumpAddress(regSrc, upMode, addrMode, data)<0) { //Error

        //Set up interrupt indicator
        entry_1 = true;

      }

    } else if (opCode==0x40) { //RET

      //Increase pc
      registers[pc]++;

      //Pop PC
      registers[pc] = memory[registers[sp]++] << 8; //even byte
      registers[pc] |= memory[registers[sp]++];  //odd byte

    } else if ((opCode&0xF0)==0x50) { //JMP, JEQ, JNE, JGT

      //Read instruction
      regSrc = memory[registers[pc]+1] & 0xf;
      upMode = memory[registers[pc]+2] >> 4;
      addrMode = memory[registers[pc]+2] & 0xf;

      //Load data if needed and increase PC
      if (addrMode==1 || addrMode==2) { //Without data
        registers[pc] += 3;
      } else { //With data
        data = memory[registers[pc]+3] << 8;
        data |= memory[registers[pc]+4];
        registers[pc] += 5;
      }

      //Check jump type
      if ((opCode&0xF)==0 || ((opCode&0xF)==1 && (registers[psw] & (1 << psw_z))) || ((opCode&0xF)==2 && !(registers[psw] & (1 << psw_z))) || ((opCode&0xF)==3 && !((registers[psw] & (1 << psw_n))^(registers[psw] & (1 << psw_o)))&!(registers[psw] & (1 << psw_z)))) { //Jump condition

        //Write operand to PC
        if (setUpJumpAddress(regSrc, upMode, addrMode, data)<0) { //Error

          //Set up interrupt indicator
          entry_1 = true;

        }

      } else { //Error

        //Set up interrupt indicator
        entry_1 = true;

      }

    } else if (opCode==0x60) { //XCHG

      //Load registers
      regDest = memory[registers[pc]+1] >> 4;
      regSrc = memory[registers[pc]+1] & 0xF;

      //Increase PC
      registers[pc] += 2;

      if (regDest>8 || regSrc>8) { //Error

        //Set up interrupt indicator
        entry_1 = true;

      } else { //Continue instruction

        //Exchange
        temp = registers[regDest];
        registers[regDest] = registers[regSrc];
        registers[regSrc] = temp;

      }

    } else if ((opCode&0xF0)==0x70) { //ADD, SUB, MUL, DIV, CMP

      //Load registers
      regDest = memory[registers[pc]+1] >> 4;
      regSrc = memory[registers[pc]+1] & 0xF;

      //Increase PC
      registers[pc] += 2;

      if ((opCode&0xF)==0) { //addition
        registers[regDest] = registers[regDest] + registers[regSrc];
      } else if ((opCode&0xF)==1) { //subtraction
        registers[regDest] = registers[regDest] - registers[regSrc];
      } else if ((opCode&0xF)==2) { //multiplication
        registers[regDest] = registers[regDest] * registers[regSrc];
      } else if ((opCode&0xF)==3) { //division
        registers[regDest] = (unsigned short int) registers[regDest] / registers[regSrc];
      } else if ((opCode&0xF)==4) { //Comparison

        temp = registers[regDest] - registers[regSrc];

        //Zero flag
        if (temp==0) //zero
          registers[psw] |= 1 << psw_z;
        else //Not zero
          registers[psw] &= ~(1 << psw_z);

        //Carry flag and overflow flag are the same (unsigned values)
        if (registers[regSrc]>registers[regDest] || (registers[regSrc]+registers[regDest]>temp))
          registers[psw] |= (1 << psw_o) | (1 << psw_c);
        else
          registers[psw] &= ~((1 << psw_o) | (1 << psw_c));

        //Negative flag
        if ((registers[regDest] - registers[regSrc])<0) //negative
          registers[psw] |= 1 << psw_n;
        else //positive
          registers[psw] &= ~(1 << psw_n);

      } else { //Error

        //Set up interrupt indicator
        entry_1 = true;

      }

    } else if ((opCode&0xF0)==0x80) { //NOT, AND, OR, XOR, TEST

      //Load registers
      regDest = memory[registers[pc]+1] >> 4;
      regSrc = memory[registers[pc]+1] & 0xF;

      //Increase PC
      registers[pc] += 2;

      if ((opCode&0xF)==0) { //not
        registers[regDest] = ~registers[regDest];
      } else if ((opCode&0xF)==1) { //and
        registers[regDest] = registers[regDest] & registers[regSrc];
      } else if ((opCode&0xF)==2) { //or
        registers[regDest] = registers[regDest] | registers[regSrc];
      } else if ((opCode&0xF)==3) { //xor
        registers[regDest] = registers[regDest] ^ registers[regSrc];
      } else if ((opCode&0xF)==4) { //test

        temp = registers[regDest] & registers[regSrc];

        //Zero flag
        if (temp==0) //zero
          registers[psw] |= 1 << psw_z;
        else //Not zero
          registers[psw] &= ~(1 << psw_z);

        //Negative flag
        if ((registers[regDest] - registers[regSrc])<0) //negative
          registers[psw] |= 1 << psw_n;
        else //positive
          registers[psw] &= ~(1 << psw_n);

      } else { //Error

        //Set up interrupt indicator
        entry_1 = true;

      }

    } else if ((opCode&0xF0)==0x90) { //SHL, SHR

      //Load registers
      regDest = memory[registers[pc]+1] >> 4;
      regSrc = memory[registers[pc]+1] & 0xF;

      //Increase PC
      registers[pc] += 2;

      if ((opCode&0xF)==0) { //shl

        //Carry flag
        if (registers[regDest] >> (16-registers[regSrc]>0 ? 16-registers[regSrc] : 0)) //set carry
          registers[psw] |= 1 << psw_c;
        else //unset carry
          registers[psw] &= ~(1 << psw_c);

        registers[regDest] = registers[regDest] << registers[regSrc];

      } else if ((opCode&0xF)==1) { //shr

        //Carry flag
        if (registers[regDest] << (16-registers[regSrc]>0 ? 16-registers[regSrc] : 0)) //set carry
          registers[psw] |= 1 << psw_c;
        else //unset carry
          registers[psw] &= ~(1 << psw_c);

        registers[regDest] = registers[regDest] >> registers[regSrc];

      } else { //Error

        //Set up interrupt indicator
        entry_1 = true;

      }

      if (!entry_1) { //Execute only if previous is corrects

        //Zero flag
        if (temp==0) //zero
        registers[psw] |= 1 << psw_z;
        else //Not zero
        registers[psw] &= ~(1 << psw_z);

        //Negative flag
        if (temp >> 15) //negative
        registers[psw] |= 1 << psw_n;
        else //positive
        registers[psw] &= ~(1 << psw_n);

      }

    } else if (opCode==0xA0) { //LDR

      //Read instruction
      regDest = memory[registers[pc]+1] >> 4;
      regSrc = memory[registers[pc]+1] & 0xf;
      upMode = memory[registers[pc]+2] >> 4;
      addrMode = memory[registers[pc]+2] & 0xf;

      //Load data if needed and increase PC
      if (addrMode==1 || addrMode==2) { //Without data
        registers[pc] += 3;
      } else { //With data
        data = memory[registers[pc]+3] << 8;
        data |= memory[registers[pc]+4];
        registers[pc] += 5;
      }

      //Load data
      if (loadData(regDest, regSrc, upMode, addrMode, data)<0) { //Error

        //Set up interrupt indicator
        entry_1 = true;

      }

    } else if (opCode==0xB0) { //STR

      //Read instruction
      regDest = memory[registers[pc]+1] >> 4;
      regSrc = memory[registers[pc]+1] & 0xf;
      upMode = memory[registers[pc]+2] >> 4;
      addrMode = memory[registers[pc]+2] & 0xf;

      //Load data if needed and increase PC
      if (addrMode==1 || addrMode==2) { //Without data
        registers[pc] += 3;
      } else { //With data
        data = memory[registers[pc]+3] << 8;
        data |= memory[registers[pc]+4];
        registers[pc] += 5;
      }

      //Store data
      if (storeData(regDest, regSrc, upMode, addrMode, data)<0) { //Error

        //Set up interrupt indicator
        entry_1 = true;

      }

    } else { //Error

      //Set up interrupt indicator
      entry_1 = true;

    }

    //Check in interrupts exist
    if (entry_0) { //Restart

      //Deactivate indicator
      entry_0 = false;

      //Go to interrupt
      jumpToInterrupt(0);

      //Mask all other interrupts
      registers[psw] |= 1 << psw_tr; // Mask timer interrupts
      registers[psw] |= 1 << psw_tl; // Mask terminal interrupts
      registers[psw] |= 1 << psw_i; // Mask all other interrupts

    } else if (entry_1) { //Incorrect instruction execution

      //Deactivate indicator
      entry_1 = false;

      //Go to interrupt
      jumpToInterrupt(1);

      //Mask all other interrupts
      registers[psw] |= 1 << psw_tr; // Mask timer interrupts
      registers[psw] |= 1 << psw_tl; // Mask terminal interrupts
      registers[psw] |= 1 << psw_i; // Mask all other interrupts

    } else if (entry_2 && !(registers[psw] & (1 << psw_tr))) { //Timer

      //Deactivate indicator
      entry_2 = false;

      //Go to interrupt
      jumpToInterrupt(2);

      //Mask all other interrupts
      registers[psw] |= 1 << psw_tr; // Mask timer interrupts
      registers[psw] |= 1 << psw_tl; // Mask terminal interrupts
      registers[psw] |= 1 << psw_i; // Mask all other interrupts

      // std::cout << "Entering timer interrupt routine!" << std::endl;
      // std::cout << "Timer ticks: " << timerValues[memory[tim_cfg+1]%8] << std::endl;

    } else if (entry_3 && !(registers[psw] & (1 << psw_tl))) { //Terminal

      //Deactivate indicator
      entry_3 = false;

      //Go to interrupt
      jumpToInterrupt(3);

      //Mask all other interrupts
      registers[psw] |= 1 << psw_tl; // Mask terminal interrupts
      registers[psw] |= 1 << psw_i; // Mask all other interrupts

    } else if (entry_4 && !(registers[psw] & (1 << psw_i))) { //Programmer defined entry 4

      //Deactivate indicator
      entry_4 = false;

      //Go to interrupt
      jumpToInterrupt(4);

      //Mask all other interrupts
      registers[psw] |= 1 << psw_i; // Mask all other interrupts

    } else if (entry_5 && !(registers[psw] & (1 << psw_i))) { //Programmer defined entry 5

      //Deactivate indicator
      entry_5 = false;

      //Go to interrupt
      jumpToInterrupt(5);

      //Mask all other interrupts
      registers[psw] |= 1 << psw_i; // Mask all other interrupts

    } else if (entry_6 && !(registers[psw] & (1 << psw_i))) { //Programmer defined entry 6

      //Deactivate indicator
      entry_6 = false;

      //Go to interrupt
      jumpToInterrupt(6);

      //Mask all other interrupts
      registers[psw] |= 1 << psw_i; // Mask all other interrupts

    } else if (entry_7 && !(registers[psw] & (1 << psw_i))) { //Programmer defined entry 7

      //Deactivate indicator
      entry_7 = false;

      //Go to interrupt
      jumpToInterrupt(7);

      //Mask all other interrupts
      registers[psw] |= 1 << psw_i; // Mask all other interrupts

    }

  }

  //Turn off timmer
  timerAlive = false;

  //Turn off terminal
  terminalAlive = false;

}

int emulate(std::string file) {

  prepareEmulation();

  if (loadFile(file)<0)
    return -1;

  // Initializing shared variables
  timerTicks = 0;
  timerAlive = true;
  terminalAlive = true;
  termOutModified = false;
  entry_0 = false;
  entry_1 = false;
  entry_2 = false;
  entry_3 = false;
  entry_4 = false;
  entry_5 = false;
  entry_6 = false;
  entry_7 = false;

  std::thread thread_timer(timer);
  std::thread thread_terminal(terminal);
  std::thread thread_cpu(cpu);

  thread_cpu.join();
  thread_timer.join();
  thread_terminal.join();

  return 0;
}
