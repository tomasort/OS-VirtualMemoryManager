// name: Tomas Ortega
// OS class lab 3 memory
// the cost of each operation is:
// maps=300, unmaps=400, ins=3100, outs=2700, fins=2800, fouts=2400, zeros=140, segv=340, segprot=420

#include <iostream>
#include <exception>
#include <fstream>
#include <sstream>
#include <string>
#include <queue>
#include <vector>
#include <list>
#include <getopt.h>

using namespace std;

#define MAX_VPAGES 64
#define SPACING 0
#define MAX_FRAMES 128
bool O_option;
bool P_option;
bool F_option;
bool S_option;
bool x_option;
bool y_option;
bool f_option;
bool a_option;

#define O_trace(fmt...) do { if (O_option) { printf(fmt); printf("\n"); fflush(stdout); } } while(0)
#define P_trace(fmt...) do { if (P_option) { printf(fmt); printf("\n"); fflush(stdout); } } while(0)
#define F_trace(fmt...) do { if (F_option) { printf(fmt); printf("\n"); fflush(stdout); } } while(0)
#define S_trace(fmt...) do { if (S_option) { printf(fmt); printf("\n"); fflush(stdout); } } while(0)
#define x_trace(fmt...) do { if (x_option) { printf(fmt); printf("\n"); fflush(stdout); } } while(0)
#define y_trace(fmt...) do { if (y_option) { printf(fmt); printf("\n"); fflush(stdout); } } while(0)
#define f_trace(fmt...) do { if (f_option) { printf(fmt); printf("\n"); fflush(stdout); } } while(0)
#define a_trace(fmt...) do { if (a_option) { printf(fmt); printf("\n"); fflush(stdout); } } while(0)


class INVALID_VPAGE_EXCEPTION: public exception{
    const char* what() const noexcept override{
        return "Invalid vpage!";
    }
};

class WRITE_PROTECTED_EXCEPTION: public exception{
    const char* what() const noexcept override{
        return "Invalid vpage!";
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
    int process_id;
    int vpage;
    int frame_id;
    int initialized;
    unsigned int age;

    Frame(){
        initialized = 0;
        age = 0;
    }
    string print(){
        if (initialized){
            string s = to_string(process_id) + ":" + to_string(vpage);
            return s;
        }
        return "*";
    }
};


class Pager{
public:
    static int hand;
    // The select_victim_frame function returns a victim frame
    // Pure virtual function
    virtual Frame* select_victim_frame() = 0;
    virtual void update_age(Frame *frame){ ; };
};
int Pager::hand = 0;

class PageTableEntry{
public:
    unsigned frame:7;
    unsigned present:1;
    unsigned referenced:1;
    unsigned modified:1;
    unsigned write_protected:1;
    unsigned pagedout:1;
    unsigned file_mapped:1;
    unsigned is_valid_vma:1;
    unsigned other:17;

