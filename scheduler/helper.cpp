#include <string>
#include <vector>
#include <deque>
#include <fstream>
#include <iostream>
#include "header.h"
using namespace std;


// Utils
// ===============================================================
const char* state_str[] = {"CREATED", "READY", "RUNNG", "BLOCK", "DONE"};       // change State to string, for verbose use
const char* trans_str[] = {"READY", "RUNNG", "BLOCK", "PREEMPT"};               // change Trans to string, for verbose use
const char* event_str[] = {"READY", "RUNNG", "BLOCK", "READY"};                 // change Event_State to string, for verbose use

bool compare_io(pair<int, int> a, pair<int, int> b) {
    return (a.first < b.first);
}

// Randoms
// ===============================================================
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

int myrandom(int burst) {
    if (ofs >= randoms.size()) {
        ofs = 0;
    }
    int rd = 1 + (randoms[ofs] % burst);
    ofs++;
    return rd;
}

// Process
// ===============================================================
Process::Process(int PID, int AT, int TC, int CB, int IO) {
    pid = PID;
    at = AT;
    tc = TC;
    cb = CB;
    io = IO;

    prio = myrandom(maxprio);
    dynamic_prio = prio-1;
    remain_time = tc;
    state = CREATED;
    state_ts = at;
    cpu_burst = 0;
    io_burst = 0;
    it = 0;
    cw = 0;
}

void Process::set_cpuburst() {
    cpu_burst = min(myrandom(cb), remain_time);     // need to compare with the remaining time
}

void Process::set_ioburst() {
    io_burst = myrandom(io);
}

void Process::set_dynamic_prio() {
    if (dynamic_prio == 0) {
        dynamic_prio = prio - 1;
    }
    else {
        dynamic_prio--;
    }
}

void Process::set_state_and_ts(State s, int t) {
    state = s;
    state_ts = t;
}

// Event and DES
// ===============================================================
Event::Event(int evt_t, Process* p, Trans trans) {
    event_time = evt_t;
    proc = p;
    transition = trans;
}

string Event::str_event() {                 // for verbose use
    string ret = "";
    ret += to_string(event_time) + " " + to_string(proc->pid) + " " + to_string(event_time-proc->state_ts);
    if (proc->remain_time == 0) {
        ret += ": Done";
        return ret;
    }
    ret = ret + ": " + state_str[proc->state] + " -> " + event_str[transition];
    if (event_str[transition] == "RUNNG") {
        ret += " cb=" + to_string(proc->cpu_burst) + " rem=" + to_string(proc->remain_time) + " prio=" + to_string(proc->dynamic_prio);
    }
    else if (event_str[transition] == "BLOCK") {
        ret += "  ib=" + to_string(proc->io_burst) + " rem=" + to_string(proc->remain_time);
    }
    else if (event_str[transition] == "READY" && proc->state == RUNNING) {      // Preempt, due to prof's program, different output string with one more space in the front
        ret += "  cb=" + to_string(proc->cpu_burst) + " rem=" + to_string(proc->remain_time) + " prio=" + to_string(proc->dynamic_prio);
    }
    return ret;
}

Event* DES::get_event() {
    if (event_queue.empty()) {
            return nullptr;
        }
        Event* evt = event_queue[0];
        event_queue.erase(event_queue.begin());
        return evt;
}

string DES::add_event(Event* evt) {
    string evtQ_before = str_des();
    int i;
    // find the position to insert
    for (i = 0; i < event_queue.size(); i++) {
        if (event_queue[i]->event_time > evt->event_time) break;
    }
    event_queue.insert(event_queue.begin()+i, evt);

    string st = "";
    st += "  AddEvent(" + to_string(evt->event_time) + ":" + to_string(evt->proc->pid) + ":" + trans_str[evt->transition] + "):" + evtQ_before + " ==> " + str_des();
    return st;
}

void DES::rm_event(Process* p) {
    int po;
    // find the next event position given a process, and remove it
    for (int i = 0; i < event_queue.size(); i++) {
        if (event_queue[i]->proc->pid == p->pid) {
            po = i;
            break;
        }
    }
    int ts = event_queue[po]->event_time;
    event_queue.erase(event_queue.begin()+po);

    if (eflag) {
        cout << "RemoveEvent(" << p->pid << "):  " << ts << ":" << p->pid << " ==> " << str_des() << endl;
    }
}

string DES::str_des() {         // for verbose use
    string st = "";
    for (int i = 0; i < event_queue.size(); i++) {
        Event* evt = event_queue[i];
        st += "  " + to_string(evt->event_time) + ":" + to_string(evt->proc->pid) + ":" + trans_str[evt->transition];
    }
    return st;
}

