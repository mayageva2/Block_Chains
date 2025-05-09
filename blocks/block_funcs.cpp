#include "block_funcs.h"
//For sleeps
#include <thread>
#include <chrono>

list<Block> readBlocksFile(const string& filename)
{
    list<Block> blocks;
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        return blocks;
    }

    string line;
    Block currBlock;

    while(getline(file,line)){
        if(line.find("Hash: ")==0){
            currBlock.hash=line.substr(line.find(":")+2);
        }
        else if(line.find("Height: ")==0){
            currBlock.height=stoi(line.substr(line.find(":")+2));
        }
        else if(line.find("Total: ")==0){
            currBlock.total=stoll(line.substr(line.find(":")+2));
        }
        else if(line.find("Time: ")==0){
            currBlock.time=line.substr(line.find(":")+2);
        }
        else if(line.find("Relayed by: ")==0){
            currBlock.relayed_by=line.substr(line.find(":")+2);
        }
        else if(line.find("Prev Block: ")==0){
            currBlock.prev_block=line.substr(line.find(":")+2);
            blocks.push_back(currBlock);
            currBlock={}; // Reset current block
        }
    }
    return blocks;
}

void printBlocks(const list<Block>& blocks)
{
    for (auto it=blocks.begin(); it != blocks.end(); ++it) {
        cout << "Hash: " << it->hash << endl;
        cout << "Height: " << it->height << endl;
        cout << "Total: " << it->total << endl;
        cout << "Time: " << it->time << endl;
        cout << "Relayed by: " << it->relayed_by << endl;
        cout << "Prev Block: " << it->prev_block << endl;
        auto next=it;
        ++next;
        if(next!=blocks.end()){
            cout << string(40,' ')<<"|\n";
            cout << string(40,' ')<<"|\n";
            cout << string(40,' ')<<"V\n";
        }
    }
}

void printBlockByHash(const list<Block>& blocks, const string& hash)
{
    for (const auto& block : blocks) {
        if (block.hash == hash) {
            cout << "Hash: " << block.hash << endl;
            cout << "Height: " << block.height << endl;
            cout << "Total: " << block.total << endl;
            cout << "Time: " << block.time << endl;
            cout << "Relayed by: " << block.relayed_by << endl;
            cout << "Prev Block: " << block.prev_block << endl;
            return;
        }
    }
    cout << "Block with hash " << hash << " not found." << endl;
}

void printBlockByHeight(const list<Block>& blocks, int height)
{
    for (const auto& block : blocks) {
        if (block.height == height) {
            cout << "Hash: " << block.hash << endl;
            cout << "Height: " << block.height << endl;
            cout << "Total: " << block.total << endl;
            cout << "Time: " << block.time << endl;
            cout << "Relayed by: " << block.relayed_by << endl;
            cout << "Prev Block: " << block.prev_block << endl;
            return;
        }
    }
    cout << "Block with height " << height << " not found." << endl;
}

bool open_new_csv_file(std::string filename) {

    std::ifstream file(filename, std::ios::out | std::ios::trunc); //Open the csv file and removes any previous content

    if (!file.is_open()) //Case: File could not be opened
        return false;

    file.close();
    return true; //Case: File could be opened
}

void print_csv_menu_to_file(std::string filename) {

    std::ofstream file(filename);

    if (!file.is_open()) //Case: File could not be opened
    {
        std::cout << "Error! Could not open file to print csv menu" << std::endl;
        return;
    }

    //Writing menu to file without null-terminator '\0'
    file.write("hash_value,", 11);
    file.write("height,", 7);
    file.write("total,", 6);
    file.write("time,", 5);
    file.write("relayed_by,", 11);
    file.write("prev_block", 10);
    file.write("\n", 1);

    file.close();
}

void printValuesToCSVFile(std::string filename, std::list<Block> lst) {

    std::ofstream file(filename, std::ios::out | std::ios::app);

    if (!file.is_open()) //Case: File could not be opened
    {
        std::cout << "Error! Could not open file to print values to the csv" << std::endl;
        return;
    }

    //Writing every block from the list to the csv file
    for (const auto& block : lst)
    {
        //First, converting all variables of the block to char* for convinient and easy write to the csv file
        const char* hash = block.hash.c_str();
        char height[10] = {};
        sprintf(height, "%d", block.height);
        char total[15] = {};
        sprintf(total, "%lld", block.total);
        const char* time = block.time.c_str();
        const char* relayedBy = block.relayed_by.c_str();
        const char* prevBlock = block.prev_block.c_str();

        file.write(hash, strlen(hash));
        file.write(",", 1);
        file.write(height, strlen(height));
        file.write(",", 1);
        file.write(total, strlen(total));
        file.write(",", 1);
        file.write(time, strlen(time));
        file.write(",", 1);
        file.write(relayedBy, strlen(relayedBy));
        file.write(",", 1);
        file.write(prevBlock, strlen(prevBlock));
        file.write("\n", 1);
    }
    file.close();
}

void reloadDatabase(unsigned int num) {

    std::string script = "blocks.sh";
    char tmp[5]; //Maximum amount of digits for int, especially unsigned
    sprintf(tmp, "%u", num);
    std::string numOfBlocks = std::string(tmp);
    script = script + numOfBlocks;
    system(script.c_str()); //Argument int: how many blocks will be in the list
}

void userMenu(std::list<Block>& blocks, std::string filename, std::string out) {

    int choice = 0;

    while (true)
    {
        printUserMenu();
        std::cin >> choice;
        performFunc(choice, blocks, filename);

        if (choice == 5) //Case: refresh data
            blocks = readBlocksFile(out);
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
        std::cout << "Please enter the number of blocks you would like to reload: ";
        unsigned int num = 0;
        std::cin >> num;
        reloadDatabase(num);
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

void loadDatabase() {

    std::cout << "Please enter the number of blocks you would like to load: ";
    unsigned int num = 0;
    std::cin >> num;
    reloadDatabase(num);
}