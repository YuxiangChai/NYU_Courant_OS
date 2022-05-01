#include <string>
#include <iostream>
#include <vector>
#include <deque>
using namespace std;

// IO request class
// ============================================================
class IOrequest {
    public:
        int op;
        int arr_ts;
        int track;
        int start_ts;
        int end_ts;
        IOrequest(int op1, int ts1, int track1);
};

// Schedulers
// ============================================================
class Scheduler {
    public:
        vector<IOrequest*> io_que;
        vector<IOrequest*> active_que;
        virtual IOrequest* get_io() {return nullptr;};
        virtual void add_req(IOrequest* req) {io_que.push_back(req);};
};

class FIFO: public Scheduler {
    public:
        IOrequest* get_io();
};

class SSTF: public Scheduler {
    public:
        IOrequest* get_io();
};

class LOOK: public Scheduler {
    public:
        IOrequest* get_io();
};

class CLOOK: public Scheduler {
    public:
        IOrequest* get_io();
};

class FLOOK: public Scheduler {
    public:
        int swap = 0;
        IOrequest* get_io();
};


// Global variables
// ==============================================
extern bool sflag;
extern bool vflag;
extern bool qflag;
extern bool fflag;
extern deque<IOrequest*> requests;
extern int direction;
extern int head;