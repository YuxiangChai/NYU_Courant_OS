#include <string>
#include <vector>
#include <iostream>
#include <deque>
using namespace std;
#define PTE_NUM 64
#define TAU 50

// Randoms
// =======================================================
extern int ofs;
extern vector<int> randoms;
void read_randoms(string file_path);
int myrandom(int inp);


// PTE and Frame
// =======================================================
class PTE {
    public:
        unsigned int present:1;
        unsigned int referenced:1;
        unsigned int modified:1;
        unsigned int write_protect:1;
        unsigned int pagedout:1;
        unsigned int frame:7;
        unsigned int file_mapped:1;
        unsigned int remain:19;

        PTE();
};

class Frame {
    public:
        int pid;
        int page_idx;
        int idx;
        unsigned int age;
        int time_last_use;
        bool free;

        Frame(int i);
};


// VMA, Process, Instuction
// =======================================================
class VMA {
    public:
        int start_vpage;
        int end_vpage;
        int write_protected;
        int file_mapped;

        VMA(int start, int end, int write, int f);
        string str_vma();
};

class Process {
    public:
        int pid;
        unsigned long long map;
        unsigned long long unmap;
        unsigned long long fout;
        unsigned long long fin;
        unsigned long long zero;
        unsigned long long in;
        unsigned long long out;
        unsigned long long segv;
        unsigned long long segprot;

        vector<VMA*> vmas;
        vector<PTE> page_table;

        Process(int p);
        void add_vma(int start, int end, int write, int f);
        string str_proc();
        ~Process();
};

// Global Variables
// =======================================================
extern bool oflag;
extern bool fflag;
extern bool pflag;
extern bool sflag;
extern bool aflag;
extern int num_proc;
extern int num_frames;
extern char algo;
extern unsigned long long inst_num;
extern vector<Process*> processes;
extern deque<int> free_frame;
extern vector<Frame> frame_table;

// Mem Management
// =======================================================
class MM {
    public:
        int hand;
        int count;
        virtual int select_victim_frame() {return 0;};
        virtual void reset_age(Frame* fr) {};
};

class FIFO: public MM {
    public:
        FIFO() {hand = 0;};
        int select_victim_frame();
        void reset_age(Frame* fr) {};
};

class Random: public MM {
    public:
        Random() {hand = 0;};
        int select_victim_frame();
        void reset_age(Frame* fr) {};
};

class Clock: public MM {
    public:
        Clock() {hand = 0;};
        int select_victim_frame();
        void reset_age(Frame* fr) {};
};

class NRU: public MM {
    public:
        unsigned long long last_reset_inst;
        NRU() {hand = last_reset_inst = 0;};
        int select_victim_frame();
        void reset_age(Frame* fr) {};
};

class Aging: public MM {
    public:
        Aging() {hand = 0;};
        int select_victim_frame();
        void reset_age(Frame* fr);
};

class WorkingSet: public MM {
    public:
        WorkingSet() {hand = 0;};
        int select_victim_frame();
        void reset_age(Frame* fr) {};
};


// helper functions
// =======================================================
int get_frame(MM* mm);
