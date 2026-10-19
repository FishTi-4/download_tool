# 贡献指南

欢迎提 issue 和 PR。参与前请先搜索，避免重复；友善交流，就事论事。

## 报告 Bug

请尽量包含：操作系统与编译器版本、复现步骤、预期/实际行为、日志或截图。

## 提交 PR

1. Fork 仓库，基于 `main` 创建分支，命名格式 `类型/描述`，如 `feat/md5-check`、`fix/deadlock`；
2. 确保通过测试：

   ```bash
   mkdir -p build && cd build
   cmake ..
   make
   ./test_thread && ./test_client && ./test_find_servet
   ```

3. Commit message 遵循 [Conventional Commits](https://www.conventionalcommits.org/zh-hans/)：`type(scope): subject`，subject 用祈使句、≤50 字符、不加句号。

   | type | 说明 | type | 说明 |
   | ---- | ---- | ---- | ---- |
   | `feat` | 新功能 | `test` | 测试 |
   | `fix` | 修复 | `build` | 构建/依赖 |
   | `docs` | 文档 | `chore` | 杂项 |
   | `refactor` | 重构 | `revert` | 回滚 |

   示例：`feat(parser): 支持解析 Transfer-Encoding 头`

4. PR 标题与 commit 同格式，描述改动动机、内容与测试情况。一个 PR 只做一件事。

## 代码规范

- C++20，2 空格缩进；
- 类、函数、变量使用小写蛇形命名（如 `thread_pool`、`read_all`、`pause_signal`）；
- 头文件用 `#pragma once`，文件末尾以 `} // namespace download` 收尾；
- 新增功能请补充 `test/` 下的测试，并同步更新 `README.md`。

## 许可证

贡献默认遵循 [MIT License](./LICENSE)。
