#pragma once

#include <thread>
#include <future>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include <functional>

namespace download{

  
  class thread_pool{

  public:


  thread_pool(int num_thread) : num_thread(num_thread) {
    for(int i = 0; i < num_thread; i++){
      threads.emplace_back([this](){ event_loop(); });
    }
  }

  ~thread_pool(){
    exit();
  }

  void exit(){
    {
      std::lock_guard<std::mutex> lock(mtx);
      this->stop_sign = 1;
      this->exit_sign = 1;
    }
    cv.notify_all();
    for(auto& t : threads) t.join();
    return;
  }


  template<typename T, typename... args>
  auto submit_task(T&& fn, args&&... inp)
    -> std::future<std::invoke_result_t<std::decay_t<T>, std::decay_t<args>...>>
  {

    using ret_type = std::invoke_result_t<std::decay_t<T>, std::decay_t<args>...>;
    
    auto bd = std::bind(std::forward<T>(fn), std::forward<args>(inp)...);
    auto pack = std::packaged_task<ret_type()>(std::move(bd));

    auto fut = pack.get_future();

    auto task = std::make_shared<decltype(pack)>(std::move(pack));

    {

      std::lock_guard<std::mutex> lock(mtx);   
      tasks.emplace([task](){ (*task)(); });
      cv.notify_one();
    }

    return fut;
  }

  private:

    void event_loop(){

      while(1){
        std::function<void()> task;
        
        {

          std::unique_lock<std::mutex> lock(mtx);
          cv.wait(lock, [this]() { return (!stop_sign && !tasks.empty()) || stop_sign;  });

          if(stop_sign || tasks.empty() ) return;

          task = std::move(tasks.front());
          tasks.pop();
        }
        task();
      }
    }

    int stop_sign = 0;
    int exit_sign = 0;
    int num_thread;
    

    std::mutex mtx;
    std::condition_variable cv;

    std::vector<std::thread> threads;
    std::queue<std::function<void()>> tasks;

  };




} //namespace download