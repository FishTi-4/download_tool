#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>

#include <vector>
#include <list>
#include <chrono>

#include <functional>

namespace download { 

  class Timer_Wheel{  
  public:

    Timer_Wheel& operator=(const Timer_Wheel&) = delete;
    Timer_Wheel(const Timer_Wheel&) = delete;

    Timer_Wheel(int slot_num, std::chrono::milliseconds tick)
      : slot_num(slot_num), tick(tick), wheel(slot_num)
    {

      start();
    }

    ~Timer_Wheel(){
      stop();
    }

    void stop(){
      if(stop_flag) return;

      {
        std::unique_lock<std::mutex> lock(mtx);
        stop_flag = 1;
         for (auto& slot : wheel) slot.clear();
      }

      cv.notify_all();
      
      if (timer_thread.get_id() != std::this_thread::get_id()) {
        timer_thread.join();
      } else {
        timer_thread.detach();
      }
    }

    void start(){
      std::lock_guard<std::mutex> lock(mtx);
      stop_flag = 0;
      cv.notify_all();

      timer_thread = std::thread([this](){ 
        timer_loop();
      });
    }

    void add_timer(std::chrono::milliseconds timeout, std::function<void()> callback){
      int cnt = (timeout + tick - std::chrono::milliseconds(1)) / tick;
      cnt = std::max(1, cnt);

      if(cnt >= slot_num)
        throw std::runtime_error("timeout too long");

      {
        std::lock_guard<std::mutex> lock(mtx);
        wheel[(cur_slot + cnt) % slot_num ].push_back(callback);
      }
    }

   
    void add_always_timer(std::chrono::milliseconds timeout, std::function<void()> callback){

      int cnt = (timeout + tick - std::chrono::milliseconds(1)) / tick;
      cnt = std::max(1, cnt);

      if(cnt >= slot_num)
        throw std::runtime_error("timeout too long");

      auto pack = [callback, this, timeout](){
        callback();
        add_always_timer(timeout, callback);
      };
      {
        std::lock_guard<std::mutex> lock(mtx);
        wheel[(cur_slot + cnt) % slot_num ].push_back(pack);
      }


    }



  private:

    int stop_flag = 0;

    int slot_num;
    std::chrono::milliseconds tick;
    int cur_slot = 0;

    std::thread timer_thread;

    std::mutex mtx;
    std::condition_variable cv;

    std::vector<std::list<std::function<void()>>> wheel;
    std::list<std::function<void()>> doing;

    void run_task(){ 
      for(auto &c : doing){
        c();
      }
      doing.clear();
    }
    
    void timer_loop(){ 
      
      while(1){ 
      
        {
          std::unique_lock<std::mutex> lk(mtx);
          cv.wait_for(lk, tick, [this](){ return stop_flag; });
          if(stop_flag) break;
          swap(wheel[cur_slot], doing);
        }

        run_task();
        {
          std::unique_lock<std::mutex> lk(mtx);
          ++ cur_slot %= slot_num;
        }
        
      }


    }




  };  //class

} //namespace