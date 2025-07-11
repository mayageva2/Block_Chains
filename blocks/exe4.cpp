#include "block_funcs.h"

int main()
{
  try
  {
    string script = "./blocks.sh ";
    cout << "Please enter the number of blocks you would like to reload: ";

    int num = 0;
    cin >> num;

    while (num < 1)
    {
      cout << "Invalid number of blocks, please try again!" << endl;
      cout << "Please enter the number of blocks: ";
      cin >> num;

      cout << endl;
    }

    reloadDatabase(num, script);
  }
  
  catch(const exception& e)
  {
    cout << "Could not load data: " << e.what() << endl;
  }

  return 0;
}
