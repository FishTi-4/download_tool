#include <iostream>
#include "../include/client.hpp"

#define DEBUG
int main()
{

  int fd = download::find_server_socket("www.baidu.com", "80");


  return 0;

}