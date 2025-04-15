#include "block_funcs.h"

int main()
{
    string filename = "blocks.out";
    list<Block> blocks = readBlocksFile(filename);
    printBlockByHeight(blocks, 892527);
    cout<<endl;
    printBlockByHash(blocks, "00000000000000000000ec1c8235b2892bacbb1ee406560de22cccbe7378c4ed");
    return 0;
}