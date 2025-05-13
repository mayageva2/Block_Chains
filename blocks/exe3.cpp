#include "block_funcs.h"

int main()
{
    string filename = "blocks.out";
    list<Block> blocks = readBlocksFile(filename);
    string databaseFilename = "database.csv";
    bool isOpen = open_new_csv_file(databaseFilename);
	if (isOpen) {
        print_csv_menu_to_file(databaseFilename);
        printValuesToCSVFile(databaseFilename, blocks);
	}

  return 0;
}