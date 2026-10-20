#pragma once

#include <unistd.h>
#include <sys/socket.h>
#include <string>
#include <cstddef>
#include <atomic>

#include "thread_pool.hpp"

//download(string filename, string addr)
namespace download { 

int find_server_socket(std::string addr, std::string port);
int send_message(int server_socket, std::string mes);

class download_class{
public:

  download_class& operator=(const download_class&) = delete;

  download_class(std::string filename, std::string host, std::mutex &mtx, std::condition_variable &cv, int file_fd, std::atomic<int> &download_history)
  : host(host), file_name(filename), mtx(mtx), cv(cv), file_fd(file_fd), download_history(download_history)
  {
  }

  ~download_class(){
    if(server_socket){
      ::close(server_socket);
    }
    if(file_fd){
      ::close(file_fd);
    }
  }

  // int initialize(std::string filename, std::string addr, std::string save_as_file){
  //   close();
  //   file_name = filename;
  //   this->addr = addr;
  //   this->save_as_filename = save_as_file;
  //   done_signal = 0;
  //   pause_signal = 1;
  //   return 0;
  // }

  int run(int start_byte, int end_byte){
    // connect();

    pause_signal = 0;
    int offset = start_byte;
    
    int server_fd = find_server_socket(host, "80");
    send_request(start_byte, end_byte, server_fd);

    std::string buf;
    char tmp[4096];
    
    while(1){       //流处理头
      int sz = recv(server_fd, tmp, sizeof(tmp), 0);
      if(sz <0){
        throw std::runtime_error("cannot recv");
        return -1;
      }
      else if(sz == 0){
        break;
      }

      buf.append(tmp, sz);
      int pos = buf.find("\r\n\r\n");
      if(pos != std::string::npos){
        buf = buf.substr(pos + 4);
      }
    }

    while(1){
      int sz = pwrite(file_fd, buf.data(), buf.size(), offset);
      if(sz < 0){
        throw std::runtime_error("cannot write");
        return -1;
      }
      else if(sz == 0){
        throw std::runtime_error("write 0 bytes");
        break;
      }
      offset += sz;
      download_history += sz;
      buf = buf.substr(sz);
      if(buf.empty()) break;
    }               //流处理头

    while(1){

      {
        std::unique_lock<std::mutex> lk(mtx);
        cv.wait(lk, [&](){return (!pause_signal || done_signal);});
      }

      if(
        [this]() -> bool {
          std::lock_guard<std::mutex> lk(mtx);
          return static_cast<bool> (done_signal);
        }()
      ) break;


      int sz = recv(server_fd, tmp, sizeof(tmp), 0);    //读
      if(sz <=0){
        throw std::runtime_error("cannot recv");
        return -1;
      } 
      else if(sz == 0){
        break;
      }

      int sz1 = pwrite(file_fd, tmp, sz, offset);      //写
      if(sz1 <= 0){
        throw std::runtime_error("cannot write");
        return -1;
      } 

      offset += sz;
      download_history += sz;
    }

    ::close(server_fd);

    return 0;
  }
  
  void pause(){
    pause_signal = 1;
  }
  
  void resume(){
    pause_signal = 0;
    cv.notify_all();
  }

  int is_done(){
    return done_signal;
  }

  void exit(){
    done_signal = 1;
    cv.notify_all();
  }
  

private:

  std::mutex& mtx;
  std::condition_variable& cv;

  std::atomic<int> pause_signal = 1;
  std::atomic<int> done_signal = 0;

  int server_socket = 0;
  int file_fd = 0;

  std::string file_name;
  std::string host;

  std::atomic<int>& download_history;

  int close(){
    if(server_socket){
      ::close(server_socket);
      server_socket = 0;
    }
    return 0;
  }

  int send_request(int start_byte, int end_byte, int fd){
    std::string mes = "GET /" + file_name + " HTTP/1.1\r\n" +
                      "Host: " + host + "\r\n" +
                      "Connection: close\r\n" +
                      "Range: bytes=" + std::to_string(start_byte) + "-" + std::to_string(end_byte) + "\r\n" +
                      "\r\n";
    send_message(fd, mes);
    return 0;
  }
  int connect(){
    server_socket = find_server_socket(host, "80");
    if(server_socket < 0){
      throw std::runtime_error("cannot connect to server");
      return -1;
    }
    return 0;
  }

};

  inline int send_message(int server_socket, std::string mes){
    size_t sent = 0;
    while (sent < mes.size()){
      ssize_t n = send(server_socket, mes.c_str() + sent, mes.size() - sent, 0);
      if (n < 0) return -1;
      sent += n;
    }
    return 0;
  }

  
  


} // namespace download