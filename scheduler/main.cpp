#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <algorithm>
#include <string>
#include <string.h>
#include <iostream>
#include <fstream>
#include "header.h"
using namespace std;


int quantum = 100000;
int maxprio = 4;
char sc;                    // scheduler type
int vflag = 0;
int tflag = 0;
int eflag = 0;
int pflag = 0;

int main (int argc, char **argv)
{
    char* st = NULL;
    int index;
    int c;
    string input_file;
    string rand_file;

    // parse command line arguments
    // ============================================================================
    while ((c=getopt(argc, argv, "vteps:")) != -1)
        switch (c) {
            case 'v':
                vflag = 1;
                break;
            case 't':
                tflag = 1;
                break;
            case 'e':
                eflag = 1;
                break;
            case 'p':
                pflag = 1;
                break;
            case 's':
                st = optarg;
                sc = st[0];
                if (strlen(st) > 1) {sscanf(st, "%c%d:%d", &sc, &quantum, &maxprio);}
                break;
            case '?':
                cout << "Unknown option" << endl;
                break;
            default:
                abort();
        }
    
    input_file = argv[optind];
    rand_file = argv[optind+1];

    // Initialize 
    // ============================================================================
    read_randoms(rand_file);
    
    DES des;
    vector<Process*> processes;
    ifstream f;
    f.open(input_file.c_str());
    int at, tc, cb, io;
    int pid = 0;
    // initialize DES layer
    while (f >> at >> tc >> cb >> io) {
        Process* p = new Process(pid, at, tc, cb, io);
        processes.push_back(p);
        Event* evt = new Event(p->state_ts, p, TRANS_TO_READY);
        des.add_event(evt);
        pid++;
    }
    f.close();

    bool preprio = false;               // is it preprio?
    Scheduler* scheduler;
    // choose scheduler
    switch (sc) {
        case 'F':
            scheduler = new FCFS;
            break;
        case 'L':
            scheduler = new LCFS;
            break;
        case 'S':
            scheduler = new SRTF;
            break;
        case 'R':
            scheduler = new RR;
            break;
        case 'P':
            scheduler = new PRIO(maxprio);
            break;
        case 'E':
            scheduler = new PREPRIO(maxprio);
            preprio = true;
            break;
    }

    // Simulation 
    // ============================================================================
    int cpu_use_time = 0;               // count cpu running time
    vector<pair<int, int>> io_use;      // store all io using time periods
    int cpu_burst;                      // cpu burst
    int io_burst;                       // io burst
    bool call_scheduler = false;        // flag to show if need to call scheduler
    int last_ts;                        // last timestamp
    int prio_higher = 0;                // is prio higher?
    bool preprio_preempt = false;       // should preempt?
    Process* current_running_proc = nullptr;
    Event* evt;
    string add_evt;
    while ((evt = des.get_event())) {
        Process* proc = evt->proc;
        int current_time = evt->event_time;
        Trans transition = evt->transition;
        int time_in_prev_state = current_time - proc->state_ts;
        last_ts = current_time;
        switch(transition) {
            case TRANS_TO_READY: {
                if (vflag) {
                    cout << evt->str_event() << endl;
                }
                if (preprio) {
                    preprio_preempt = false;
                    if (current_running_proc != nullptr) {
                        int cur_proc_next_evt_ts = des.get_proc_next_event_time(current_running_proc);
                        prio_higher = proc->dynamic_prio > current_running_proc->dynamic_prio ? 1 : 0;
                        if (prio_higher && (cur_proc_next_evt_ts != current_time)) {
                            preprio_preempt = true;
                        }
                        if (pflag) {
                            printf("---> PRIO preemption %1d by %1d ? %1d TS=%1d now=%1d) --> ",
                                    current_running_proc->pid, proc->pid, prio_higher, cur_proc_next_evt_ts, current_time);
                            if (preprio_preempt) printf("YES\n");
                            else printf("NO\n");
                        }
                        if (preprio_preempt) {
                            des.rm_event(current_running_proc);
                            Event* e = new Event(current_time, current_running_proc, TRANS_TO_PREEMPT);
                            current_running_proc->remain_time += cur_proc_next_evt_ts - current_time;
                            current_running_proc->cpu_burst += cur_proc_next_evt_ts - current_time;
                            cpu_use_time -= cur_proc_next_evt_ts - current_time;
                            add_evt = des.add_event(e);
                            if (eflag) {
                                cout << add_evt << endl;
                            }
                            current_running_proc = nullptr;
                        }
                    }
                }
                scheduler->add_process(proc);
                proc->set_state_and_ts(READY, current_time);
                call_scheduler = true;
                if (tflag) {
                    cout << scheduler->str_sche() << endl;
                }
                break;
            }
            case TRANS_TO_RUN: {
                if (proc->cpu_burst > 0) {
                    cpu_burst = proc->cpu_burst;
                }
                else {
                    proc->set_cpuburst();
                    cpu_burst = proc->cpu_burst;
                }
                if (vflag) {
                    cout << evt->str_event() << endl;
                }
                current_running_proc = proc;
                proc->cw += current_time - proc->state_ts;
                proc->set_state_and_ts(RUNNING, current_time);

                if (cpu_burst > quantum) {
                    cpu_use_time += quantum;
                    proc->remain_time -= quantum;
                    proc->cpu_burst -= quantum;
                    Event* e = new Event(current_time+quantum, proc, TRANS_TO_PREEMPT);
                    add_evt = des.add_event(e);
                    if (eflag) {
                        cout << add_evt << endl;
                    }
                }
                else {
                    cpu_use_time += cpu_burst;
                    proc->remain_time -= cpu_burst;
                    proc->cpu_burst = 0;
                    Event* e = new Event(current_time+cpu_burst, proc, TRANS_TO_BLOCK);
                    add_evt = des.add_event(e);
                    if (eflag) {
                        cout << add_evt << endl;
                    }
                }
                break;
            }
            case TRANS_TO_BLOCK: {
                if (proc->remain_time == 0) {
                    if (vflag) {
                        cout << evt->str_event() << endl;
                    }
                    current_running_proc = nullptr;
                    proc->set_state_and_ts(DONE, current_time);
                    call_scheduler = true;
                }
                else {
                    proc->set_ioburst();
                    io_burst = proc->io_burst;
                    io_use.push_back({current_time, current_time+io_burst});
                    if (vflag) {
                        cout << evt->str_event() << endl;
                    }
                    current_running_proc = nullptr;
                    proc->it += io_burst;
                    proc->set_state_and_ts(BLOCK, current_time);
                    proc->dynamic_prio = proc->prio - 1;
                    Event* e = new Event(current_time+io_burst, proc, TRANS_TO_READY);
                    add_evt = des.add_event(e);
                    if (eflag) {
                        cout << add_evt << endl;
                    }
                    call_scheduler = true;
                }
                if (tflag) {
                    cout << scheduler->str_sche() << endl;
                }
                break;
            }
            case TRANS_TO_PREEMPT: {
                if (vflag) {
                    cout << evt->str_event() << endl;
                }
                current_running_proc = nullptr;
                scheduler->add_process(proc);
                proc->set_state_and_ts(READY, current_time);
                call_scheduler = true;
                break;
            }
        }
        delete evt;
        if (call_scheduler) {
            if (des.get_next_event_time() == current_time) continue;
            call_scheduler = false;
            if (current_running_proc == nullptr) {
                current_running_proc = scheduler->get_next_process();
                if (current_running_proc == nullptr) continue;
                Event* e = new Event(current_time, current_running_proc, TRANS_TO_RUN);
                current_running_proc = nullptr;
                add_evt = des.add_event(e);
                if (eflag) {
                    cout << add_evt << endl;
                }
            }
        }
    }

    // Print out final results
    // ============================================================================
    switch (sc) {
        case 'F':
            cout << "FCFS" << endl;
            break;
        case 'L':
            cout << "LCFS" << endl;
            break;
        case 'S':
            cout << "SRTF" << endl;
            break;
        case 'R':
            cout << "RR " << quantum << endl;
            break;
        case 'P':
            cout << "PRIO " << quantum << endl;
            break;
        case 'E':
            cout << "PREPRIO " << quantum << endl;
            break;
    }
    int io_use_time = 0;
    int total_tt = 0;
    int total_cw = 0;
    for (int i=0; i<processes.size(); i++) {
        Process* p = processes[i];
        printf("%04d: %4d %4d %4d %4d %1d | %5d %5d %5d %5d\n", p->pid, p->at, p->tc, p->cb, p->io, p->prio, p->state_ts, p->state_ts-p->at, p->it, p->cw);
        total_tt += p->state_ts - p->at;
        total_cw += p->cw;
    }

    // Calculate io use time
    // ============================================================================
    sort(io_use.begin(), io_use.end(), compare_io);
    int start = io_use[0].first;
    int end = io_use[0].second;
    io_use_time += end - start;
    for (int i=1; i<io_use.size(); i++) {
        if (end < io_use[i].first) {
            io_use_time += io_use[i].second - io_use[i].first;
            end = io_use[i].second;
        }
        else if (end < io_use[i].second) {
            io_use_time += io_use[i].second - end;
            end = io_use[i].second;
        }
    }

    // Print out the summary
    // ============================================================================
    printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n", last_ts, double(cpu_use_time) / double(last_ts) * 100, double(io_use_time) / double(last_ts) * 100, 
            double(total_tt) / double(processes.size()), double(total_cw) / double(processes.size()), double(processes.size()) / double(last_ts) * 100);
    delete scheduler;
    return 0;
}