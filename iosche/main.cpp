#include <iostream>
#include <string>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <deque>
#include <unistd.h>
#include <iomanip>
#include "header.h"
using namespace std;

bool sflag = false;
bool vflag = false;
bool qflag = false;
bool fflag = false;
char algo;
int direction = 1;
deque<IOrequest*> requests;
deque<IOrequest*> requests_dup;

int head = 0;


int main(int argc, char **argv) {
    string st;
    int index;
    int c;
    string input_file;

    // parse command line arguments
    // ============================================================================
    while ((c=getopt(argc, argv, "s:vqf")) != -1)
        switch (c) {
            case 's':
                sflag = true;
                algo = *optarg;
                break;
            case 'v':
                vflag = true;
                break;
            case 'q':
                qflag = true;
                break;
            case 'f':
                fflag = true;
                break;
            case '?':
                cout << "Unknown option" << endl;
                break;
            default:
                abort();
        }

    input_file = argv[optind];

    // Read input file and initialize
    // ============================================================================
    ifstream f;
    f.open(input_file.c_str());
    string line;
    int op = 0;
    while (getline(f, line)) {
        if (line[0] == '#') continue;
        istringstream iss(line);
        int ts;
        int track;
        iss >> ts >> track;
        IOrequest* req = new IOrequest(op, ts, track);
        requests.push_back(req);
        requests_dup.push_back(req);
        op++;
    }

    Scheduler* sche;
    switch (algo) {
    case 'i':
        sche = new FIFO;
        break;
    case 'j':
        sche = new SSTF;
        break;
    case 's':
        sche = new LOOK;
        break;
    case 'c':
        sche = new CLOOK;
        break;
    case 'f':
        sche = new FLOOK;
        break;
    default:
        break;
    }

    // Simulation
    // ============================================================================
    int tot_movement = 0;
    int total_time = 0;
    int cur_time = 0;
    IOrequest* cur_io = nullptr;
    if (vflag) cout << "TRACE" << endl;
    while (true) {
        if (!requests.empty() && requests.front()->arr_ts == cur_time) {
            IOrequest* req = requests.front();
            if (vflag) cout << cur_time << ": " << setw(5) << req->op << " add " << req->track << endl;
            sche->add_req(req);
            requests.pop_front();
        }
        if (cur_io && cur_io->track == head) {
            cur_io->end_ts = cur_time;
            if (vflag) cout << cur_time << ": " << setw(5) << cur_io->op << " finish " << cur_io->end_ts - cur_io->arr_ts << endl;
            cur_io = nullptr;
            continue;
        }
        else if (cur_io && cur_io->track != head) {
            head += direction;
            tot_movement++;
        }
        if (cur_io == nullptr) {
            if (!sche->io_que.empty() || !sche->active_que.empty()) {
                cur_io = sche->get_io();
                cur_io->start_ts = cur_time;
                if (vflag) cout << cur_time << ": " << setw(5) << cur_io->op << " issue " << cur_io->track << " " << head << endl;
                if (cur_io->track == head) {
                    cur_io->end_ts = cur_time;
                    if (vflag) cout << cur_time << ": " << setw(5) << cur_io->op << " finish " << cur_io->end_ts - cur_io->arr_ts << endl;
                    cur_io = nullptr;
                    continue;
                }
                else {
                    head += direction;
                    tot_movement++;
                }
            }
            else if (requests.empty() && sche->io_que.empty() && sche->active_que.empty()) {
                total_time = cur_time;
                break;
            }
        }
        cur_time++;
    }
    
    // Print results
    // ============================================================
    double turnaround = 0.0;
    double waittime = 0.0;
    int max_waittime = 0;

    for (int i=0; i<requests_dup.size(); i++) {
        printf("%5d: %5d %5d %5d\n", i, requests_dup[i]->arr_ts, requests_dup[i]->start_ts, requests_dup[i]->end_ts);
        turnaround += requests_dup[i]->end_ts - requests_dup[i]->arr_ts;
        waittime += requests_dup[i]->start_ts - requests_dup[i]->arr_ts;
        max_waittime = max(max_waittime, requests_dup[i]->start_ts - requests_dup[i]->arr_ts);
    }
    printf("SUM: %d %d %.2lf %.2lf %d\n", total_time, tot_movement, turnaround / requests_dup.size(), waittime / requests_dup.size(), max_waittime);

    // Free
    // ============================================================
    for (int i=0; i<requests_dup.size(); i++) {
        delete requests_dup[i];
    }
    delete sche;

    return 0;
}