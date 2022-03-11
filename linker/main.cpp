#include <iostream>
#include <vector>
#include <string>
#include "header.h"


int main(int argc, char* argv[]) {
    vector<token> tokens;
    string file_path = argv[1];
    tokens = tokenizer(file_path.c_str());          // to tokenize and store all tokens in a vactor
    vector<pair<symbol, pair<int, int>>> sym_used = parse1(tokens);         // Pass 1, and store the symbol table
    parse2(sym_used, tokens);                       // Pass 2 to print the memory map
    return 0;
}