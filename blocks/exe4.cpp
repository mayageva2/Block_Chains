#include "block_funcs.h"

int main()
{
  try
  {
    std::string script = "./blocks.sh ";
    std::cout << "Please enter the number of blocks you would like to reload: ";

    int num = 0;
    std::cin >> num;

    while (num < 1 )
    {
      cout << "Invalid number of blocks, please try again!" << std::endl;
      cout << "Please enter the number of blocks: ";
      cin >> num;

      cout << endl;
    }

    reloadDatabase(num, script);
  }
  
  catch(const std::exception& e)
  {
    std::cout << "Could not load data: " << e.what() << std::endl;
  }

  return 0;
}