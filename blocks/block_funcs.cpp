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
    for (const auto& block : blocks) {
        cout << "Hash: " << block.hash << endl;
        cout << "Height: " << block.height << endl;
        cout << "Total: " << block.total << endl;
        cout << "Time: " << block.time << endl;
        cout << "Relayed by: " << block.relayed_by << endl;
        cout << "Prev Block: " << block.prev_block << endl;
        cout << string(40,' ')<<"|\n";
        cout << string(40,' ')<<"|\n";
        cout << string(40,' ')<<"V\n";
    }
}