    PageTableEntry(){
        frame = file_mapped = is_valid_vma = present = referenced = modified = write_protected = pagedout = other =  0;

    }
    string print(){
        // # for not valid entries that have been swapped out
        // * for not valid entries that don't have a swap are associated with them.
        if (pagedout && !present && !file_mapped){
            return "#";
        }
        if (!is_valid_vma || !present){
            return "*";
        }
        string s = "";
        if (referenced){
            s = s + "R";
        }else{
            s = s + "-";
        }
        if (modified){
            s = s + "M";
        }else{
            s = s + "-";
        }
        if (pagedout){
            s = s + "S";
        }else{
            s = s + "-";
        }
        return  s;
    }
};

class Process{
    // This class represents a process with a virtual address space
public:
    static int process_counter;
    int process_id;
    unsigned long maps=0;
    unsigned long unmaps=0;
    unsigned long  ins=0;
    unsigned long outs=0;
    unsigned long fins=0;
    unsigned long fouts=0;
    unsigned long zeros=0;
    unsigned long segv=0;
    unsigned long segprot=0;
    // There might be holes between virtual memory areas.
    vector<VirtualMemoryArea*> virtual_memory_areas;  // vector of virtual memory areas for a specific process.
    // page_table represents the translations from virtual pages to page frames.
    PageTableEntry page_table[MAX_VPAGES];  // stores the PageTableEntry objects
    Process(){
        process_id = process_counter;
        process_counter++;
    }
    string print_page_table(){
        string s = "PT[" + to_string(process_id) + "]: ";
        for (int i = 0; i < MAX_VPAGES; i++){
            PageTableEntry p = page_table[i];
            string pte_string = p.print();
            if (pte_string == "*" || pte_string == "#"){
                s = s + p.print();
            }else{
                s = s + to_string(i) + ":" + p.print();
            }
            if(SPACING){
                if (i < MAX_VPAGES-1){
                    s = s + " ";
                }
            }
            else{
                s = s + " ";
            }
        }
        return s;
    }
};

int Process::process_counter = 0;
int* random_values = nullptr;
unsigned long number_of_random_values;
int number_of_frames;
vector<Process*> processes;
vector<Instruction*> instructions;
unsigned long instruction_number = 0;
Frame frame_table[128];  //Provide this reverse mapping (frame => <proc-id,vpage>) inside each frame’s frame table entry.

string print_frame_table(){
    string s = "FT: ";
    for (int i = 0; i < number_of_frames; i++){
        s = s + frame_table[i].print();
        if(SPACING){
            if (i < number_of_frames-1){
                s = s + " ";
            }
        }else{
            s = s + " ";
        }
    }
    return s;
}

class FIFO : public Pager{
public:
    int hand = 0;
    Frame* select_victim_frame() override{
        // This function selects a frame to UNMAP
        return &frame_table[(hand++)%number_of_frames];
    };
};

int get_random(){
    // Function that returns a "random" value using the number in the file provided by the user
    static int offset = 0;
    if (offset >= number_of_random_values){
        offset = 0;
    }
    int random_number = random_values[offset++]%number_of_frames;
    return random_number;
}

class Random : public Pager{
public:
    Frame* select_victim_frame() override{
        // This function selects a frame to UNMAP
        return &frame_table[get_random()];
    };
};

class Clock : public Pager{
public:
    Frame* select_victim_frame() override{
        // This function selects a frame to UNMAP
        // to so we go through the page table and find one with the reference bit set to 0
        int i = hand;
        while (true){
            if (processes[frame_table[i%number_of_frames].process_id]->page_table[frame_table[i%number_of_frames].vpage].referenced==1){
                processes[frame_table[i%number_of_frames].process_id]->page_table[frame_table[i%number_of_frames].vpage].referenced = 0;
                i++;
            }else{
                a_trace("ASELECT %d %d", hand, i-hand+1);
                hand = i%number_of_frames + 1;
                return &frame_table[i%number_of_frames];
            }
        }
    };
};

class EnhancedSecondChance : public Pager{
public:
    static unsigned int _previous_instruction;
    Frame* select_victim_frame() override{
        // This function selects a frame to UNMAP
        bool classes[4] = {0, 0, 0, 0};
        Frame *frame_options[4];
        Frame *selected_frame = nullptr;
        int hand_options[4];
        int reset = 0;
        int i = hand;
        int old_hand = hand;
        while (true){
            int ref = processes[frame_table[i%number_of_frames].process_id]->page_table[frame_table[i%number_of_frames].vpage].referenced;
            int mod = processes[frame_table[i%number_of_frames].process_id]->page_table[frame_table[i%number_of_frames].vpage].modified;
            int _class = 2*ref + mod;
            if(!classes[_class]){
                classes[_class] = true;
                hand_options[_class] = (i+1)%number_of_frames;
                frame_options[_class] = &frame_table[i%number_of_frames];
            }
            if (instruction_number-_previous_instruction >= 50){
                reset = 1;
                // we need to reset every referenced bit for every valid page
                processes[frame_table[i%number_of_frames].process_id]->page_table[frame_table[i%number_of_frames].vpage].referenced = 0;
            }else if (classes[0]){
                break;
            }
            if (i-old_hand == number_of_frames-1){
                break;
            }
            i++;
        }
        if (reset){
            _previous_instruction = instruction_number;
        }
        for(int j = 0; j < 4; j++){
            if(classes[j]){
                selected_frame = frame_options[j];
                hand = hand_options[j];
                a_trace("ASELECT: hand=%2d %d | %d %2d %2d", old_hand, reset, j, selected_frame->frame_id, i-old_hand+1);
                break;
            }
        }
        return selected_frame;
    };
};
unsigned int EnhancedSecondChance::_previous_instruction = 0;

class Aging : public Pager{
public:
    void update_age(Frame *frame) override{
        frame->age = 0;
    };
    Frame* select_victim_frame() override{
        // This function selects a frame to UNMAP
        int i = hand;
        int old_hand = hand;
        bool found = false;
        Frame *selected_frame;
        unsigned int min_age = 0;
        string s = "";
        char hex_string[32];
        while (true){
            unsigned int age = frame_table[i%number_of_frames].age;
            age = age >> 1;
            if(processes[frame_table[i%number_of_frames].process_id]->page_table[frame_table[i%number_of_frames].vpage].referenced){
                age = (age | 0x80000000);
            }
            frame_table[i%number_of_frames].age = age;
            if(i == old_hand){
                min_age = frame_table[i%number_of_frames].age;
                hand = (hand+1)%number_of_frames;
                selected_frame = &frame_table[i%number_of_frames];
            }
            processes[frame_table[i%number_of_frames].process_id]->page_table[frame_table[i%number_of_frames].vpage].referenced = 0;
            sprintf(hex_string, "%x", age);
            s += to_string(i%number_of_frames) + ":" + string(hex_string) + " ";
            if (age < min_age || (age == min_age && !found)){
                hand = (i+1)%number_of_frames;
                selected_frame = &frame_table[i%number_of_frames];
                min_age = age;
                found = true;
            }
            if (i-old_hand == number_of_frames-1){
                break;
            }
            i++;
        }
        s = "ASELECT " + to_string(old_hand) + "-" + to_string(i%number_of_frames) + " | " + s;
        s += "| " + to_string(selected_frame->frame_id);
        a_trace("%s", s.c_str());
        return selected_frame;
    };
};

class WorkingSet : public Pager{
public:
    void update_age(Frame *frame) override{
        frame->age = instruction_number-1;
    };
    // TODO: Implement WorkingSet pager
    static const int tau = 49;
    Frame* select_victim_frame() override{
        // This function selects a frame to UNMAP
        int i = hand;
        int old_hand = hand;
        bool found = false;
        Frame *selected_frame;
        unsigned int min_age = 0;
        string s = "";
        char _string[32];
        bool scanning = true;
        if(instruction_number == 3584){
            ;
        }
        while (true){
            Frame *current_frame = &frame_table[i%number_of_frames];
            int referenced = processes[current_frame->process_id]->page_table[current_frame->vpage].referenced;
            int previous_age = current_frame->age;
            unsigned int age = (instruction_number-1)-current_frame->age;
            if(referenced && !found){
                // set the time of last use to current time
                current_frame->age = instruction_number-1;
                processes[frame_table[i%number_of_frames].process_id]->page_table[frame_table[i%number_of_frames].vpage].referenced = 0;
            }
//            age = instruction_number-frame_table[i%number_of_frames].age;
            if (!referenced && (age > tau) && !found){
                selected_frame = current_frame;
                hand = (i+1)%number_of_frames;  // hand of the next page after the selected one (in this case the first one)
                found = true;
            }else{
                if(i == old_hand && !found){
                    // initialize the variable to store the minimum
                    selected_frame = current_frame;
                    min_age = current_frame->age;
                    hand = (i+1)%number_of_frames;  // hand of the next page after the selected one (in this case the first one)
                }
                if (current_frame->age < min_age && !found){
                    // update the minimum if we find it
                    hand = (i+1)%number_of_frames;
                    selected_frame = current_frame;
                    min_age = current_frame->age;
                }
            }
            if(scanning){
                sprintf(_string, "%d(%d %d:%d %d) ", current_frame->frame_id, referenced, current_frame->process_id, current_frame->vpage, previous_age);
                s += string(_string);
            }
            if (found && scanning){
                sprintf(_string, "STOP(%d) ", i-old_hand+1);
                s += string(_string);
                scanning = false;
            }
            if (i-old_hand == number_of_frames-1){
                // We have visited every single page.
                break;
            }
            i++;
        }
        s = "ASELECT " + to_string(old_hand) + "-" + to_string(i%number_of_frames) + " | " + s;
        s += "| " + to_string(selected_frame->frame_id);
        a_trace("%s", s.c_str());
        return selected_frame;
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
    Process *current_process;
    Instruction* current_instruction = nullptr;
    queue<Frame*> free_list;
    Pager *pager;

    // stats
    unsigned long process_exits = 0;
    unsigned long long total_cost = 0;
    unsigned long ctx_switches = 0;

    Simulation(Pager *p){
        pager = p;
        for (int i = 0; i < number_of_frames; i++){
            frame_table[i] = Frame();
            frame_table[i].frame_id = i;
        }
        for (int i = 0; i < number_of_frames; i++){
            free_list.push(&frame_table[i]);
        }
    }

    Frame *allocate_frame_from_free_list(){
        // This function finds a frame in the pool of free frames
        if (free_list.empty()){
            return nullptr;
        }
        Frame* free_frame = free_list.front();
        free_list.pop();
        return free_frame;
    }

    Frame *get_frame(){
        // This functions return a pointer to a frame that we can either use or we can replace
        Frame *frame = allocate_frame_from_free_list();  // allocate frame from free list
        if (frame == nullptr){  // if we are out of free frames then call the pager
            frame = pager->select_victim_frame();
        }
        return frame;
    }

    void return_frames(){
        for(PageTableEntry &pte : current_process->page_table) {
            if(pte.present){
                Frame* frame = &frame_table[pte.frame];
                processes[frame->process_id]->unmaps += 1;
                total_cost += 400;
                O_trace(" UNMAP %s", frame->print().c_str());
                frame->initialized = 0;
                free_list.push(frame);
                if (pte.file_mapped && pte.modified) {
                    processes[frame->process_id]->fouts += 1;
                    total_cost += 2400;
                    O_trace(" FOUT");
                }
            }
            pte.present = 0;
            pte.pagedout = 0;
        }
    }

    void page_fault_handler(int virtual_page){
        // The frame table can only be accessed as part of the “simulated page fault handler”
        // verify this is actually a valid page in a VMA if not raise an ERROR and go to the next instruction
        PageTableEntry *pte = &(current_process->page_table[virtual_page]);
        if (!pte->is_valid_vma){
            for (VirtualMemoryArea *vma : current_process->virtual_memory_areas){
                if (vma->start_page <= virtual_page && virtual_page <= vma->end_page){
                    pte->is_valid_vma = 1;
                    // At this point you can store bits in the PageTableEntry based on what you found in the VMA
                    // and what bits are not occupied by the mandatory bits
                    pte->file_mapped = vma->file_mapped;
                    pte->write_protected = vma->write_protected;
                }
            }
            if (!pte->is_valid_vma) {
                throw INVALID_VPAGE_EXCEPTION();
            }
        }
        // if the virtual_page is part of a VMA then allocate a frame and assign it to the PTE belonging to the virtual_page of this instruction.
        Frame *new_frame = get_frame();
        pte->frame = new_frame->frame_id;
        if (new_frame->initialized){
            processes[new_frame->process_id]->unmaps += 1;
            total_cost += 400;
            O_trace(" UNMAP %s", new_frame->print().c_str());
            int old_vpage = new_frame->vpage;
            PageTableEntry *old_pte = &(processes[new_frame->process_id]->page_table[old_vpage]);
            old_pte->present = 0;
            if (old_pte->modified){
                if (old_pte->file_mapped){
                    processes[new_frame->process_id]->fouts += 1;
                    total_cost += 2400;
                    O_trace(" FOUT");
                }else{
                    old_pte->pagedout = 1;
                    processes[new_frame->process_id]->outs += 1;
                    total_cost += 2700;
                    O_trace(" OUT");
                }
            }
        }
        if (pte->pagedout || pte->file_mapped){
            if (pte->file_mapped){
                current_process->fins += 1;
                total_cost += 2800;
                O_trace(" FIN");
            }else{
                current_process->ins += 1;
                total_cost += 3100;
                O_trace(" IN");
            }
        }else if (!new_frame->initialized || (!pte->pagedout && !pte->file_mapped)){
            current_process->zeros += 1;
            total_cost += 140;
            O_trace(" ZERO");
        }
        // Populate new_frame
        new_frame->process_id = current_process->process_id;
        new_frame->vpage = virtual_page;
        new_frame->initialized = 1;
        pte->present = 1;
        pte->modified = 0;
        // if paged out (the page must be brought back from the swap space (“IN”) or (“FIN” in case it is a memory mapped file).
        // If the vpage was never swapped out and is not file mapped,
        // then by definition it still has a zero filled content and you issue the “ZERO” output.
        // The population depends whether the vpage (in the frame) was previously paged out
        // current_process->page_table[virtual_page].frame = new_frame->frame_number;

        // you must inspect the state of the R and M bits. If the page was modified, then the page frame must be paged out
        // to the swap device (“OUT”) or in case it was file mapped written back to the file (“FOUT”)
        current_process->maps += 1;
        total_cost += 300;
        pager->update_age(new_frame);
        O_trace(" MAP %d", new_frame->frame_id);
    }

    bool get_next_instruction(Instruction **instruction){
        if (instruction_number >= instructions.size()) return false;
        *instruction = instructions[instruction_number++];
        return true;
    }

    void run_simulation(){
        while(get_next_instruction(&current_instruction)){
            char current_command= current_instruction->cmd;
            int current_argument= current_instruction->arg;
//            if (instruction_number == 43){
//                ;
//            }
            O_trace("%lu: ==> %c %d", instruction_number - 1, current_instruction->cmd, current_instruction->arg);
            if (current_command == 'e'){  // We need to exit
                O_trace("EXIT current process %d", current_process->process_id);
                return_frames();
                process_exits++;
                total_cost += 1250;
                continue;
            }else if (current_command == 'c'){  // Do a context switch
                total_cost += 130;
                ctx_switches++;
                current_process = processes[current_argument];
                continue;
            }

            // Handle the instructions "r" and "w"
            total_cost++;
            PageTableEntry *pte = &(current_process->page_table[current_instruction->arg]);
            try{
                // The hardware would raise a page fault exception.
                if (!pte->present){
                    page_fault_handler(current_argument);
                }
                // check the write protection of pte
                // update the referenced and modified bits of pte
                if (current_command == 'w' || current_command == 'r') pte->referenced = 1;
                if (current_command == 'w' && pte->write_protected) throw WRITE_PROTECTED_EXCEPTION();
                if (current_command == 'w') pte->modified = 1;
                x_trace("%s", current_process->print_page_table().c_str());
                f_trace("%s", print_frame_table().c_str());
            } catch (INVALID_VPAGE_EXCEPTION &e){
                total_cost += 340;
                current_process->segv += 1;
                O_trace("%s", " SEGV");
                continue;
            } catch (WRITE_PROTECTED_EXCEPTION &e){
                total_cost += 420;  // 420 fuck yeah!! smoke that shit baby
                current_process->segprot += 1;
                O_trace("%s", " SEGPROT");
                x_trace("%s", current_process->print_page_table().c_str());
                f_trace("%s", print_frame_table().c_str());
                continue;
            }
        }
        if (P_option){
            for (Process *proc : processes){
                P_trace("%s", proc->print_page_table().c_str());
            }
        }
        F_trace("%s", print_frame_table().c_str());
        if (S_option){
            for (Process *proc : processes){
                    printf("PROC[%d]: U=%lu M=%lu I=%lu O=%lu FI=%lu FO=%lu Z=%lu SV=%lu SP=%lu\n",
                           proc->process_id, proc->unmaps, proc->maps, proc->ins, proc->outs,
                           proc->fins, proc->fouts, proc->zeros, proc->segv, proc->segprot);
            }
            printf("TOTALCOST %lu %lu %lu %llu %lu\n", instruction_number, ctx_switches, process_exits, total_cost, sizeof(PageTableEntry));
        }
    }
};



int main(int argc, char* argv[]){
    string usage = "./mmu –f<num_frames> -a<algo> [-o<options>] inputfile randomfile";
    int c;
    char alg;
    Pager *p;
    while ((c = getopt(argc, argv, "F:f:A:a:O:o:")) != -1)  {
        switch(c){
            case 'f':
            case 'F':
                number_of_frames = stoi(optarg);
                if (MAX_FRAMES < number_of_frames){
                    exit(0); // even though this is never going to be tested so why bother with this bullshit
                }
                break;
            case 'a':
            case 'A':
                alg = optarg[0];
                switch(alg){
                    case 'f':
                    case 'F':
                        p = new FIFO();
                        break;
                    case 'r':
                    case 'R':
                        p = new Random();
                        break;
                    case 'c':
                    case 'C':
                        p = new Clock();
                        break;
                    case 'e':
                    case 'E':
                        p = new EnhancedSecondChance();
                        break;
                    case 'a':
                    case 'A':
                        p = new Aging();
                        break;
                    case 'w':
                    case 'W':
                        p = new WorkingSet();
                        break;
                }
                break;
            case 'O':
            case 'o':
                string option_string(optarg);
                for (char const &option: option_string) {
                    if (option == 'O') O_option = 1;
                    if (option == 'P') P_option = 1;
                    if (option == 'F') F_option = 1;
                    if (option == 'S') S_option = 1;
                    if (option == 'x') x_option = 1;
                    if (option == 'y') y_option = 1;
                    if (option == 'f') f_option = 1;
                    if (option == 'a') a_option = 1;
                }
                break;
        }
    }
    string input_file = argv[optind];
    string random_file = argv[optind+1];
    read_input_file(input_file);
    read_random_file(random_file);
    // Start the simulation
    Simulation s(p);
    s.run_simulation();
    return 0;
}