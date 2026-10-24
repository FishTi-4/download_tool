#pragma once

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <io.h>
  #include <fcntl.h>
  #include <unistd.h>
  #include <sys/types.h>
  #include <stdio.h>
  #include <mutex>

  namespace download {

    // 一次性初始化 Winsock，进程退出时自动清理
    struct WsaInit {
      WsaInit(){
        WSADATA data;
        WSAStartup(MAKEWORD(2, 2), &data);
      }
      ~WsaInit(){
        WSACleanup();
      }
    };
    inline static WsaInit wsa_init;

    inline int close_socket(int fd){
      return closesocket(fd);
    }

    // MinGW 没有 pwrite；多线程共享同一个文件 fd，
    // 必须加锁保证 lseek + write 的原子性，避免分块写互相踩偏移量
    inline ssize_t pwrite_at(int fd, const void* buf, size_t n, long long offset){
      static std::mutex mtx;
      std::lock_guard<std::mutex> lock(mtx);
      if (_lseeki64(fd, offset, SEEK_SET) < 0) return -1;
      return _write(fd, buf, (unsigned)n);
    }

  }

#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <netdb.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <fcntl.h>

  #ifndef O_BINARY
  #define O_BINARY 0
  #endif

  namespace download {

    inline int close_socket(int fd){
      return ::close(fd);
    }

    inline ssize_t pwrite_at(int fd, const void* buf, size_t n, long long offset){
      return ::pwrite(fd, buf, n, offset);
    }

  }

#endif
