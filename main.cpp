#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

// file data
fstream inFile;
fstream outFile;

// Initial data
int cc = 0;

int registers[10] = {0, 9, 5, 7, 1, 2, 3, 4, 5, 6};
int inRegisters[10] = {0, 9, 5, 7, 1, 2, 3, 4, 5, 6};

int memory[5] = {5, 9, 4, 8, 7};
int inMemory[5] = {5, 9, 4, 8, 7};
string memName[5] = {"00", "04", "08", "0C", "10"};

int PC = 0;
string instruction = "00000000000000000000000000000000";

int idEx[6] = {};
string idExControl = "000000000";
string idExName[6] = {"ReadData1", "ReadData2", "sign_ext", "Rs", "Rt", "Rd"};

int exMem[3] = {};
string exMemControl = "00000";
string exMemName[3] = {"ALUout", "WriteData", "Rt/Rd"};

int memWb[3] = {};
string memWbControl = "00";
string memWbName[3] = {"ReadData", "ALUout", "Rt/Rd"};

string taskName[3] = {};

bool stall = false;
bool Rtype = false;

// function
int IF(int i);
int ID();
int EX();
int MEM();
int WB();

int biToDec(string bi);
void readFile();
void output();

// temp data
vector<string> inCode;
int checkEND = 0;


int main()
{
    // open read & write file
    inFile.open("General.txt", ios::in);
    outFile.open("genResult.txt", ios::out);
    readFile();

    inFile.open("Datahazard.txt", ios::in);
    outFile.open("dataResult.txt", ios::out);
    readFile();

    inFile.open("Lwhazard.txt", ios::in);
    outFile.open("loadResult.txt", ios::out);
    readFile();

    inFile.open("Branchhazard.txt", ios::in);
    outFile.open("branchResult.txt", ios::out);
    readFile();

    return 0;
}

int WB()
{
    // dataHazard
    if(idEx[4]==memWb[2] && idEx[4]!=0 && Rtype)
    {
        if(memWbControl=="10")  idEx[1] = memWb[1];
        if(memWbControl=="11")  idEx[1] = memWb[0];
    }
    if(idEx[3]==memWb[2] && idEx[3]!=0)
    {
        if(memWbControl=="10")  idEx[0] = memWb[1];
        if(memWbControl=="11")  idEx[0] = memWb[0];
    }

    // ReadData to Rt/Rd
    if(memWbControl == "11")
    {
        registers[memWb[2]] = (memWb[2]!=0)?memWb[0]:0;
    }
    // ALUout to Rt/Rd
    if(memWbControl == "10")
    {
        registers[memWb[2]] = (memWb[2]!=0)?memWb[1]:0;
    }
    return 0;
}

int MEM()
{
    if(stall) stall = false;

    if(exMemControl == "00000")
    {
        taskName[2] = "";
        for(int i=0;i<3;i++)    memWb[i] = 0;
        memWbControl = "00";
        return 0;
    }

    // dataHazard
    if(idEx[4]==exMem[2] && idEx[4]!=0 && Rtype)
    {
        if(exMemControl.substr(3, 2)=="10")  idEx[1] = exMem[0];
    }
    if(idEx[3]==exMem[2] && idEx[3]!=0)
    {
        if(exMemControl.substr(3, 2)=="10")  idEx[0] = exMem[0];
    }

    taskName[2] = taskName[1];
    // MEM:sw
    if(taskName[2] == "sw")  memory[exMem[0]/4] = exMem[1];
    // ReadData
    if(taskName[2] == "lw") memWb[0] = memory[exMem[0]/4];
    else    memWb[0] = 0;
    // ALUout
    memWb[1] = exMem[0];
    // Rt/Rd
    memWb[2] = exMem[2];
    memWbControl = exMemControl.substr(3, 2);

    return 1;
}

int EX()
{
    if(idExControl == "000000000")
    {
        taskName[1] = "";
        for(int i=0;i<3;i++)    exMem[i] = 0;
        exMemControl = "00000";
        return 0;
    }

    taskName[1] = taskName[0];
    // ALUout
    if(taskName[1] == "add")    exMem[0] = idEx[0] + idEx[1];
    if(taskName[1] == "sub")    exMem[0] = idEx[0] - idEx[1];
    if(taskName[1] == "and")    exMem[0] = idEx[0] & idEx[1];
    if(taskName[1] == "or")     exMem[0] = idEx[0] | idEx[1];
    if(taskName[1] == "slt")    exMem[0] = (idEx[0] < idEx[1])?1:0;
    if(taskName[1] == "lw")     exMem[0] = idEx[0] + idEx[2];
    if(taskName[1] == "sw")     exMem[0] = idEx[0] + idEx[2];
    if(taskName[1] == "addi")   exMem[0] = idEx[0] + idEx[2];
    if(taskName[1] == "andi")   exMem[0] = idEx[0] & idEx[2];
    if(taskName[1] == "beq")
    {
        exMem[0] = idEx[0] - idEx[1];
        if(idEx[0] == idEx[1])
        {
            PC += idEx[2]-1;
            instruction = "00000000000000000000000000000000";
        }
    }
    // WriteData
    exMem[1] = idEx[1];
    // Rt/Rd
    if(idExControl[0] == '1')   exMem[2] = idEx[5];
    if(idExControl[0] == '0')   exMem[2] = idEx[4];
    // Control signals
    exMemControl = idExControl.substr(4, 5);

    return 1;
}

