#include "block_funcs.h"


int main()
{
    string filename = "blocks.out";
    list<Block> blocks = readBlocksFile(filename);
    printBlocks(blocks);
    return 0;
}