#include "block_funcs.h"

int main()
{
    std::cout << "Please enter the number of blocks you would like to reload: ";
    unsigned int num = 0;
    std::cin >> num;
    reloadDatabase(num);

  return 0;
}