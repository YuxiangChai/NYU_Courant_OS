#include <iostream>
#include <vector>
#include <string>
#include <map>
using namespace std;


struct token {
    int line_num;
    int offset;
    int end_offset;
    string tok;
};

struct symbol {
    string sym;
    int val;
    string err;
    string warning;
    int mod_num;
};

bool operator<(const symbol s1, const symbol s2);

// This function is to print out the error message. 
void syntax_error(int errcode, token tk);

// Pass 1
vector<pair<symbol, pair<int, int>>> parse1(vector<token> tokens);

// Pass 2
void parse2(vector<pair<symbol, pair<int, int>>>& sym_used, vector<token> tokens);

// This function is to tokenize the input file.
vector<token> tokenizer(const char* file_path);

int read_int(token tk);

string read_symbol(token tk);

char read_iaer(token tk);