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
    catch(const exception& e)
    {
        cout << "Error reading blocks from the text file: " << filename << endl;
    }

  return 0;
}

void userMenu(list<Block>& blocks, string filename, string out) {

  int choice = 0;

  while (true)
  {
    try
    {
        printUserMenu();
      cin >> choice;
      while(choice>5 || choice<1)
      {
      cout<<"Invalid choice. Please try again: ";
      cin>>choice;
      }
      performFunc(choice, blocks, filename);

      if (choice == 5) //Case: refresh data
          blocks = readBlocksFile(out);
    }
    catch(const exception& e)
    {
        cout << "Could not load data: " << e.what() << endl;
    }
  }
}

void printUserMenu() {

  cout << "Choose an option:" << endl;
  cout << "1. Print db" << endl;
  cout << "2. Print block by hash" << endl;
  cout << "3. Print block by height" << endl;
  cout << "4. Export data to csv" << endl;
  cout << "5. Refresh data" << endl;
  cout << "Enter your choice: ";
}

void performFunc(int choice, const list<Block>& blocks, string filename) {

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
      cout << "Enter block hash: ";
      cin >> tmp;
      string hash(tmp);
      printBlockByHash(blocks, hash);
      break;
  }

  case 3:
  {
      int height;
      cout << "Enter block height: ";
      cin >> height;
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
            cout << "Error! Could not open a new csv file!" << endl;
        break;
    }

    case 5:
    {
        string script = "./blocks.sh ";
       cout << "Please enter the number of blocks you would like to reload: ";
        unsigned int num = 0;
        cin >> num;
        reloadDatabase(num, script);
        break;
    }

    default:
    {
        cout << "Illegal choice! Printing menu again..." << endl;
        break;
    }

    }
   this_thread::sleep_for(std::chrono::milliseconds(500)); //Sleeps for half a second to let the user see the result before the next print
}
