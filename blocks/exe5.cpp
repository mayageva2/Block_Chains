//For sleeps
#include <thread>
#include <chrono>

#include "block_funcs.h"

//Functions Declarations
void performFunc(int choice, const list<Block>& blocks, string filename);
void userMenu(list<Block>& blocks, string filename, string out);
void printUserMenu();

int main()
{
    string filename = "../blocks.list";
    try
    {
        list<Block> blocks = readBlocksFile(filename);
        string databaseFilename = "database.csv";
        userMenu(blocks, databaseFilename, filename);
    }
    catch(const std::exception& e)
    {
        std::cout << "Error reading blocks from the text file: " << filename << endl;
    }

  return 0;
}

void userMenu(std::list<Block>& blocks, std::string filename, std::string out) {

  int choice = 0;

  while (true)
  {
    try
    {
        printUserMenu();
      std::cin >> choice;
      performFunc(choice, blocks, filename);

      if (choice == 5) //Case: refresh data
          blocks = readBlocksFile(out);
    }
    catch(const std::exception& e)
    {
        std::cout << "Could not load data: " << e.what() << std::endl;
    }
  }
}

void printUserMenu() {

  std::cout << "Choose an option:" << std::endl;
  std::cout << "1. Print db" << std::endl;
  std::cout << "2. Print block by hash" << std::endl;
  std::cout << "3. Print block by height" << std::endl;
  std::cout << "4. Export data to csv" << std::endl;
  std::cout << "5. Refresh data" << std::endl;
  std::cout << "Enter your choice: ";
}

void performFunc(int choice, const std::list<Block>& blocks, std::string filename) {

  switch (choice)
  {
  case 1:
  {
      printBlocks(blocks);
      break;
  }

  case 2:
  {
      char tmp[70];
      std::cout << "Enter block hash: ";
      std::cin >> tmp;
      std::string hash(tmp);
      printBlockByHash(blocks, hash);
      break;
  }

  case 3:
  {
      int height;
      std::cout << "Enter block height: ";
      std::cin >> height;
      printBlockByHeight(blocks, height);
      break;
  }

  case 4:
    {
        bool isOpen = open_new_csv_file(filename);
        if (isOpen)
        {
            print_csv_menu_to_file(filename);
            printValuesToCSVFile(filename, blocks);
        }
        else
            std::cout << "Error! Could not open a new csv file!" << std::endl;
        break;
    }

    case 5:
    {
        std::string script = "./blocks.sh ";
        std::cout << "Please enter the number of blocks you would like to reload: ";
        unsigned int num = 0;
        std::cin >> num;
        reloadDatabase(num, script);
        break;
    }

    default:
    {
        std::cout << "Illegal choice! Printing menu again..." << std::endl;
        break;
    }

    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500)); //Sleeps for half a second to let the user see the result before the next print
}