int DES::get_next_event_time() {
    if (event_queue.empty()) {
        return -1;
    }
    return event_queue.front()->event_time;
}

int DES::get_proc_next_event_time(Process* p) {
    for (int i = 0; i < event_queue.size(); i++) {
        if (event_queue[i]->proc->pid == p->pid) {
            return event_queue[i]->event_time;
        }
    }
    return -1;
}

// FIFO
// ===============================================================
Process* FCFS::get_next_process() {
    if (run_queue.empty()) {
        return nullptr;
    }
    Process* p;
    p = run_queue.front();
    run_queue.pop_front();
    return p;
}

void FCFS::add_process(Process* p) {
    run_queue.push_back(p);
}

string FCFS::str_sche() {
    string ret = "SCHED (";
    ret += to_string(run_queue.size()) + "): ";
    if (run_queue.size() > 0) {
        for (auto it=run_queue.begin(); it!=run_queue.end(); it++) {
            ret += " " + to_string((*it)->pid) + ":" + to_string((*it)->state_ts);
        }
    }
    return ret;
}

// LIFO
// ===============================================================
Process* LCFS::get_next_process() {
    if (run_queue.empty()) {
        return nullptr;
    }
    Process* p;
    p = run_queue.back();
    run_queue.pop_back();
    return p;
}

void LCFS::add_process(Process* p) {
    run_queue.push_back(p);
}

string LCFS::str_sche() {
    string ret = "SCHED (";
    ret += to_string(run_queue.size()) + "): ";
    if (run_queue.size() > 0) {
        for (auto it=run_queue.begin(); it!=run_queue.end(); it++) {
            ret += " " + to_string((*it)->pid) + ":" + to_string((*it)->state_ts);
        }
    }
    return ret;
}

// SRTF
// ===============================================================

// used to generate the priority queue
bool CompareSRTF::operator()(Process* a, Process* b) {
    if (a->remain_time > b->remain_time) return true;
    else if (a->remain_time < b->remain_time) return false;
    else {
        if (a->state_ts > b->state_ts) return true;
        else if (a->state_ts < b->state_ts) return false;
        else {
            if (a->pid > b->pid) return true;
            else return false;
        }
    }
}

Process* SRTF::get_next_process() {
    if (run_queue.empty()) {
        return nullptr;
    }
    Process* p;
    p = run_queue.top();
    run_queue.pop();
    return p;
}

void SRTF::add_process(Process* p) {
    run_queue.push(p);
}

string SRTF::str_sche() {       // because priority queue is not necessary to print, only show size.
    string ret = "SCHED (";
    ret += to_string(run_queue.size()) + "): ";
    return ret;
}

// Round-Robin
// ===============================================================
Process* RR::get_next_process() {
    if (run_queue.empty()) {
        return nullptr;
    }
    Process* p;
    p = run_queue.front();
    run_queue.pop_front();
    return p;
}

void RR::add_process(Process* p) {
    run_queue.push_back(p);
}

string RR::str_sche() {             // for verbose use
    string ret = "SCHED (";
    ret += to_string(run_queue.size()) + "): ";
    if (run_queue.size() > 0) {
        for (auto it=run_queue.begin(); it!=run_queue.end(); it++) {
            ret += " " + to_string((*it)->pid) + ":" + to_string((*it)->state_ts);
        }
    }
    return ret;
}

// PRIO
// ===============================================================
PRIO::PRIO(int maxprio) {       // initialize
    for (int i=0; i<maxprio; i++) {
        deque<Process*>* temp1 = new deque<Process*>;
        active_queue.push_back(temp1);
        deque<Process*>* temp2 = new deque<Process*>;
        expired_queue.push_back(temp2);
    }
    mp = maxprio-1;
}

bool PRIO::all_empty() {        // to check is the active queue is empty
    for (int i=0; i<=mp; i++) {
        if (!active_queue[i]->empty()) return false;
    }
    return true;
}

void PRIO::swap() {             // to swap the activate queue and expired queue
    for (int i=0; i<=mp; i++) {
        deque<Process*>* temp;
        temp = active_queue[i];
        active_queue[i] = expired_queue[i];
        expired_queue[i] = temp;
    }
}

Process* PRIO::get_next_process() {
    if (all_empty()) {
        swap();
        if (tflag) {
            cout << "switched queues" << endl;
        }
    }
    for (int pr=mp; pr>=0; pr--) {
        if (active_queue[pr]->empty()) continue;
        Process* p;
        p = active_queue[pr]->front();
        active_queue[pr]->pop_front();
        return p;
    }
    return nullptr;
}

