// ===========================================================================
// 简单用法示例
// ---------------------------------------------------------------------------
//   download::Worker c("http://www.example.com/index.html", "save.html", 4);
//
//   c.run();                 // 开始下载（异步，立即返回）
//   c.get_progress();        // 查询进度 [0, 1]
//   c.get_speed();           // 查询速度（字节/秒）
//   c.pause();               // 暂停
//   c.resume();              // 恢复
//   c.stop();                // 停止下载
//   c.is_done();             // 查询是否完成
// ===========================================================================

#include <iostream>
#include <fstream>
#include <iterator>
#include <filesystem>
#include <string>
#include <thread>
#include <chrono>

#include "../include/client.hpp"

using namespace std::chrono_literals;

// ---------------------------------------------------------------------------
// 迷你测试框架：与 timer_wheel_test.cpp 保持一致的 check + wait_until
// ---------------------------------------------------------------------------

static int g_checks = 0;
static int g_failed = 0;

static void check(bool ok, const std::string& msg) {
  g_checks++;
  if (ok) {
    std::cout << "    [PASS] " << msg << std::endl;
  } else {
    std::cout << "    [FAIL] " << msg << std::endl;
    g_failed++;
  }
}

// 轮询等待条件成立，超时返回 false
template <typename Pred>
static bool wait_until(std::chrono::milliseconds timeout, Pred pred) {
  auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    if (pred()) return true;
    std::this_thread::sleep_for(20ms);
  }
  return pred();
}

// ---------------------------------------------------------------------------
// 用例：下载页面并检测文件创建与内容（保持原有检测逻辑）
// ---------------------------------------------------------------------------
static void test_download() {
  // 用法（与原有测试相同）
  const std::string url = "http://www.xinhuanet.com/";
  const std::string save_name = "test_xinhuanet.html";  // 保存的文件名，文件会下载程序运行的路径，后期可以改成指定的绝对路径

  download::Worker c(url, save_name, 2);

  c.run();


  // 下面是测试信息（原有逻辑）
  std::cout << "速度接口测试 " << c.get_speed() << std::endl;    // 显示是0是正常的，如果要看到数值的话把update_speed_loop里面的轮询时间调成1毫秒
  std::cout << "进度接口测试 " << c.get_progress() << std::endl;

  check(c.get_speed() >= 0.0, "速度接口正常返回");
  check(c.get_progress() >= 0.0 && c.get_progress() <= 1.0,
        "进度接口返回区间 [0, 1]");

  // 等待下载完成，保证下面的文件检测发生在下载完成之后
  bool done = wait_until(30s, [&c] { return c.get_progress() >= 1.0; });
  check(done, "下载在 30 秒内完成");

  std::filesystem::path save = std::filesystem::absolute(save_name);
  std::ifstream f(save, std::ios::binary);

  if (!f) {
    check(false, "file not created");
  } else {
    check(true, "file created");
  }

  std::string content((std::istreambuf_iterator<char>(f)),
                      std::istreambuf_iterator<char>());
  // 原有检测：content 以 "HTTP/" 开头视为收到 HTTP 响应；
  // 下载器在流处理中已剥掉 HTTP 头，该判断不再适用，保持原有注释状态。
  // if (content.empty() || content.rfind("HTTP/", 0) != 0) {
  //   std::cout << "FAIL: no HTTP response" << std::endl;
  //   return 1;
  // }

  std::cout << "PASS: downloaded " << content.size() << " bytes" << std::endl;
  check(!content.empty(), "下载内容非空");
}

int main() {
  std::cout << "== client_test ==" << std::endl;

  test_download();

  std::cout << "----------------------------------------" << std::endl;
  std::cout << "共 " << g_checks << " 项断言, 失败 " << g_failed << " 项"
            << std::endl;
  if (g_failed > 0) {
    std::cout << "结果: FAIL" << std::endl;
    return 1;
  }
  std::cout << "结果: ALL PASS" << std::endl;
  return 0;
}
