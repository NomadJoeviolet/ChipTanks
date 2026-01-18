# clangd 配置教程（保姆级）

> 适用场景：STM32/嵌入式交叉编译（ARM GCC）、C/C++ 混编、第三方库（如 ETL）大量头文件、Windows + VS Code + clangd 语义分析。
>
> 目标：让 clangd 的“跳转/补全/诊断”与真实编译一致，消灭 `string.h not found`、`type_traits not found`、`Unsupported option '-mcpu=' for target x86_64...`、`unknown type name 'namespace'` 这类“能编译但 clangd 报错”的假红线。

---

## 1. 先搞清楚 clangd 在做什么（核心概念）

clangd 本质是在**模拟编译器**来解析你的代码。

它需要两类信息：

1. **每个源文件实际的编译参数**（include 路径、宏、`-mcpu`、`-std` 等）
   - 最佳来源：`compile_commands.json`（由 CMake/Ninja/Bear 等生成）
2. **工具链自带的系统头/标准库头路径**（如 ARM GCC 的 newlib、libstdc++ 头）
   - clangd 自己猜不准，必须让它能“问”到你的交叉编译器（query-driver）。

如果 clangd 拿不到正确配置，它会退回默认目标（Windows 下常见是 `x86_64-pc-windows-msvc`），然后你就会看到很多“离谱”的报错。

---

## 2. 安装与基本使用（VS Code）

### 2.1 安装 clangd

建议使用：VS Code 扩展 **clangd**（LLVM 官方 clangd 语言服务器的 VS Code 客户端）。

重要提醒：
- 如果你同时装了 MS C/C++ 扩展（cpptools），要注意它可能会提供另一套 IntelliSense。
- 想完全用 clangd：通常做法是**禁用 cpptools 的 IntelliSense**，只保留它的调试/构建能力（可选）。

### 2.2 “怎么确认 clangd 在工作？”

打开 VS Code：
- “输出(OUTPUT)”面板右上角下拉选择 **clangd**
- 你能看到它为哪个文件“Build preamble / publishDiagnostics”等日志。

---

## 3. 必须准备：compile_commands.json

### 3.1 为什么必须要它？

它是“clangd 和真实编译一致”的根基。

没有它，你必须手写一堆 `-I`、`-D`、`-mcpu`，很难维护。

### 3.2 在本项目里它在哪里？

你的工程构建目录里有：
- `build/Debug/compile_commands.json`
- `build/Release/compile_commands.json`

### 3.3 典型坑：Debug/Release 两套数据库

clangd 一次只能用一个 `--compile-commands-dir`。

- 用 Debug 就配 Debug
- 用 Release 就配 Release

---

## 4. VS Code 里配置 clangd（最关键）

clangd 的“启动参数”是最关键入口：`clangd.arguments`。

在项目的：`.vscode/settings.json` 里配置。

### 4.1 我们这次项目最终用的核心参数

（示例：使用 Release 数据库 + ARM GCC 驱动）

```jsonc
{
  "clangd.arguments": [
    "--compile-commands-dir=build/Release",

    // 允许 clangd 调用交叉编译器以获取系统头/标准库头路径
    // 重点：一定要能匹配到 compile_commands.json 里出现的“编译器路径”
    "--query-driver=C:\\INCLUD~1\\Tools\\EMBEDD~1\\ARM-NO~1\\142BCE~1.3RE\\bin\\AR10B2~1.EXE",

    // 允许读取 .clangd 配置文件（建议显式打开）
    "--enable-config",

    // 后台索引
    "--background-index",

    // 排错时用 verbose；稳定后可改回 error
    "--log=verbose"
  ]
}
```

#### 4.1.1 `--query-driver` 的“致命坑”（我们踩过的）

你的 `compile_commands.json` 里编译器是 Windows **8.3 短路径**形式，例如 `AR10B2~1.EXE`。

如果你写成 `arm-none-eabi-g++.exe` 但数据库里实际不是这个路径，clangd 可能不会去 query-driver，于是：
- `<string.h>` / `<type_traits>` 找不到
- 解析崩一片，但工程真实编译没问题

因此：`--query-driver` 要尽量写成“能匹配数据库里真实出现的路径”。

---

## 5. `.clangd` 配置基础（模板 + 解释）

`.clangd` 用来给 clangd **补充/覆盖**编译参数（尤其是“打开某个头文件时没有 compile_commands 条目”的情况）。

> 注意：`.clangd` 不是用来指定 compile_commands 目录的。目录必须用 `--compile-commands-dir`。

### 5.1 通用 `.clangd` 模板（可直接复制）

把它放到项目根目录：`.clangd`

```yaml
CompileFlags:
  Add:
    # 交叉编译目标（避免默认 x86_64 导致 -mcpu 不支持）
    - -target
    - arm-none-eabi

    # MCU/指令集（按你的芯片改）
    - -mcpu=cortex-m3
    - -mthumb

    # 常用工程 include（按项目实际改）
    - -ICore/Inc
    - -IDrivers/CMSIS/Include
    - -IDrivers/CMSIS/Device/ST/STM32F1xx/Include
    - -IDrivers/STM32F1xx_HAL_Driver/Inc

Diagnostics:
  UnusedIncludes: None
  MissingIncludes: None
```

### 5.2 什么时候需要写 `-isystem`？

正常情况下，`--query-driver` 生效后，clangd 会自动得到 ARM GCC 的系统头路径。

如果 query-driver 仍不稳定（或你经常单独打开库头），你可以在 `.clangd` 里额外补 `-isystem`，把 toolchain 的 `include/c++/...` 等路径写进去作为兜底。

