#include "block_funcs.h"

#define MODE_HASH "hash"
#define MODE_HEIGHT "height"


int main()
{
  try
  {
    string mode;
    string value;
    
    std::cout<<"Enter mode: hash / height"<<std::endl;
    std::cin>>mode;
    
    while(mode!= "hash" && mode!= "height") 
    {
    std::cout << "Mode is not supported. Please enter height or hash."<<std::endl;
    cin>>mode;
    }
    
    std::cout<<"Enter value: "<<std::endl;
    std::cin>>value;
    std::cout<<std::endl;
    
    string filename = "../blocks.list";
    list<Block> blocks = readBlocksFile(filename);
    if(mode=="hash") {printBlockByHash(blocks,value); }
    else if(mode=="height"){
    int height=stoi(value);
        printBlockByHeight(blocks, height);
    }
  }
  catch(const std::exception& e)
  {
      std::cout << "Could not load data: " << e.what() << std::endl;
  }
    
  return 0;
}
