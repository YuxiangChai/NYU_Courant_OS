#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <iterator>
#include <iomanip>
#include "header.h"
using namespace std;


bool operator<(const symbol s1, const symbol s2) {
    return s1.sym < s2.sym;
}

// syntax error when tokenizing
void syntax_error(int errcode, token tk) {
    const char* errstr[] = {
        "NUM_EXPECTED", // Number expect, anything >= 2^30 is not a number either   -- 0
        "SYM_EXPECTED", // Symbol Expected                                          -- 1
        "ADDR_EXPECTED", // Addressing Expected which is A/E/I/R                    -- 2
        "SYM_TOO_LONG", // Symbol Name is too long                                  -- 3
        "TOO_MANY_DEF_IN_MODULE", // > 16                                           -- 4
        "TOO_MANY_USE_IN_MODULE", // > 16                                           -- 5
        "TOO_MANY_INSTR", // total num_instr exceeds memory size (512)              -- 6
    };
    cout << "Parse Error line " << tk.line_num << " offset " << tk.offset << ": " << errstr[errcode] << endl;
    exit(EXIT_FAILURE);
}

// add symbol and its usage to the vector.
void add_sym_tab(vector<pair<symbol, pair<int, int>>>& sym_used, string sym, int val, int mod_num, int mod_addr, int size) {
    for (auto iter = sym_used.begin(); iter != sym_used.end(); iter++) {
        if (iter->first.sym == sym) {
            // Rule 2
            iter->first.err.assign(string("Error: This variable is multiple times defined; first value used"));
            return;
        }
    }
    if (val >= size) {
        // Rule 5
        cout << "Warning: Module " << mod_num << ": " << sym << " too big " << val << " (max=" << size-1 << ") assume zero relative" << endl;
        val = 0;
    }
    symbol new_sym;
    new_sym.sym = sym;
    new_sym.val = val + mod_addr;
    new_sym.mod_num = mod_num;
    new_sym.err = "";
    sym_used.push_back({new_sym, {0, 0}});
}

// Pass 1
vector<pair<symbol, pair<int, int>>> parse1(vector<token> tokens) {
    int i = 0;
    vector<pair<symbol, pair<int, int>>> sym_used;
    map<int, pair<int, int>> module_map;
    int mod_num = 1;
    int mod_addr = 0;
    int inst_total = 0;
    while (i < tokens.size() - 1) {
        int def_count = read_int(tokens.at(i));
        if (def_count > 16) {
            // Rule 1
            syntax_error(4, tokens.at(i));
        }
        i++;
        int var = i;        // to store the index of the variables
        for (int j = 0; j < def_count; j++) {
            string sym = read_symbol(tokens.at(var + j*2));
            int val = read_int(tokens.at(var + j*2 + 1));
        }
        i = i + def_count * 2;
        int use_count = read_int(tokens.at(i));
        if (use_count > 16) {
            // Rule 1
            syntax_error(5, tokens.at(i));
        }
        i++;
        for (int j = 0; j < use_count; j++) {
            string sym = read_symbol(tokens.at(i + j));
        }
        i = i + use_count;
        int inst_count = read_int(tokens.at(i));
        inst_total = inst_total + inst_count;
        if (inst_total > 512) {
            // Rule 1
            syntax_error(6, tokens.at(i));
        }
        i++;
        for (int j = 0; j < inst_count; j++) {
            char mode = read_iaer(tokens.at(i + j*2));
            int op = read_int(tokens.at(i + j*2 + 1));
        }
        i = i + inst_count*2;
        module_map.insert({mod_num, {mod_addr, inst_count}});
        for (int j = 0; j < def_count; j++) {
            string sym = read_symbol(tokens.at(var + j*2));
            int val = read_int(tokens.at(var + j*2 + 1));
            add_sym_tab(sym_used, sym, val, mod_num, mod_addr, inst_count);
        }
        mod_num++;
        mod_addr = mod_addr + inst_count;
    }
    cout << "Symbol Table" << endl;
    for (auto iter = sym_used.begin(); iter != sym_used.end(); iter++) {
        cout << iter->first.sym << "=" << iter->first.val;
        if (iter->first.err == "") cout << endl;
        else cout << " " << iter->first.err << endl;
    }
    cout << endl;
    return sym_used;
}

// find the index of a given variable stored in a vector
int find_in_map(vector<pair<symbol, pair<int, int>>>& sym_used, string sym) {
    for (int i = 0; i < sym_used.size(); i++) {
        if (sym_used[i].first.sym == sym) {
            return i;
        }
    }
    return -1;
}

