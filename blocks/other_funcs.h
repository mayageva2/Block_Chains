#pragma once
#include <list>
#include <string>

struct Block{
    std::string hash;
    int height;
    long long total;
    std::string time;
    std::string relayed_by;
    std::string prev_block;
};

//Functions Declarations
bool open_new_csv_file(std::string filename);
void print_csv_menu_to_file(std::string filename);
void printValuesToCSVFile(std::string filename, std::list<Block> lst);
void reloadDatabase(unsigned int num);
void loadDatabase();
void userMenu(std::list<Block>& blocks, std::string filename, std::string out);
void printUserMenu();
void performFunc(int choice, const std::list<Block>& blocks, std::string filename);