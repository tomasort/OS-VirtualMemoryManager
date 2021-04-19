#include <iostream>
#include <exception>
#include <fstream>
#include <string>
#include <sstream>
#include <queue>
#include <vector>
#include <list>
#include <iomanip>

using namespace std;

#define MAX_VPAGES 64
#define MAX_FRAMES 128

class INV_ARG_EXCEPTION: public exception
{
    const char* what() const noexcept override{
        return "My exception happened";
    }
};

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
    unsigned frame_number:7;
    Frame(){
        frame_number = 0;
    }
};

class Pager{
public:
    // The select_victim_frame function returns a victim frame
    // Pure virtual function
    virtual Frame* select_victim_frame() = 0;
};

class PageTableEntry{
public:
    unsigned frame:7;
    unsigned present:1;
    unsigned referenced:1;
    unsigned modified:1;
    unsigned write_protected:1;
    unsigned pagedout:1;
    unsigned other:20;

    PageTableEntry(){
        present = referenced = modified = write_protected = pagedout = other =  0;
    }
};

class Process{
    // This class represents a process with a virtual address space
public:
    vector<VirtualMemoryArea*> virtual_memory_areas;  // vector of virtual memory areas for a specific process.
    // There might be holes between virtual memory areas.
    // page_table represents the translations from virtual pages to page frames.
    PageTableEntry page_table[MAX_VPAGES];  // stores the PageTableEntry objects
};

int* random_values = nullptr;
int number_of_random_values;
vector<Process*> processes;
vector<Instruction*> instructions;
Frame frame_table[MAX_FRAMES];  //Provide this reverse mapping (frame => <proc-id,vpage>) inside each frame’s frame table entry.

class FIFO : public Pager{
public:
    // Pure virtual function
    Frame* select_victim_frame() override{
        // This function selects a frame to UNMAP
        return nullptr;
    };
};
class Random : public Pager{
public:
    // TODO: implement random pager
    Frame* select_victim_frame() override{
        // This function selects a frame to UNMAP
        return nullptr;
    };
};
class Clock : public Pager{
public:
    // TODO: implement Clock pager
    Frame* select_victim_frame() override{
        // This function selects a frame to UNMAP
        return nullptr;
    };
};

class EnhancedSecondChance : public Pager{
public:
    // TODO: Implement EnhancedSecondChance pager
    Frame* select_victim_frame() override{
        // This function selects a frame to UNMAP
        return nullptr;
    };
};

class Aging : public Pager{
public:
    // TODO: Implement Aging pager
    Frame* select_victim_frame() override{
        // This function selects a frame to UNMAP
        return nullptr;
    };
};

class WorkingSet : public Pager{
public:
    // TODO: Implement WorkingSet pager
    Frame* select_victim_frame() override{
        // This function selects a frame to UNMAP
        return nullptr;
    };
};

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
        auto *current_instruction = new Instruction();
        instruction_line >> current_instruction->cmd >> current_instruction->arg;
        instructions.push_back(current_instruction);
    }
}

class Simulation{
public:
    int instruction_number = 0;
    Process *current_process;
    Instruction* current_instruction = nullptr;
    Pager *pager;
    Simulation(Pager *p){
        pager = p;
    }

    Frame *allocate_frame_from_free_list(){
        // This function finds a frame in the pool of free frames
        // TODO: implement me (allocate_frame_from_free_list)
        return nullptr;
    }

    Frame *get_frame(){
        // This functions return a pointer to a frame that we can either use or we can replace
        Frame *frame = allocate_frame_from_free_list();  // allocate frame from free list
        if (frame == nullptr){  // if we are out of free frames then call the pager
            frame = pager->select_victim_frame();
        }
        return frame;
    }

    void page_fault_handler(int virtual_page){
        // verify this is actually a valid page in a VMA if not raise an ERROR and go to the next instruction
        cout << "Starting the page fault handler" << endl;
        bool is_valid = false;
        for (VirtualMemoryArea *vma : current_process->virtual_memory_areas){
            if (vma->start_page < virtual_page && virtual_page < vma->end_page){
                is_valid = true;
            }
        }
        if (!is_valid) {
            throw INV_ARG_EXCEPTION();
        }
        // if the virtual_page is part of a VMA then allocate a frame and assign it to the PTE belonging to the virtual_page of this instruction.
        Frame *new_frame = get_frame();
        int old_frame = current_process->page_table[virtual_page].frame;
        cout << old_frame;
        // Populate the new frame
        // The population depends whether this page (the old frame) was previously paged out (in which case the page must be brought back
        // from the swap space (“IN”) or (“FIN” in case it is a memory mapped file). If the vpage was never swapped out and
        // is not file mapped, then by definition it still has a zero filled content and you issue the “ZERO” output.
        // current_process->page_table[virtual_page].frame = new_frame->frame_number;

        // you must inspect the state of the R and M bits. If the page was modified, then the page frame must be paged out
        // to the swap device (“OUT”) or in case it was file mapped written back to the file (“FOUT”)

    }

    bool get_next_instruction(Instruction **instruction){
        if (instruction_number >= instructions.size()) return false;
        *instruction = instructions[instruction_number++];
        return true;
    }

    void run_simulation(){
        cout << "Running simulation!" << endl;
        while(get_next_instruction(&current_instruction)){
            char current_command= current_instruction->cmd;
            int current_argument= current_instruction->arg;

            if (current_command == 'e'){  // We need to exit
                cout << "The command is e!";
                continue;
            }else if (current_command == 'c'){  // Do a context switch
                current_process = processes[current_argument];
                continue;
            }

            // Handle the instructions "r" and "w"
            PageTableEntry *pte = &(current_process->page_table[current_instruction->arg]);
            if (!pte->present){
                // The hardware would raise a page fault exception.
                try{
                    page_fault_handler(current_argument);
                } catch (INV_ARG_EXCEPTION &e){
                    cout << "SEGV" << endl;
                }
            }
            // check the write protection of pte
            // update the referenced and modified bits of pte
        }
    }
};



int main(int argc, char* argv[]){
    // TODO: use getopt to find the flags, input file and output file.
    // This is going to be replace by getopt at some point
    string input_file = "/Users/tomasortega/Desktop/os_lab3/lab3_assign/inputs/in3";
    string rfile = "/Users/tomasortega/Desktop/os_lab3/lab3_assign/inputs/rfile";
    Pager *pager = new FIFO;
    read_input_file(input_file);
    read_random_file(rfile);
    cout << sizeof(PageTableEntry) << endl;
    // Start the simulation
    Simulation s(pager);
    s.run_simulation();
    return 0;
}