#include <iostream>
#include <string>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <deque>
#include <unistd.h>
#include "header.h"
using namespace std;

bool oflag = false;
bool pflag = false;
bool fflag = false;
bool sflag = false;
bool aflag = false;
bool xflag = false;
bool yflag = false;
bool f_flag = false;
int num_proc = 0;
int num_frames = 0;
char algo;
vector<Process*> processes;
deque<int> free_frame;
vector<Frame> frame_table;
unsigned long long inst_num = 0;


int main(int argc, char **argv) {
    string st;
    int index;
    int c;
    string input_file;
    string rand_file;

    // parse command line arguments
    // ============================================================================
    while ((c=getopt(argc, argv, "f:a:o:")) != -1)
        switch (c) {
            case 'f':
                num_frames = atoi(optarg);
                break;
            case 'a':
                algo = *optarg;
                break;
            case 'o':
                st = optarg;
                if (st.find('O') != string::npos) oflag = true;
                if (st.find('P') != string::npos) pflag = true;
                if (st.find('F') != string::npos) fflag = true;
                if (st.find('S') != string::npos) sflag = true;
                if (st.find('a') != string::npos) aflag = true;
                if (st.find('x') != string::npos) xflag = true;
                if (st.find('y') != string::npos) yflag = true;
                if (st.find('f') != string::npos) f_flag = true;
                break;
            case '?':
                cout << "Unknown option" << endl;
                break;
            default:
                abort();
        }

    // initialize frame_table and free_frame
    // ============================================================================
    for (int i=0; i<num_frames; i++) {
        Frame* f = new Frame(i);
        frame_table.push_back(*f);
        free_frame.push_back(i);
        delete f;
    }

    // Read input file and random file and initialize
    // ============================================================================
    input_file = argv[optind];
    rand_file = argv[optind+1];

    read_randoms(rand_file);

    bool read_vma = false;
    int num_vma = 0;
    int start, end, write, fl;
    int pid = -1;
    ifstream f;
    f.open(input_file.c_str());
    string line;
    while (getline(f, line)) {
        if (line[0] == '#') continue;
        istringstream iss(line);
        if (num_proc == 0) {
            iss >> num_proc;
            continue;
        }
        if (read_vma) {
            if(num_vma != 0) {
                iss >> start >> end >> write >> fl;
                num_vma--;
                Process* p = processes.back();
                p->add_vma(start, end, write, fl);
                if (num_vma == 0) {
                    read_vma = false;
                    if (p->pid == num_proc-1) break;
                }
            }
        }
        else if (!read_vma && (num_proc != pid + 1)) {
            iss >> num_vma;
            read_vma = true;
            pid++;
            Process* p = new Process(pid);
            processes.push_back(p);
        }
    }

    MM* mmu;
    switch (algo)
    {
        case 'f':
        case 'F':
            mmu = new FIFO();
            break;
        case 'r':
        case 'R':
            mmu = new Random();
            break;
        case 'c':
        case 'C':
            mmu = new Clock();
            break;
        case 'e':
        case 'E':
            mmu = new NRU();
            break;
        case 'a':
        case 'A':
            mmu = new Aging();
            break;
        case 'w':
        case 'W':
            mmu = new WorkingSet();
        default:
            break;
    }
    // Simulation
    // ============================================================================
    unsigned long long cost = 0;
    unsigned long long context_switch = 0;
    unsigned long long num_exit = 0;
    Process* current_proc;

    while (getline(f, line)) {
        if (line[0] == '#') continue;
        istringstream iss(line);
        char inst;
        int target;
        iss >> inst >> target;

        if (oflag) cout << inst_num << ": ==> " << inst << " " << target << endl;

        switch (inst) {
            case 'c': {
                current_proc = processes[target];
                context_switch++;
                cost += 130;
                for (int i=0; i<current_proc->vmas.size(); i++) {
                    for (int j=current_proc->vmas[i]->start_vpage; j<=current_proc->vmas[i]->end_vpage; j++) {
                        current_proc->page_table[j].write_protect = current_proc->vmas[i]->write_protected;
                        current_proc->page_table[j].file_mapped = current_proc->vmas[i]->file_mapped;
                    }
                }
                inst_num++;
                break; }
            case 'e': {
                cout << "EXIT current process " << current_proc->pid << endl;
                cost += 1250;
                for (int i=0; i<current_proc->page_table.size(); i++) {
                    if (current_proc->page_table[i].present) {
                        PTE* pte = &(current_proc->page_table[i]);
                        Frame* fr = &(frame_table[pte->frame]);
                        fr->time_last_use = 0;
                        fr->free = 1;
                        fr->pid = -1;
                        free_frame.push_back(fr->idx);

                        cost += 400;

                        if (oflag) cout << " UNMAP " << current_proc->pid << ":" << i << endl;
                        if (pte->modified && pte->file_mapped) {
                            cost += 2400;
                            current_proc->fout++;
                            cout << " FOUT" << endl;
                        }
                        current_proc->unmap++;
                    }
                    current_proc->page_table[i].present = 0;
                    current_proc->page_table[i].file_mapped = 0;
                    current_proc->page_table[i].referenced = 0;
                    current_proc->page_table[i].pagedout = 0;
                }
                inst_num++;
                num_exit++;
                break; }
            case 'w':
            case 'r': {
                cost += 1;
                PTE* p = &(current_proc->page_table[target]);

                if (!p->present) {
                    bool segv_flag = true;
                    for (int i=0; i<current_proc->vmas.size(); i++) {
                        if (target <= current_proc->vmas[i]->end_vpage && target >= current_proc->vmas[i]->start_vpage) {
                            segv_flag = false;
                        }
                    }

                    if (segv_flag) {
                        cost += 340;
                        if (oflag) cout << " SEGV" << endl;
                        current_proc->segv++;
                        inst_num++;
                        continue;
                    }
                    
                    int fr_idx = get_frame(mmu);
                    Frame* fr = &(frame_table[fr_idx]);
                    if (!fr->free) {
                        // unmap old pte
                        PTE* old_p = &(processes[fr->pid]->page_table[fr->page_idx]);
                        old_p->present = 0;
                        if (oflag) cout << " UNMAP " << fr->pid << ":" << fr->page_idx << endl;
                        processes[fr->pid]->unmap++;
                        cost += 400;
                        if (old_p->modified) {
                            if (!old_p->file_mapped) {
                                old_p->pagedout = 1;
                                cost += 2700;
                                if (oflag) cout << " OUT" << endl;
                                processes[fr->pid]->out++;
                            }
                            else {
                                cost += 2400;
                                if (oflag) cout << " FOUT" << endl;
                                processes[fr->pid]->fout++;
                            }
                            old_p->modified = 0;
                            old_p->frame = 0;
                        }
                    }

                    if (p->file_mapped) {
                        cost += 2800;
                        if (oflag) cout << " FIN" << endl;
                        current_proc->fin++;
                    }
                    else if (p->pagedout) {
                        cost += 3100;
                        if (oflag) cout << " IN" << endl;
                        current_proc->in++;
                    }
                    else {
                        cost += 140;
                        if (oflag) cout << " ZERO" << endl;
                        current_proc->zero++;
                    }

                    // set the frame info
                    fr->pid = current_proc->pid;
                    fr->page_idx = target;
                    fr->time_last_use = inst_num;
                    fr->free = 0;
                    mmu->reset_age(fr);

                    p->frame = fr->idx;
                    p->present = 1;
                    cost += 300;
                    
                    if (oflag) cout << " MAP " << fr->idx << endl;
                    current_proc->map++;
                }
                p->referenced = 1;
                if (inst == 'w') {
                    if (!p->write_protect) {
                        p->modified = 1;
                    }
                    else {
                        cost += 420;
                        current_proc->segprot++;
                        if (oflag) cout << " SEGPROT" << endl;
                    }
                }
                inst_num++;

                if (xflag && (!yflag)) {
                    string st = "";
                    st += "PT[" + to_string(current_proc->pid) + "]: ";
                    for (int j=0; j<current_proc->page_table.size(); j++) {
                        if (current_proc->page_table[j].present) {
                            st += to_string(j) + ":";
                            if (current_proc->page_table[j].referenced) st += "R";
                            else st += "-";
                            if (current_proc->page_table[j].modified) st += "M";
                            else st += "-";
                            if (current_proc->page_table[j].pagedout) st += "S";
                            else st += "-";
                            st += " ";
                        }
                        else {
                            if (current_proc->page_table[j].pagedout) st += "# ";
                            else st += "* ";
                        }
                    }
                    st.pop_back();
                    cout << st << endl;
                }

                if (yflag) {
                    for (int i=0; i<processes.size(); i++) {
                        string st = "";
                        st += "PT[" + to_string(i) + "]: ";
                        for (int j=0; j<processes[i]->page_table.size(); j++) {
                            if (processes[i]->page_table[j].present) {
                                st += to_string(j) + ":";
                                if (processes[i]->page_table[j].referenced) st += "R";
                                else st += "-";
                                if (processes[i]->page_table[j].modified) st += "M";
                                else st += "-";
                                if (processes[i]->page_table[j].pagedout) st += "S";
                                else st += "-";
                                st += " ";
                            }
                            else {
                                if (processes[i]->page_table[j].pagedout) st += "# ";
                                else st += "* ";
                            }
                        }
                        st.pop_back();
                        cout << st << endl;
                    }
                }

                if (f_flag) {
                    string st = "FT: ";
                    for (int i=0; i<frame_table.size(); i++) {
                        if (frame_table[i].free) st += "* ";
                        else st += to_string(frame_table[i].pid) + ":" + to_string(frame_table[i].page_idx) + " ";
                    }
                    st.pop_back();
                    cout << st << endl;
                }

                break; }
            default:
                break;
        }
    }

    f.close();
    
    if (pflag) {
        for (int i=0; i<processes.size(); i++) {
            string st = "";
            st += "PT[" + to_string(i) + "]: ";
            for (int j=0; j<processes[i]->page_table.size(); j++) {
                if (processes[i]->page_table[j].present) {
                    st += to_string(j) + ":";
                    if (processes[i]->page_table[j].referenced) st += "R";
                    else st += "-";
                    if (processes[i]->page_table[j].modified) st += "M";
                    else st += "-";
                    if (processes[i]->page_table[j].pagedout) st += "S";
                    else st += "-";
                    st += " ";
                }
                else {
                    if (processes[i]->page_table[j].pagedout) st += "# ";
                    else st += "* ";
                }
            }
            st.pop_back();
            cout << st << endl;
        }
    }

    if (fflag) {
        string st = "FT: ";
        for (int i=0; i<frame_table.size(); i++) {
            if (frame_table[i].free) st += "* ";
            else st += to_string(frame_table[i].pid) + ":" + to_string(frame_table[i].page_idx) + " ";
        }
        st.pop_back();
        cout << st << endl;
    }

    if (sflag) {
        for (int i=0; i<processes.size(); i++) {
            Process* p = processes[i];
            printf("PROC[%d]: U=%lu M=%lu I=%lu O=%lu FI=%lu FO=%lu Z=%lu SV=%lu SP=%lu\n",
                    p->pid, p->unmap, p->map, p->in, p->out, p->fin, p->fout, p->zero, p->segv, p->segprot);
        }
        printf("TOTALCOST %lu %lu %lu %llu %lu\n", inst_num, context_switch, num_exit, cost, sizeof(PTE));
    }

    // Free
    for (int i=0; i<processes.size(); i++) {
        delete processes[i];
    }
    delete mmu;
    return 0;
}