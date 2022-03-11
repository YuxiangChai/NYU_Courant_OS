#include <string>
#include <deque>
#include <queue>
#include <vector>
using namespace std;


enum Trans {TRANS_TO_READY, TRANS_TO_RUN, TRANS_TO_BLOCK, TRANS_TO_PREEMPT};        // enumerate Trans types
enum State {CREATED, READY, RUNNING, BLOCK, DONE};                                  // enumerate State types

// Process class
// ================================================================================
class Process {
    public:
        int pid;            // process id
        int at;             // arrival time
        int tc;             // total cpu time
        int cb;             // initial cpu burst
        int io;             // initial io burst

        int prio;           // static prio
        int ft;             // finish time
        int tt;             // turnaround time
        int it;             // IO time
        int cw;             // cpu wait time
        int remain_time;    // process remaining time
        int cpu_burst;      // cpu burst time for each event
        int io_burst;       // io burst for each event
        int dynamic_prio;   // dynamic prio
        int state_ts;       // timestamp of entering the current state
        State state;        // current state

        Process(int PID, int AT, int TC, int CB, int IO);
        void set_cpuburst();                        // generate cpu burst for the event
        void set_ioburst();                         // generate io burst for the event
        void set_dynamic_prio();                    // generate new dynamic prio
        void set_state_and_ts(State s, int t);      // set the process to a new state and timestamp
};

// Event and DES layer class
// ================================================================================
class Event {
    public:
        int event_time;                             // event happen timestamp
        Process* proc;                              // pointer to process
        Trans transition;                           // which transition
        Event(int evt_t, Process* p, Trans trans);  // constructor
        string str_event();                         // change to string to be printed
};

class DES {
    public:
        vector<Event*> event_queue;                 // event queue, use vector for convinience
        DES() {};                                   // constructor
        Event* get_event();                         // return a pointer to the next event
        string add_event(Event* evt);               // add a pointer to an event to the queue and return a string of "add an event"
        void rm_event(Process* p);                  // remove future events for Process p
        string str_des();                           // return a string of the current DES layer to be printed
        int get_next_event_time();                  // return the next event time
        int get_proc_next_event_time(Process* p);   // given a process, return the next event time of that process, if none, return -1
};

// Schedulers
// ================================================================================
class Scheduler {
    public:
        virtual void add_process(Process* p) {};                    // virtual function for add_process
        virtual Process* get_next_process() { return nullptr; };    // virtual function for get_next_process
        virtual string str_sche() {return "";};                     // virtual function for str_sche
};

class FCFS: public Scheduler {
    public:
        deque<Process*> run_queue;
        void add_process(Process* p);
        Process* get_next_process();
        string str_sche();
};

class LCFS: public Scheduler {
    public:
        deque<Process*> run_queue;
        void add_process(Process* p);
        Process* get_next_process();
        string str_sche();
};

class CompareSRTF {
    public:
        bool operator()(Process* a, Process* b);
};

class SRTF: public Scheduler {
    public:
        priority_queue<Process*, vector<Process*>, CompareSRTF> run_queue;
        void add_process(Process* p);
        Process* get_next_process();
        string str_sche();
};

class RR: public Scheduler {
    public:
        deque<Process*> run_queue;
        void add_process(Process* p);
        Process* get_next_process();
        string str_sche();
};

class PRIO: public Scheduler {
    public:
        vector<deque<Process*>*> active_queue, expired_queue;
        int mp;
        PRIO(int maxprio);
        bool all_empty();
        void swap();
        void add_process(Process* p);
        Process* get_next_process();
        string str_sche();
        ~PRIO();
};

class PREPRIO: public Scheduler {
    public:
        vector<deque<Process*>*> active_queue, expired_queue;
        int mp;
        PREPRIO(int maxprio);
        bool all_empty();
        void swap();
        void add_process(Process* p);
        Process* get_next_process();
        string str_sche();
        ~PREPRIO();
};

// Read random file and create randoms
// ================================================================================
extern vector<int> randoms;                 // a vector contains all random numbers
extern int ofs;                             // offset
void read_randoms(string file_path);        // read the rfile and store the numbers in randoms

// Utils
// ================================================================================
extern int quantum;
extern int maxprio;
extern char sc;
extern int vflag;
extern int tflag;
extern int eflag;
extern int pflag;

bool compare_io(pair<int, int> a, pair<int, int> b);    // used for calculating total io time.