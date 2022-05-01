#include <string>
#include <iostream>
#include <vector>
#include <deque>
#include "header.h"
using namespace std;

// IO request instruction
// ==================================================================
IOrequest::IOrequest(int op1, int ts1, int track1) {
    op = op1;
    arr_ts = ts1;
    track = track1;
    start_ts = 0;
    end_ts = 0;
}

// Schedulers
// ==================================================================
IOrequest* FIFO::get_io() {
    IOrequest* req = io_que.front();
    io_que.erase(io_que.begin());
    if (req->track > head) direction = 1;
    else direction = -1;
    return req;
}

IOrequest* SSTF::get_io() {
    int idx = 0;
    if (qflag) cout << "\t";
    for (int i=0; i<io_que.size(); i++) {
        if (abs(io_que[i]->track - head) < abs(io_que[idx]->track - head)) {
            idx = i;
        }
        if (qflag) cout << io_que[i]->op << ":" << abs(io_que[i]->track - head) << " ";
    }
    if (qflag) cout << endl;
    IOrequest* req = io_que[idx];
    if (io_que[idx]->track >= head) direction = 1;
    else direction = -1;
    io_que.erase(io_que.begin()+idx);
    return req;
}

IOrequest* LOOK::get_io() {
    int idx = -1;
    int mn_dist = 100000000;
    if (qflag) cout << "\tGet: (";
    for (int i=0; i<io_que.size(); i++) {
        if ((io_que[i]->track - head)*direction >= 0) {
            if (abs(io_que[i]->track - head) < mn_dist) {
                idx = i;
                mn_dist = abs(io_que[i]->track - head);
            }
            if (qflag) cout << io_que[i]->op << ":" << abs(io_que[i]->track - head) << " ";
        }
    }
    if (idx == -1) {
        direction = 0 - direction;
        if (qflag) cout << ") --> change direction to " << direction << endl;
        IOrequest* req = get_io();
        return req;
    }
    IOrequest* req = io_que[idx];
    io_que.erase(io_que.begin()+idx);
    if (qflag) cout << ") --> " << req->op << " dir=" << direction << endl;
    return req;
}

IOrequest* CLOOK::get_io() {
    int idx = -1;
    int mn_dist = 100000000;
    int lowest = 100000000;
    int lowest_idx = -1;
    bool change = false;
    if (direction == -1) {
        direction = 1;
        IOrequest* req = get_io();
        return req;
    }
    if (qflag) cout << "\tGet: (";
    for (int i=0; i<io_que.size(); i++) {
        if (io_que[i]->track >= head) {
            if ((io_que[i]->track - head) < mn_dist) {
                idx = i;
                mn_dist = io_que[i]->track - head;
            }
            if (qflag) cout << io_que[i]->op << ":" << abs(io_que[i]->track - head) << " ";
        }
        if (io_que[i]->track < lowest) {
            lowest = io_que[i]->track;
            lowest_idx = i;
        }
    }
    if (idx == -1) {
        idx = lowest_idx;
        change = true;
        direction = -1;
    }
    IOrequest* req = io_que[idx];
    io_que.erase(io_que.begin()+idx);
    if (qflag && change) cout << ") --> go to bottom and pick " << req->op << endl;
    if (qflag && !change) cout << ") --> " << req->op << endl;
    return req;
}

IOrequest* FLOOK::get_io() {
    if (active_que.empty()) {
        active_que.swap(io_que);
        swap = 1 - swap;
    }
    int idx = -1;
    int mn_dist = 100000000;
    if (qflag) cout << "\tGet: (";
    for (int i=0; i<active_que.size(); i++) {
        if ((active_que[i]->track - head)*direction >= 0) {
            if (abs(active_que[i]->track - head) < mn_dist) {
                idx = i;
                mn_dist = abs(active_que[i]->track - head);
            }
            if (qflag) cout << active_que[i]->op << ":" << abs(active_que[i]->track - head) << " ";
        }
    }
    if (idx == -1) {
        direction = 0 - direction;
        if (qflag) cout << ") --> change direction to " << direction << endl;
        IOrequest* req = get_io();
        return req;
    }
    IOrequest* req = active_que[idx];
    active_que.erase(active_que.begin()+idx);
    if (qflag) cout << ") --> " << req->op << " dir=" << direction << endl;
    return req;
}