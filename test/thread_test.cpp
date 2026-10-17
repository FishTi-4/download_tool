#include "../include/thread_pool.hpp"
#include <iostream>

int main()
{
  
  download::thread_pool pool(4);
  
  for(int i = 0; i < 10; i++)
  pool.submit_task([]() {
    std::cout << "hello world" << std::endl;
  });

  return 0;
}