int ID()
{
    string ins = instruction;
    if(ins == "00000000000000000000000000000000")
    {
        for(int i=0;i<6;i++)    idEx[i] = 0;
        idExControl = "000000000";
        taskName[0] = "";
        Rtype = false;
        return 0;
    }

    string op = ins.substr(0, 6);

    idEx[2] = biToDec(ins.substr(16, 16));  // im
    idEx[3] = biToDec(ins.substr(6, 5));    // rs
    idEx[4] = biToDec(ins.substr(11, 5));   // rt
    idEx[0] = registers[idEx[3]];           // *rs
    idEx[1] = registers[idEx[4]];           // *rt

    if(op == "000000")
    {
        // R-type
        idEx[5] = biToDec(ins.substr(16, 5));   // rd

        string funct = ins.substr(26, 6);
        if(funct == "100000")   taskName[0] = "add";
        if(funct == "100010")   taskName[0] = "sub";
        if(funct == "100100")   taskName[0] = "and";
        if(funct == "100101")   taskName[0] = "or";
        if(funct == "101010")   taskName[0] = "slt";

        idExControl = "110000010";
        Rtype = true;

        // Lwhazard
        if(exMemControl.substr(3, 2)=="11")
        {
            idExControl = "000000000";
            stall = true;
        }
    }
    else
    {
        idEx[5] = 0;                            // rd
        if(op == "100011")
        {
            taskName[0] = "lw";
            idExControl = "000101011";

        }
        if(op == "101011")
        {
            taskName[0] = "sw";
            idExControl = "000100100";
        }
        if(op == "001000")
        {
            taskName[0] = "addi";
            idExControl = "000100010";
        }
        if(op == "001100")
        {
            taskName[0] = "andi";
            idExControl = "011100010";
        }
        if(op == "000100")
        {
            taskName[0] = "beq";
            idExControl = "001010000";
        }

        Rtype = false;
    }

    // Lwhazard
    if(exMemControl.substr(3, 2)=="11")
    {
        idExControl = "000000000";
        stall = true;
    }

    return 1;
}

int IF(int i)
{
    if(stall)
    {
        PC--;
        return 1;
    }

    // fetch
    if(i < inCode.size())
    {
        instruction = inCode[i];
        return 1;
    }
    else
    {
        instruction = "00000000000000000000000000000000";
        return 0;
    }
}

int biToDec(string bi)
{
    // 二進位轉十進位
    bool neg = false;
    if(bi[0]=='1')
    {
        neg = true;
        for(int i=0;i<bi.size();i++) bi[i] = bi[i]=='1'?'0':'1';
    }
    int t = 0;
    int pow = 1;
    for(int i=bi.size()-1;i>=0;i--)
    {
        t += (bi[i]-'0')*pow;
        pow *= 2;
    }
    if(neg) t = -(t+1);

    return t;
}

void readFile()
{
    // initial
    string tstr = "";
    inCode.clear();
    for(int i=0;i<10;i++) registers[i] = inRegisters[i];
    for(int i=0;i<5;i++) memory[i] = inMemory[i];
    cc = 0;
    stall = false;
    Rtype = false;

    // read instruction
    while(inFile >> tstr)
    {
        inCode.push_back(tstr);
    }

    // execute
    for(PC=0;;PC++)
    {
        checkEND = 0;
        checkEND += WB();
        checkEND += MEM();
        checkEND += EX();
        checkEND += ID();
        checkEND += IF(PC);
        output();
        if(checkEND == 0)
        {
            // end program
            break;
        }
    }

    inFile.close();
    outFile.close();
}

void output()
{
    outFile << "CC " << ++cc << ":\n";

    outFile << "\nRegisters:\n";
    for(int i=0;i<10;i++)
    {
        outFile << "$" << i << ": " << registers[i] << "\n";
    }

    outFile << "\nData memory:\n";
    for(int i=0;i<5;i++)
    {
        outFile << "0x" << memName[i] << ": " << memory[i] << "\n";
    }

    outFile << "\nIF/ID :\n";
    outFile << setw(16) << left << "PC" << PC*4+4 << "\n";
    outFile << setw(16) << left << "Instruction" << instruction << "\n";

    outFile << "\nID/EX :\n";
    for(int i=0;i<6;i++)
    {
        outFile << setw(16) << left << idExName[i] << idEx[i] << "\n";
    }
    outFile << setw(16) << left << "Control signals" << idExControl << "\n";

    outFile << "\nEX/MEM :\n";
    for(int i=0;i<3;i++)
    {
        outFile << setw(16) << left << exMemName[i] << exMem[i] << "\n";
    }
    outFile << setw(16) << left << "Control signals" << exMemControl << "\n";

    outFile << "\nMEM/WB :\n";
    for(int i=0;i<3;i++)
    {
        outFile << setw(16) << left << memWbName[i] << memWb[i] << "\n";
    }
    outFile << setw(16) << left << "Control signals" << memWbControl << "\n";

    for(int i=0;i<65;i++)
    {
        outFile << "=";
    }
    outFile << "\n";
}
