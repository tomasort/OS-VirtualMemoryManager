#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <queue>
#include <vector>
#include <list>
#include <iomanip>

using namespace std;


class VirtualMemoryArea{
public:
    int start_page;
    int end_page;
    unsigned write_protected:1;  // indicates if the VMA is protected or not.
    unsigned file_mapped:1;  // indicates if the VMA is mapped to a file or not.

    VirtualMemoryArea(int start, int end, unsigned w_protected, unsigned f_mapped){
        start_page = start;
        end_page = end;
        write_protected = w_protected;
        file_mapped = f_mapped;
    }
};

class Process{
public:
    vector<VirtualMemoryArea*> virtual_memory_areas;
};

class Instruction{
    // This class represents a single instruction in this simulation
    //      c: context switch to another process. In this instruction the argument is the new process
    //      r: load virtual page. The virtual page number is the argument
    //      w: store virtual page. The virtual page number is the argument
    //      e: exit. The argument is the process that wants to exit
public:
    char cmd;
    int arg;
};

class Frame{
public:
};

class Paging{
public:
    // Pure virtual function
    virtual Frame* select_victim_frame() = 0;
};

class PageTableEntry{
public:
    unsigned page_frame:7;
    unsigned valid:1;
    unsigned referenced:1;
    unsigned modified:1;
    unsigned write_protected:1;
    unsigned pagedout:1;
    unsigned other:20;
};

int* random_values = nullptr;
int number_of_random_values;
vector<Process*> processes;
vector<Instruction*> instructions;

void read_random_file(const string& file_name){
    // Function that takes the name of a file with random numbers and stores them in a global variable called random_values
    ifstream file(file_name);
    file >> number_of_random_values;
    random_values = new int[number_of_random_values];
    for (int i = 0; i < number_of_random_values; i++){
        file >> random_values[i];
    }
}

int get_random(int size){
    // Function that returns a "random" value using the number in the file provided by the user
    static int offset = 0;
    if (offset >= number_of_random_values){
        offset = 0;
    }
    int random_number = 1 + random_values[offset++]%size;
    return random_number;
}

void read_input_file(const string& file_name){
    // Functions that reads the input file. It ignores every line starting with #
    ifstream file(file_name);
    string line;
    while(getline(file, line)){
        if (line.at(0) == '#') continue;
        break;
    }
    // Input specification: first line that is not a comment is the number of processes
    int number_of_processes = stoi(line);

    for (int i = 0; i < number_of_processes; i++){
        while(getline(file, line)){
            if (line.at(0) == '#') continue;
            break;
        }
        processes.push_back(new Process);  // create a new process and add it to the vector of processes
        int number_of_vmas = stoi(line);  // For each process there is a number specifying the number of Virtual Memory Areas

        for (int j = 0; j < number_of_vmas; j++){
            while(getline(file, line)){
                if (line.at(0) == '#') continue;  // Ignoring all the comments
                stringstream vma_specification;
                vma_specification << line;
                int start, end, w_protected, f_mapped;
                vma_specification >> start >> end >> w_protected >> f_mapped;
                processes[i]->virtual_memory_areas.push_back(new VirtualMemoryArea(start, end, w_protected, f_mapped));
                break;
            }
        }
    }
    // Now we can just read the instructions here!
    while(getline(file, line)){
        if (line.at(0) == '#') continue;  // Ignoring all the comments
        stringstream instruction_line;
        instruction_line << line;
        Instruction *current_instruction = new Instruction();
        instruction_line >> current_instruction->command >> current_instruction->argument;
        instructions.push_back(current_instruction);
    }
}

class Simulation{
public:
    void run_simulation(){
        cout << "Running simulation!" << endl;
    }
};



int main(int argc, char* argv[]){
    // TODO: user getopt to find the flags, input file and output file.
    string input_file = "/Users/tomasortega/Desktop/os_lab3/lab3_assign/inputs/in3";
    string rfile = "/Users/tomasortega/Desktop/os_lab3/lab3_assign/inputs/rfile";
    cout << sizeof(PageTableEntry) << endl;
    read_input_file(input_file);
    read_random_file(rfile);
    return 0;
}