void PRIO::add_process(Process* p) {
    if (p->state == RUNNING) {      // preempt
        if (p->dynamic_prio == 0) {
            p->set_dynamic_prio();
            expired_queue[p->dynamic_prio]->push_back(p);
        }
        else {
            p->set_dynamic_prio();
            active_queue[p->dynamic_prio]->push_back(p);
        }
    }
    else {
        active_queue[p->dynamic_prio]->push_back(p);
    }
    if (tflag) {
        cout << str_sche() << endl;
    }
    if (all_empty()) {
        swap();
        if (tflag) {
            cout << "switched queues" << endl;
        }
    }
}

string PRIO::str_sche() {       // for verbose use
    string act = "{ ";
    string exp = "{ ";
    string ret = "";
    for (int i=mp; i>=0; i--) {
        act += "[";
        if (!active_queue[i]->empty()) {
            for (auto it=active_queue[i]->begin(); it!=active_queue[i]->end(); it++) {
                act += to_string((*it)->pid) + ",";
            }
            act.pop_back();
        }
        act += "]";

        exp += "[";
        if (!expired_queue[i]->empty()) {
            for (auto it=expired_queue[i]->begin(); it!=expired_queue[i]->end(); it++) {
                exp += to_string((*it)->pid) + ",";
            }
            exp.pop_back();
        }
        exp += "]";
    }
    act += "}";
    exp += "}";
    ret += act + " : " + exp + " : ";
    return ret;
}

PRIO::~PRIO() {         // to free the active and expired queues
    for (int i=0; i<maxprio; i++) {
        delete active_queue[i];
        delete expired_queue[i];
    }
}

// PREPRIO
// ===============================================================
PREPRIO::PREPRIO(int maxprio) {     // initialize
    for (int i=0; i<maxprio; i++) {
        deque<Process*>* temp1 = new deque<Process*>;
        active_queue.push_back(temp1);
        deque<Process*>* temp2 = new deque<Process*>;
        expired_queue.push_back(temp2);
    }
    mp = maxprio-1;
}

bool PREPRIO::all_empty() {         // check if all activate queues are empty
    for (int i=0; i<=mp; i++) {
        if (!active_queue[i]->empty()) return false;
    }
    return true;
}

void PREPRIO::swap() {              // swap the active and expired queues
    for (int i=0; i<=mp; i++) {
        deque<Process*>* temp;
        temp = active_queue[i];
        active_queue[i] = expired_queue[i];
        expired_queue[i] = temp;
    }
}

Process* PREPRIO::get_next_process() {
    if (all_empty()) {
        swap();
        if (tflag) {
            cout << "switched queues" << endl;
        }
    }
    for (int pr=mp; pr>=0; pr--) {
        if (active_queue[pr]->empty()) continue;
        Process* p;
        p = active_queue[pr]->front();
        active_queue[pr]->pop_front();
        return p;
    }
    return nullptr;
}

void PREPRIO::add_process(Process* p) {
    if (p->state == RUNNING) {          // preempt
        if (p->dynamic_prio == 0) {
            p->set_dynamic_prio();
            expired_queue[p->dynamic_prio]->push_back(p);
        }
        else {
            p->set_dynamic_prio();
            active_queue[p->dynamic_prio]->push_back(p);
        }
    }
    else {
        active_queue[p->dynamic_prio]->push_back(p);
    }
    if (tflag) {
        cout << str_sche() << endl;
    }
    if (all_empty()) {
        swap();
        if (tflag) {
            cout << "switched queues" << endl;
        }
    }
}

string PREPRIO::str_sche() {        // for verbose use
    string act = "{ ";
    string exp = "{ ";
    string ret = "";
    for (int i=mp; i>=0; i--) {
        act += "[";
        if (!active_queue[i]->empty()) {
            for (auto it=active_queue[i]->begin(); it!=active_queue[i]->end(); it++) {
                act += to_string((*it)->pid) + ",";
            }
            act.pop_back();
        }
        act += "]";

        exp += "[";
        if (!expired_queue[i]->empty()) {
            for (auto it=expired_queue[i]->begin(); it!=expired_queue[i]->end(); it++) {
                exp += to_string((*it)->pid) + ",";
            }
            exp.pop_back();
        }
        exp += "]";
    }
    act += "}";
    exp += "}";
    ret += act + " : " + exp + " : ";
    return ret;
}

PREPRIO::~PREPRIO() {       // free all active and expired queues
    for (int i=0; i<maxprio; i++) {
        delete active_queue[i];
        delete expired_queue[i];
    }
}