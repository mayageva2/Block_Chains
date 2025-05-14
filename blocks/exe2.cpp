#include "block_funcs.h"


int main(int argc, char* argv[])
{
    string mode=argv[1];
    string value=argv[2];

    string filename = "./blocks.out";
    list<Block> blocks = readBlocksFile(filename);
    if(mode=="hash") {printBlockByHash(blocks,value); }
    else if(mode=="height"){
        int height=stoi(value);
        printBlockByHeight(blocks, height);
    }
  return 0;
}