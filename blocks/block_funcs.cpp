#include "block_funcs.h"

#define MAX_PATH_LEN 50

#include <cstring>
#include <unistd.h>

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
        else if(line.find("Relayed By: ")==0){
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

void reloadDatabase(unsigned int num, std::string script) {

    char tmp[5]; //Maximum amount of digits for int, especially unsigned
    sprintf(tmp, "%u", num);
    std::string numOfBlocks = std::string(tmp);
    script = script + numOfBlocks;

    //Save current directory
    char currDir[MAX_PATH_LEN] = {}; //Initialize
    if (!getcwd(currDir, sizeof(currDir)))
    {
        std::cout << "Error getting current directory: " << strerror(errno) << std::endl;
        return;
    }

    //Change to parent directory
    if (chdir("..") != 0)
    {
        std::cout << "Error changing directory: " << strerror(errno) << std::endl;
        return;
    }

    //Execute the script in the parent directory
    system(script.c_str()); //Argument int: how many blocks will be in the list

    //Return to the original directory
    if (chdir(currDir) != 0)
        std::cout << "Error returning to original directory: " << strerror(errno) << std::endl;
}