---

## 6. 这次最难的坑：ETL 的 `.h` 是 C++，但 clangd 按 C 解析

### 6.1 现象

在 ETL 头里出现：
- `unknown type name 'namespace'`
- `unknown type name 'template'`
- `expected ';' after top level declarator`
- 甚至把 `namespace etl {}` 错误恢复成 `int etl`

并且 clangd 日志出现：
- `Indexing c17 standard library in the context of .../etl/*.h`

这说明 clangd 正在用 **C17（C 语言）**解析这个头。

### 6.2 根因

很多库（ETL 就是典型）大量使用 `.h` 扩展名，但内容是 C++。

clangd 对 `.h` 默认语言判定不总是你期待的：
- 可能按 C 解析
- 可能“借用”某个 `.c` 的编译参数（例如 `-std=gnu11` / `-x c`）来解析头

### 6.3 解决策略（强烈推荐，稳定）

**在 ETL 目录内放一个“就近生效”的 `.clangd`**，让 clangd 在解析 ETL 头时强制 C++。

我们这次最终采用：

1) 在 `Lib/etl/include/etl/.clangd` 放配置

```yaml
CompileFlags:
  Remove:
    - "-std=gnu11"
    - "-std=c11"
    - "-std=gnu99"
    - "-std=c99"
    - "-x"
    - "c"
  Add:
    - "-xc++"
    - "-std=gnu++17"
```

2) 在 `Lib/etl/include/etl/compile_flags.txt` 放 fallback（可选但建议）

```txt
-xc++
-std=gnu++17
```

这两者配合后，你单独打开 ETL 的任何头文件，一般都不会再被当 C。

---

## 7. 我们踩过的坑点总表（对照排错）

### 7.1 `string.h not found`（工程能编译，clangd 报错）

**根因**：clangd 没拿到交叉工具链的系统头路径。

**解决**：
- 配 `--compile-commands-dir=...`
- 配 `--query-driver=...` 并确保能匹配数据库里的编译器路径

### 7.2 `Unsupported option '-mcpu=' for target x86_64-pc-windows-msvc`

**根因**：clangd 回退到 Windows x86_64 目标解析（没有正确使用交叉编译参数）。

**解决**：
- 首先保证 compile_commands 被读取（`--compile-commands-dir`）
- `.clangd` 里兜底加 `-target arm-none-eabi`

### 7.3 `<type_traits>` / C++ 标准库头找不到

**根因**：和 `string.h` 类似，本质还是 query-driver 没生效（或者编译器路径不匹配）。

**解决**：同 7.1。

### 7.4 ETL 报 `namespace` / `template`（最容易误判）

**根因**：头被按 C 解析。

**解决**：同第 6 节，用“就近 `.clangd` + compile_flags”强制 C++。

---

## 8. 保姆级排错流程（以后你自己就能定位）

按顺序做，基本不会走弯路：

1) **看 clangd 日志**（输出面板选择 clangd）
2) 找到你打开的文件对应的日志片段，观察关键字：
   - 是否出现 `Indexing c17 standard library`？（说明它按 C 解析）
   - 是否出现 `Indexing c++ standard library`？（说明它按 C++ 解析）
3) 确认 `compile_commands.json` 是否存在且内容包含该文件
4) 检查 `.vscode/settings.json`：
   - `--compile-commands-dir` 是否指向正确目录
   - `--query-driver` 是否能匹配 compile_commands 里的编译器路径
5) 若是库头（尤其是 `.h` 写 C++）：
   - 在库目录下放“就近 `.clangd`”强制语言
6) 最后手段（兜底）：
   - 在 `.clangd` 里补 `-isystem` 指向工具链头文件路径

---

## 9. 一套通用配置模板（可复制到新项目）

### 9.1 `.vscode/settings.json`（通用）

```jsonc
{
  "clangd.arguments": [
    "--compile-commands-dir=build/Debug",
    "--query-driver=C:\\path\\to\\arm-none-eabi-g++.exe",
    "--enable-config",
    "--background-index",
    "--log=error"
  ]
}
```

### 9.2 项目根 `.clangd`（通用）

```yaml
CompileFlags:
  Add:
    - -target
    - arm-none-eabi
    - -mcpu=cortex-m3
    - -mthumb
    - -ICore/Inc
    - -IDrivers/CMSIS/Include
    - -IDrivers/CMSIS/Device/ST/STM32F1xx/Include
    - -IDrivers/STM32F1xx_HAL_Driver/Inc

Diagnostics:
  UnusedIncludes: None
  MissingIncludes: None
```

### 9.3 “库目录就近 `.clangd`”（当库头是 `.h` 但内容是 C++ 时）

把它放在库头目录（例如 `third_party/foo/include/foo/.clangd`）：

```yaml
CompileFlags:
  Remove:
    - "-std=gnu11"
    - "-std=c11"
    - "-x"
    - "c"
  Add:
    - "-xc++"
    - "-std=gnu++17"
```

---

## 10. 建议的“稳定后收尾”

当一切正常后：
- 把 `--log=verbose` 改回 `--log=error`（减少输出噪声）
- 保持 `--enable-config`（推荐一直开）

---

### 附：常用小贴士

- “报错能编译”的 80% 都是：clangd 没拿到正确的 compile_commands 或 query-driver。
- 看到 `namespace/template` 这类关键字报错，优先怀疑“它按 C 解析了”。
- `.clangd` 规则不生效时，用日志确认它是否启用了 `--enable-config`，以及是否读到了你期望的 `.clangd`。
