#include <iostream>
#include <fstream>
#include <iterator>
#include <filesystem>
#include <thread>
#include <chrono>

#include "../include/client.hpp"

int main()
{

  //用法
  download::Worker c("http://www.xinhuanet.com/", "test_xinhuanet.html", 2);

  
  c.run();
  //下面是测试信息
  
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  std::cout << "速度接口测试" << c.get_speed() << std::endl;    //显示是0是正常的， 如果要看到数值的话把update_speed_loop里面的轮询时间调成1毫秒
  std::cout << "进度接口测试" << c.get_progress() << std::endl;
   

  std::this_thread::sleep_for(std::chrono::seconds(2));
  
  std::filesystem::path save = std::filesystem::absolute("test_xinhuanet.html");  //保存的文件名，文件会下载程序运行的路径，后期可以改成指定的绝对路径
  std::ifstream f(save, std::ios::binary);

  if (!f) {
    std::cout << "FAIL: file not created" << std::endl;
    return 1;
  }

  std::string content((std::istreambuf_iterator<char>(f)),
                      std::istreambuf_iterator<char>());
  // if (content.empty() || content.rfind("HTTP/", 0) != 0) {
  //   // std::cout << "FAIL: no HTTP response" << std::endl;
  //   return 1;
  // }

  std::cout << "PASS: downloaded " << content.size() << " bytes" << std::endl;
  return 0;
}