// Pass 2
void parse2(vector<pair<symbol, pair<int, int>>>& sym_used, vector<token> tokens) {
    int i = 0;
    vector<pair<pair<int, int>, string>> mem_map;
    map<int, pair<int, int>> module_map;
    int mod_num = 1;
    int mod_addr = 0;
    int base_addr = 0;
    cout << "Memory Map" << endl;
    while (i < tokens.size() - 1) {
        vector<pair<string, int>> uselist;
        int def_count = read_int(tokens.at(i));
        i++;
        int var = i;        // to store the index of the variables
        for (int j = 0; j < def_count; j++) {
            string sym = read_symbol(tokens.at(var + j*2));
            int val = read_int(tokens.at(var + j*2 + 1));
        }
        i = i + def_count * 2;
        int use_count = read_int(tokens.at(i));
        i++;
        for (int j = 0; j < use_count; j++) {
            string sym = read_symbol(tokens.at(i + j));
            uselist.push_back({sym, 0});
            int p = find_in_map(sym_used, sym);
            if (p >= 0) {
                sym_used[p].second.first++;
            }
        }
        i = i + use_count;
        int inst_count = read_int(tokens.at(i));
        i++;
        for (int j = 0; j < inst_count; j++) {
            pair<pair<int, int>, string> mem;
            char mode = read_iaer(tokens.at(i + j*2));
            int op = read_int(tokens.at(i + j*2 + 1));
            int opcode = div(op, 1000).quot;
            int operand = div(op, 1000).rem;
            if (mode == 'E') {
                if (opcode >= 10) {
                    string err = "Error: Illegal opcode; treated as 9999";
                    mem = {{base_addr, 9999}, err};
                }
                else if (operand >= uselist.size()) {
                    // Rule 6
                    string err = "Error: External address exceeds length of uselist; treated as immediate";
                    mem = {{base_addr, op}, err};
                }
                else {
                    string ref = uselist[operand].first;
                    int p = find_in_map(sym_used, ref);
                    if (p == -1) {
                        // Rule 3
                        uselist[operand].second++;
                        string err = "Error: " + ref + " is not defined; zero used";
                        mem = {{base_addr, opcode*1000}, err};
                    }
                    else {
                        sym_used[p].second.second++;
                        uselist[operand].second++;
                        int addr = opcode*1000 + sym_used[p].first.val;
                        mem = {{base_addr, addr}, ""};
                    }
                }
            }
            else if (mode == 'I') {
                if (op >= 10000) {
                    // Rule 10
                    string err = "Error: Illegal immediate value; treated as 9999";
                    mem = {{base_addr, 9999}, err};
                }
                else mem = {{base_addr, op}, ""};
            }
            else if (mode == 'R') {
                if (opcode >= 10) {
                    string err = "Error: Illegal opcode; treated as 9999";
                    mem = {{base_addr, 9999}, err};
                }
                else if (operand >= inst_count) {
                    // Rule 9
                    string err = "Error: Relative address exceeds module size; zero used";
                    mem = {{base_addr, opcode*1000 + mod_addr}, err};
                }
                else mem = {{base_addr, op + mod_addr}, ""};
            }
            else if (mode == 'A') {
                if (opcode >= 10) {
                    string err = "Error: Illegal opcode; treated as 9999";
                    mem = {{base_addr, 9999}, err};
                }
                else if (operand >= 512) {
                    // Rule 8
                    string err = "Error: Absolute address exceeds machine size; zero used";
                    mem = {{base_addr, opcode*1000}, err};
                }
                else mem = {{base_addr, op}, ""};
            }
            cout << setfill('0') << setw(3) << mem.first.first << ": " << setfill('0') << setw(4) << mem.first.second;
            if (mem.second == "") cout << endl;
            else cout << " " + mem.second << endl;
            base_addr++;
        }
        i = i + inst_count*2;
        for (int i = 0; i < uselist.size(); i++) {
            if (uselist[i].second == 0) {
                cout << "Warning: Module " << mod_num << ": " << uselist[i].first << " appeared in the uselist but was not actually used" << endl;
            }
        }
        mod_num++;
        mod_addr = mod_addr + inst_count;
    }
    cout << endl;
    for (int i = 0; i < sym_used.size(); i++) {
        if (sym_used[i].second.second == 0) {
            cout << "Warning: Module " << sym_used[i].first.mod_num << ": " << sym_used[i].first.sym << " was defined but never used" << endl;
        }
    }
    cout << endl;
}
