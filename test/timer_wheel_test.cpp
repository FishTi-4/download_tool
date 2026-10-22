#include "../include/timer_wheel.hpp"

#include <atomic>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

using namespace std::chrono_literals;

// ---------------------------------------------------------------------------
// 迷你测试框架：项目无第三方测试库，用计数断言 + PASS/FAIL 输出保持风格一致
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
    std::this_thread::sleep_for(5ms);
  }
  return pred();
}

// ---------------------------------------------------------------------------
// 用例 1：单次定时器按时触发
// ---------------------------------------------------------------------------
static void test_basic_fire() {
  std::atomic<int> count{0};
  std::chrono::steady_clock::time_point fired_at{};

  {
    download::Timer_Wheel wheel(32, 10ms);
    auto t0 = std::chrono::steady_clock::now();

    wheel.add_timer(30ms, [&] {
      fired_at = std::chrono::steady_clock::now();
      count++;
    });

    bool fired = wait_until(500ms, [&] { return count.load() > 0; });
    auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(fired_at - t0)
            .count();

    check(fired, "定时器被触发");
    check(count.load() == 1, "回调恰好触发 1 次");
    // 时间轮有量化误差：30ms 的定时器落槽后应在 [25ms, 100ms] 内触发
    check(elapsed >= 25 && elapsed <= 100,
          "触发时间在 [25ms, 100ms] 内, 实际=" + std::to_string(elapsed) +
              "ms");
  }
  check(true, "析构函数正常完成");
}

// ---------------------------------------------------------------------------
// 用例 2：多个定时器按顺序触发
// ---------------------------------------------------------------------------
static void test_order() {
  std::atomic<int> fired{0};
  std::atomic<int> seq{0};
  std::atomic<int> order_a{-1};
  std::atomic<int> order_b{-1};

  {
    download::Timer_Wheel wheel(32, 10ms);

    wheel.add_timer(20ms, [&] {
      order_a = ++seq;
      fired++;
    });
    wheel.add_timer(60ms, [&] {
      order_b = ++seq;
      fired++;
    });

    wait_until(1s, [&] { return fired.load() >= 2; });
  }

  check(fired.load() == 2, "两个定时器都只触发一次");
  check(order_a.load() == 1 && order_b.load() == 2,
        "20ms 的先于 60ms 的触发 (顺序=" + std::to_string(order_a.load()) +
            "," + std::to_string(order_b.load()) + ")");
}

// ---------------------------------------------------------------------------
// 用例 3：同槽批量触发
// ---------------------------------------------------------------------------
static void test_batch() {
  std::atomic<int> count{0};

  {
    download::Timer_Wheel wheel(32, 10ms);
    for (int i = 0; i < 100; i++) {
      wheel.add_timer(10ms, [&] { count++; });
    }
    wait_until(2s, [&] { return count.load() >= 100; });
  }

  check(count.load() == 100,
        "100 个同 timeout 定时器全部触发 (实际=" + std::to_string(count.load()) +
            ")");
}

// ---------------------------------------------------------------------------
// 用例 4：回绕边界
// slot_num=8, tick=10ms：
// 4a. timeout=80ms -> cnt == slot_num，落槽会绕回当前槽、在下个 tick 提前
//     触发，实现选择直接拒绝（方案 A），应抛 std::runtime_error；
// 4b. 边界内最大合法值 70ms（cnt=7）应正常触发且不得提前。
// 回归用例：若判断被改回 cnt > slot_num，4a 将不再抛异常，4b 类场景会在
// 首个 tick 提前触发。
// ---------------------------------------------------------------------------
static void test_wrap_around() {
  // 4a：cnt == slot_num 必须被拒绝
  {
    bool threw = false;
    download::Timer_Wheel wheel(8, 10ms);
    try {
      wheel.add_timer(80ms, [] {});
    } catch (const std::runtime_error&) {
      threw = true;
    }
    check(threw, "cnt == slot_num 的 timeout 抛出 runtime_error");
  }

  // 4b：边界内最大值 70ms 正常触发且不提前
  std::atomic<int> count{0};
  std::atomic<long long> elapsed_ms{-1};

  {
    download::Timer_Wheel wheel(8, 10ms);
    auto t0 = std::chrono::steady_clock::now();

    wheel.add_timer(70ms, [&] {
      elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::steady_clock::now() - t0)
                       .count();
      count++;
    });

    wait_until(200ms, [&] { return count.load() > 0; });
  }

  long long e = elapsed_ms.load();
  check(count.load() == 1, "定时器只触发一次");
  check(e >= 60, "70ms 定时器不得提前触发 (实际=" + std::to_string(e) + "ms)");
  check(e <= 120,
        "70ms 定时器在 120ms 内触发 (实际=" + std::to_string(e) + "ms)");
}

