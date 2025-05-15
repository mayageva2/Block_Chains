#include "block_funcs.h"

int main()
{
    int num = 0;
    cout << "Please enter the number of blocks: ";
    cin >> num;
    while (num < 0 )
    {
      cout << "Invalid number of blocks, please try again";
      cout << "Please enter the number of blocks: ";
      cin >> num;
    }

    string filename = "../blocks.sh " + std::to_string(num);
    system(filename.c_str());

    filename = "./blocks.out";
    list<Block> blocks = readBlocksFile(filename);
    string databaseFilename = "database.csv";
    userMenu(blocks, databaseFilename, filename);

  return 0;
}
