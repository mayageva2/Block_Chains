#include "block_funcs.h"

int main()
{
    string filename = "blocks.out";
    list<Block> blocks = readBlocksFile(filename);
    string databaseFilename = "database.csv";
    userMenu(blocks, databaseFilename, filename);

  return 0;
}