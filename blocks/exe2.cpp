#include "block_funcs.h"

#define MODE_HASH "hash"
#define MODE_HEIGHT "height"

int main()
{
  try
  {
    string mode;
    string value;
    
    cout << "Enter mode: hash / height" << endl;
    cin >> mode;
    
    while (mode!= "hash" && mode!= "height") 
    {
        cout << "Mode is not supported. Please enter height or hash." << endl;
        cin >> mode;
    }
    
    cout << "Enter value: " << endl;
    cin >> value;
    cout << endl;
    
    string filename = "../blocks.list";
    list<Block> blocks = readBlocksFile(filename);
    if (mode=="hash") {printBlockByHash(blocks,value); }
    else if (mode=="height") {
        int height=stoi(value);
        printBlockByHeight(blocks, height);
    }
  }
  catch(const exception& e)
  {
      cout << "Could not load data: " << e.what() << endl;
  }
    
  return 0;
}
