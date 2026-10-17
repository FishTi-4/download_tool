# download_tool

基于 C++20 的多线程 HTTP 下载工具，使用 HTTP Range 请求实现分块并发下载，支持暂停、恢复。  

<br> <br>

*想说的话* <br>
*本项目属于轻量级入门下载器*
*对刚开始学习网络编程的人，应该也是一个还不错的参考，在之后预计会添加学习文档，便于其他想要学习网络编程和多线程知识的同学快速学习*<br>
*本人水平有限，不过在之后还会持续更新（目前来说是的）不断完善*<br>

*欢迎大家提issue，欢迎大家提pr*
*有不足的地方也欢迎指点*

## 特性

- 多线程分块下载
- 服务器支持 `Accept-Ranges` 时自动分段，否则降级为单线程
- 暂停 / 恢复 / 停止
- 基于 socket 的简易 HTTP 解析（`http_parser.hpp`）
- 自带线程池（`thread_pool.hpp`）

## 构建

```bash
mkdir -p build && cd build
cmake ..
make
```

## 使用

```cpp
#include "client.hpp"

download::Worker worker("http://example.com/file.zip", "file.zip", 4);
worker.run();
```

- `run()` 开始下载
- `pause()` / `resume()` 暂停 / 恢复
- `is_done()` 查询是否完成
- `kill_task()` 终止下载

## 测试

```bash
./build/test_thread    # 线程池测试
./build/test_client    # 下载测试
./build/test_find_servet
```