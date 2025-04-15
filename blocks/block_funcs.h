#ifndef BLOCK_FUNCS_H
#define BLOCK_FUNCS_H

#include <iostream>
#include <list>
#include <string>
#include <fstream>
using namespace std;


struct Block{
    string hash;
    int height;
    long long total;
    string time;
    string relayed_by;
    string prev_block;
};

list<Block> readBlocksFile(const string& filename);
void printBlocks(const list<Block>& blocks);
void printBlockByHash(const list<Block>& blocks, const string& hash);
void printBlockByHeight(const list<Block>& blocks, int height);

#endif // BLOCK_FUNCS_H