// ---------------------------------------------------------------------------
// 用例 6：stop() 不挂死 + 析构不挂起
// 回归用例：修复前 cv.wait 谓词反向，stop() 的 join 会永久挂死。
// ---------------------------------------------------------------------------
static void test_stop_no_hang() {
  // 6a：显式 stop() 必须正常返回
  {
    download::Timer_Wheel wheel(32, 10ms);
    std::atomic<int> count{0};
    wheel.add_timer(100ms, [&] { count++; });

    wheel.stop();  // 挂死则整个测试进程卡住，即回归信号

    check(true, "stop() 正常返回");
    check(count.load() == 0, "stop 后挂起的定时器不再触发");
  }

  // 6b：仅靠作用域析构（析构函数里自动 stop）也必须正常结束
  {
    download::Timer_Wheel wheel(32, 10ms);
    std::atomic<int> count{0};
    wheel.add_timer(100ms, [&] { count++; });
  }
  check(true, "析构路径 stop 不挂起");
}

// ---------------------------------------------------------------------------
// 用例 7：超长 timeout 抛异常
// slot_num=8, tick=10ms -> 最大允许 cnt=8, 90ms 需要 cnt=9, 应抛 runtime_error
// ---------------------------------------------------------------------------
static void test_too_long_throws() {
  bool threw = false;
  {
    download::Timer_Wheel wheel(8, 10ms);
    try {
      wheel.add_timer(90ms, [] {});
    } catch (const std::runtime_error&) {
      threw = true;
    }
  }
  check(threw, "timeout 超过 slot_num*tick 时抛出 std::runtime_error");
}

// ---------------------------------------------------------------------------
// 用例 8：回调里重入 add_timer（无死锁）
// 回归用例：回调在锁内执行时会自死锁；回调里再挂定时器必须正常工作。
// ---------------------------------------------------------------------------
static void test_reentrant_add() {
  std::atomic<int> fired{0};
  std::atomic<int> seq{0};
  std::atomic<int> order_inner{-1};

  {
    download::Timer_Wheel wheel(32, 10ms);

    wheel.add_timer(10ms, [&] {
      fired++;
      seq++;
      // 回调里重入添加第二个定时器
      wheel.add_timer(10ms, [&] {
        order_inner = ++seq;
        fired++;
      });
    });

    wait_until(1s, [&] { return fired.load() >= 2; });
  }

  check(fired.load() == 2, "外层与重入添加的定时器都触发");
  check(order_inner.load() == 2, "重入定时器在外层之后触发, 无死锁");
}

// ---------------------------------------------------------------------------

static void run(const char* name, void (*fn)()) {
  std::cout << "[测试] " << name << std::endl;
  fn();
}

int main() {
  std::cout << "== timer_wheel_test ==" << std::endl;

  run("1. 单次定时器按时触发", test_basic_fire);
  run("2. 多个定时器按顺序触发", test_order);
  run("3. 同槽批量触发", test_batch);
  run("4. 回绕边界", test_wrap_around);
  run("6. stop() 不挂死", test_stop_no_hang);
  run("7. 超长 timeout 抛异常", test_too_long_throws);
  run("8. 回调里重入 add_timer", test_reentrant_add);

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
