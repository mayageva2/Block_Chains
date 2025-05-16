#include "block_funcs.h"

int main()
{
    try
    {
        string filename = "../blocks.list";
        list<Block> blocks = readBlocksFile(filename);
        printBlocks(blocks);
    }
    catch(const std::exception& e)
    {
        std::cout << "Could not load data: " << e.what() << std::endl;
    }
    
    return 0;
}