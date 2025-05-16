#include "block_funcs.h"

#define MODE_HASH "hash"
#define MODE_HEIGHT "height"

int main(int argc, char* argv[])
{
  try
  {
    string mode=argv[1];
    string value=argv[2];

    string filename = "../blocks.list";
    list<Block> blocks = readBlocksFile(filename);
    if(mode=="hash") {printBlockByHash(blocks,value); }
    else if(mode=="height"){
        int height=stoi(value);
        printBlockByHeight(blocks, height);
    }
    else { //Case: mode was not hash or height

        std::cout << "Mode is not supported." << std::endl
                  << "Please use " << MODE_HASH << " or " << MODE_HEIGHT << ".";

    }
  }
  catch(const std::exception& e)
  {
      std::cout << "Could not load data: " << e.what() << std::endl;
  }
    
  return 0;
}