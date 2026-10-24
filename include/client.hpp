#pragma once

#include "platform.hpp"

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>

#include <string>
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <chrono>

#include "download_class.hpp"
#include "http_parser.hpp"
#include "timer_wheel.hpp"

#define DEBUG

namespace download{

  extern Timer_Wheel timer;

  std::string read_all(int);

  class Worker{
  public:
    Worker(const Worker&) = delete;
    Worker& operator=(const Worker&) = delete;

  Worker(std::string url, std::string save_path, int num_thread) : 
    Worker(parse_url(url), std::move(save_path), num_thread)
  {
  }

  ~Worker() {
    downloader.exit();
    exit_flag = true;
  }

  int run(){

    try{
      int tp_fd = find_server_socket(host, "80");

      send_message(tp_fd, 
        "HEAD /" + filename + " HTTP/1.1\r\n" +
        "Host: " + host + "\r\n" +
        "Connection: close\r\n" +
        "\r\n"
      );

      std::string head = read_all(tp_fd);
      HttpResponse res = parse_http(head);
      
      // if(res.status_code != 200){
      //   std::cerr << res.status_code << std::endl;
      //   throw std::runtime_error("服务器返回错误");
      //   return -1;
      // }

      if(res.headers["Accept-Ranges"] == "bytes" ){}
      else{
        std::cout << "服务器不支持断点续传" << std::endl;
        num_thread = 1;
      } 
      close_socket(tp_fd);


      auto tm = std::chrono::milliseconds(100);
      std::this_thread::sleep_for(tm);

      // std::thread speed_thread(&Worker::update_speed_loop, this);
      // speed_thread.detach();

      timer.add_always_timer(std::chrono::milliseconds(1000), [this](){ update_speed_loop(); } );

      int content_size = stoi(res.headers["Content-Length"]);
      if(content_size == 0){
        throw std::runtime_error("文件为空");
      }
      need_download_size = content_size;

#ifdef DEBUG
  std::cout << "文件大小: " << content_size << std::endl;
#endif

      int block_size = (content_size + num_thread - 1) / num_thread;

      for(int i = 0; i < num_thread; i++){
        int start_byte = i * block_size;
        int end_byte = std::min(start_byte + block_size, content_size) - 1;
        work_threads.submit_task(
          [this, start_byte, end_byte](){
            downloader.run(start_byte, end_byte);
          }
        );
      }

    }
    catch(std::exception &e){
      std::cout << e.what() << std::endl;
    }

    return 0;
  }

  void pause(){
    downloader.pause();
  }

  void resume(){
    downloader.resume();
  }

  void stop(){
    downloader.exit();
  }

  int is_done(){
    return done_flag;
  }

  double get_progress(){
    return (double)downloaded_size / need_download_size;
  }

  double get_speed(){
    return speed;
  }



  // int change_download_task(std::string url, std::string save_path){
    
  //   downloader.exit();

  //   auto res = parse_url(url);
  //   std::string filename = res.second;
  //   std::string addr = res.first;
  //   return downloader.initialize(filename, addr, save_path);
  // }

  private:
    int file_fd;
    download_class downloader;

    int have_download = 0;

    std::mutex mtx;
    std::condition_variable cv;

    std::string host;
    std::string filename;

    std::atomic<int> downloaded_size = 0;
    int last_download_size = 0;
    int need_download_size = -1;

    bool exit_flag = false;
    bool done_flag = false;

    std::atomic<double> speed = 0;

    int num_thread;
    std::pair<std::string, std::string> parse_url(std::string url){
      int pos = url.find("://");
      if (pos == std::string::npos) pos = 0;
      else  pos += 3;

      int cut = url.find("/", pos);

      if(cut == std::string::npos) return {url.substr(pos), ""};
      else return {url.substr(pos, cut - pos), url.substr(cut + 1)};
    }

    Worker(std::pair<std::string, std::string> parse_res, std::string save_path, int num_thread)
      : file_fd(open(save_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0644)),
        downloader(parse_res.second, parse_res.first, mtx, cv, file_fd, downloaded_size),
        work_threads(num_thread), num_thread(num_thread)
    {
      if(file_fd < 0){
        throw std::runtime_error("无法打开保存文件: " + save_path);
      }
      host = parse_res.first;
      filename = parse_res.second;
    }

    void update_speed_loop(){ 
      while(1){
      
        int cur_size = downloaded_size - last_download_size;
        
        speed = cur_size;
        last_download_size = downloaded_size;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

#ifdef DEBUG 
  // if(cur_size!= 0)
  // std::cout << "下载差值: " << cur_size << std::endl;
  // if(speed != 0)
  // std::cout << "下载速度: " << speed << "M/s" << std::endl;
#endif
        if(exit_flag || done_flag) break;
        if( downloaded_size == need_download_size ){
          std::cout << "下载完成" << std::endl;
          exit_flag = true;
        }
      }
    }


    thread_pool work_threads;
  };
//==========================class===========================================================

  
  std::string read_all(int fd){ 
    
    std::string buf;
    char tmp[1024];

    while(true){ 
      int ret = recv(fd, tmp, 1024, 0);
      if (ret <= 0) break;
      buf += std::string(tmp, ret);
    }

    return std::move(buf);
  }
  int find_server_socket(std::string addr, std::string port = "80"){
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0){
      std::cout << "Error opening socket" << std::endl;
      return -1;
    }
    addrinfo hints {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
  
    addrinfo *server_info;
    int ret = getaddrinfo(addr.c_str(), port.c_str(), &hints, &server_info);
    if (ret != 0 || server_info == nullptr){
      std::cout << "Error getaddrinfo: " << gai_strerror(ret) << std::endl;
      return -1;
    }
    if (connect(server_socket, server_info->ai_addr, server_info->ai_addrlen) < 0){
      std::cout << "Error connecting" << std::endl;
      freeaddrinfo(server_info);
      return -1;
    }

#ifdef DEBUG
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &((sockaddr_in*)server_info->ai_addr)->sin_addr, ip, sizeof(ip));
    std::cout << "将从地址" << ip << "处下载" << std::endl;
#endif

    freeaddrinfo(server_info);
    return server_socket;
  }

  

  

} // namespace download