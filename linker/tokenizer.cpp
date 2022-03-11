#include <iostream>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include "header.h"
using namespace std;


bool str_empty(char* str) {
    if (str && str[0] == '\n') return true;
    return false;
}

// tokenize one line
vector<token> one_line(char* line, int line_num) {
    vector<token> tokens;
    char* word = strtok(line, " \t\r\n");

    while (word != NULL) {
        token tk;
        string word_str(word);
        tk.line_num = line_num;
        tk.offset = word - line + 1;
        tk.end_offset = word_str.length() + tk.offset;
        tk.tok = word;
        tokens.push_back(tk);
        word = strtok(NULL, " \t\r\n");
    }
    
    return tokens;
}

// tokenize the whole file
vector<token> tokenizer(const char* file_path) {
    ifstream input_file;
    input_file.open(file_path);
    string line;
    vector<token> tokens;
    int line_count = 0;
    token last;

    while (getline(input_file, line)) {
        line_count++;
        char* str = strcpy(new char[line.length() + 1], line.c_str());
        if (str_empty(str)) {
            last.tok = "";
            last.line_num = line_count;
            last.offset = 1;
            last.end_offset = 1;
            continue;
        }
        vector<token> one_line_tokens;
        one_line_tokens = one_line(str, line_count);
        tokens.insert(tokens.end(), one_line_tokens.begin(), one_line_tokens.end());
        last.tok = "";
        last.line_num = line_count;
        last.offset = line.length()+1;
        last.end_offset = line.length()+1;
        delete[] str;
    }
    tokens.push_back(last);
    input_file.close();
    return tokens;
}

// check if a token is an int and return its value
int read_int(token tk) {
    if (tk.tok == "") {
        // Rule 1
        syntax_error(0, tk);
    }
    string tok = tk.tok;
    if (!all_of(tok.begin(), tok.end(), ::isdigit) || (tok.length() >= 10) && (tok.compare("1073741824"))) {
        // Rule 1
        syntax_error(0, tk);
    }
    int n = stoi(tk.tok);
    return n;
}

// check if a token is a string and return
string read_symbol(token tk) {
    if (tk.tok == "" || !isalpha(tk.tok[0])) {
        // Rule 1
        syntax_error(1, tk);
    }
    for (int i=1; i < tk.tok.length(); i++) {
        if (!isalnum(tk.tok[i])) {
            // Rule 1
            syntax_error(1, tk);
        }
    }
    if (tk.tok.length() > 16) {
        // Rule 1
        syntax_error(3, tk);
    }
    return tk.tok;
}

// check is a token is "IAER" and return 
char read_iaer(token tk) {
    if (tk.tok == "") {
        // Rule 1
        syntax_error(2, tk);
    }
    string str = "IAER";
    if (tk.tok.length() > 1 || str.find(tk.tok[0]) == string::npos) {
        // Rule 1
        syntax_error(2, tk);
    }
    char c;
    c = tk.tok[0];
    return c;
}