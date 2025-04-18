#include "block_funcs.h"

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

    auto it = blocks.begin();
    while (it != blocks.end())
    {
        const Block& block = *it;

        cout << "Hash: " << block.hash << endl;
        cout << "Height: " << block.height << endl;
        cout << "Total: " << block.total << endl;
        cout << "Time: " << block.time << endl;
        cout << "Relayed by: " << block.relayed_by << endl;
        cout << "Prev Block: " << block.prev_block << endl;

        it++; //Move to next block

        if (it != blocks.end()) //Case: print an arrow only if it is not the last
        {
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



