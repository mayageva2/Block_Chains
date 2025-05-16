#include "block_funcs.h"

int main()
{
    try
    {
      string filename = "../blocks.list";
      list<Block> blocks = readBlocksFile(filename);
      string databaseFilename = "database.csv";
      bool isOpen = open_new_csv_file(databaseFilename);
	    if (isOpen) {
        print_csv_menu_to_file(databaseFilename);
        printValuesToCSVFile(databaseFilename, blocks);
	    }
      else
      {
        cout << "Error: could not open the csv file" << endl;
      }
    }
    catch (const exception& e)
    {
        cout << "Could not load data: " << e.what() << endl;
    }

  return 0;
}
