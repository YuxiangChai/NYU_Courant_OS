#include "header.h"
#include <vector>
#include <fstream>
using namespace std;

// Randoms
// ===================================================================
vector<int> randoms;
int ofs = 0;

void read_randoms(string file_path) {
    ifstream f;
    f.open(file_path.c_str());
    int n;
    f >> n;
    int r;
    while (f >> r) {
        randoms.push_back(r);
    }
    f.close();
}

int myrandom(int inp) {
    if (ofs >= randoms.size()) {
        ofs = 0;
    }
    int rd = randoms[ofs] % inp;
    ofs++;
    return rd;
}

// PTE and Frame
// ===================================================================
PTE::PTE() {
    present = 0;
    referenced = 0;
    modified = 0;
    write_protect = 0;
    pagedout = 0;
    frame = 0;
    file_mapped = 0;
    remain = 0;
}

Frame::Frame(int i) {
    pid = -1;
    page_idx = -1;
    idx = i;
    age = 0;
    time_last_use = 0;
    free = true;
}


// VMA, Process, Instruction
// ===================================================================
VMA::VMA(int start, int end, int write, int f) {
    start_vpage = start;
    end_vpage = end;
    write_protected = write;
    file_mapped = f;
}

string VMA::str_vma() {
    string ret = to_string(start_vpage) + " " + to_string(end_vpage) + " " + to_string(write_protected) + " " + to_string(file_mapped);
    return ret;
}

Process::Process(int p) {
    pid = p;
    zero = 0;
    map = unmap = 0;
    fin = fout = 0;
    in = out = 0;
    segv = segprot = 0;
    for (int i=0; i<PTE_NUM; i++) {
        PTE pte;
        page_table.push_back(pte);
    }
}

void Process::add_vma(int start, int end, int write, int f) {
    VMA* v = new VMA(start, end, write, f);
    vmas.push_back(v);
}

string Process::str_proc() {
    string ret = "Pid = " + to_string(pid) + "\n";
    for (int i=0; i<vmas.size(); i++) {
        ret += vmas[i]->str_vma() + "\n";
    }
    return ret;
}

Process::~Process() {
    for (int i=0; i<vmas.size(); i++) {
        delete vmas[i];
    }
}


// Mem Management
// ===================================================================
int FIFO::select_victim_frame() {
    int idx;
    idx = ofs;
    ofs++;
    if (ofs >= frame_table.size()) ofs = 0;
    return idx;
}

int Random::select_victim_frame() {
    return myrandom(num_frames);
}

int Clock::select_victim_frame() {
    Process* proc = processes[frame_table[hand].pid];
    count = 0;
    int start = hand;
    while (proc->page_table[frame_table[hand].page_idx].referenced == 1) {
        count++;
        proc->page_table[frame_table[hand].page_idx].referenced = 0;
        hand++;
        if (hand >= frame_table.size()) hand = 0;
        proc = processes[frame_table[hand].pid];
    }
    int idx = hand;
    count++;
    hand++;
    if (hand >= frame_table.size()) hand = 0;
    if (aflag) cout << "ASELECT " << start << " " << count << endl;
    return idx;
}

int NRU::select_victim_frame() {
    vector<int> classes;
    int start = hand;
    int cls_ret = -1;
    int fr_ret = -1;
    count = 0;
    string st = "ASELECT: hand= ";
    bool reset;
    if (inst_num - last_reset_inst + 1 < TAU) reset = 0;
    else reset = 1;
    for (int i=0; i<4; i++) classes.push_back(-1);

    for (int i=0; i<frame_table.size(); i++) {
        int idx = (i + hand) % frame_table.size();
        Process* proc = processes[frame_table[idx].pid];
        PTE* pte = &(proc->page_table[frame_table[idx].page_idx]);
        int cls_idx = pte->referenced * 2 + pte->modified;
        if (cls_idx == 0) {
            if (classes[cls_idx] == -1) {
                cls_ret = cls_idx;
                fr_ret = idx;
                hand = (idx + 1) % frame_table.size();
                count++;
                break;
            }
        }
        else if (classes[cls_idx] == -1) classes[cls_idx] = idx;
        count++;
    }

    if (reset) {
        for (int i=0; i<frame_table.size(); i++) {
            if (frame_table[i].pid >= 0) processes[frame_table[i].pid]->page_table[frame_table[i].page_idx].referenced = 0;
        }
        last_reset_inst = inst_num + 1;
    }

    if (fr_ret == -1) {
        for (int i=0; i<4; i++) {
            if (classes[i] != -1) {
                cls_ret = i;
                fr_ret = classes[i];
                hand = (classes[i] + 1) % frame_table.size();
                break;
            }
        }
    }
    if (aflag) {
        st += to_string(start) + " " + to_string(reset) + " | " + to_string(cls_ret) + "  " + to_string(fr_ret) + "  " + to_string(count);
        cout << st << endl;
    }
    return fr_ret;
}

int Aging::select_victim_frame() {
    int ret = hand;
    int start = hand;
    if (aflag) cout << "ASELECT " + to_string(start) + "-" + to_string((start + frame_table.size()-1) % frame_table.size()) + " | ";
    for (int i=0; i<frame_table.size(); i++) {
        int idx = (i + hand) % frame_table.size();
        frame_table[idx].age = frame_table[idx].age >> 1;
        if (processes[frame_table[idx].pid]->page_table[frame_table[idx].page_idx].referenced) {
            frame_table[idx].age = (frame_table[idx].age | 0x80000000);
            processes[frame_table[idx].pid]->page_table[frame_table[idx].page_idx].referenced = 0;
        }
        if (frame_table[ret].age > frame_table[idx].age) ret = idx;
        if (aflag) printf("%d:%08x ", idx, frame_table[idx].age);
    }
    hand = (ret + 1) % frame_table.size();
    if (aflag) cout << "| " + to_string(ret) << endl;
    return ret;
}

void Aging::reset_age(Frame* fr) {
    fr->age = 0;
}

int WorkingSet::select_victim_frame() {
    int ret = -1;
    int old = -1;
    int start = hand;
    count = 0;
    if (aflag) cout << "ASELECT " << start << "-" << (start+frame_table.size()-1)%frame_table.size() << " | ";
    for (int i=0; i<frame_table.size(); i++) {
        int idx = (i + hand) % frame_table.size();
        PTE* pte = &(processes[frame_table[idx].pid]->page_table[frame_table[idx].page_idx]);
        if (aflag) cout << idx << "(" << pte->referenced << " " << frame_table[idx].pid << ":" << frame_table[idx].page_idx << " " << frame_table[idx].time_last_use << ") ";
        count++;
        if (pte->referenced) {
            frame_table[idx].time_last_use = inst_num;
            pte->referenced = 0;
        }
        else if (inst_num - frame_table[idx].time_last_use >= TAU) {
            ret = idx;
            if (aflag) cout << "STOP(" << count << ") ";
            break;
        }
        else if (old == -1 || old > frame_table[idx].time_last_use) {
            old = frame_table[idx].time_last_use;
            ret = idx;
        }
    }
    if (ret == -1) {
        ret = hand;
    }
    if (aflag) cout << "| " << ret << endl;
    hand = (ret + 1) % frame_table.size();
    return ret;
}

// helper functions
// ===================================================================
int get_frame(MM* mm) {
    int idx;
    if (!free_frame.empty()) {
        idx = free_frame.front();
        free_frame.pop_front();
    }
    else idx = mm->select_victim_frame();
    return idx;
}