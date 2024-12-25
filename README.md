# LinkLab 2024：构建你自己的链接器

```
 ___       ___  ________   ___  __    ___       ________  ________
|\  \     |\  \|\   ___  \|\  \|\  \ |\  \     |\   __  \|\   __  \
\ \  \    \ \  \ \  \\ \  \ \  \/  /|\ \  \    \ \  \|\  \ \  \|\ /_
 \ \  \    \ \  \ \  \\ \  \ \   ___  \ \  \    \ \   __  \ \   __  \
  \ \  \____\ \  \ \  \\ \  \ \  \\ \  \ \  \____\ \  \ \  \ \  \|\  \
   \ \_______\ \__\ \__\\ \__\ \__\\ \__\ \_______\ \__\ \__\ \_______\
    \|_______|\|__|\|__| \|__|\|__| \|__|\|_______|\|__|\|__|\|_______|
```

> 每个程序员都用过链接器，但很少有人真正理解它。
>
> 在这个实验中，你将亲手实现一个链接器，揭开程序是如何被"拼接"在一起的秘密。我们设计了一个友好的目标文件格式（FLE），让你可以专注于理解链接的核心概念。

> [!WARNING]
> 这是 LinkLab 的第一个版本，可能存在一些问题。如果你：
>
> - 发现了 bug，请[提交 issue](https://github.com/RUCICS/LinkLab-2024-Assignment/issues)（记得遵循 issue 模板）
> - 有任何疑问，请在[讨论区](https://github.com/RUCICS/LinkLab-2024-Assignment/discussions)提出
> - 想要改进实验，欢迎提交 PR
>
> 预计耗时：
>
> - 基础任务：10-15 小时
> - 进阶内容：5-10 小时
>
> 我们会认真对待每一个反馈！

[![GitHub Issues](https://img.shields.io/github/issues/RUCICS/LinkLab-2024-Assignment?style=for-the-badge&logo=github)](https://github.com/RUCICS/LinkLab-2024-Assignment/issues)

## 环境要求

- 操作系统：Linux（推荐 Ubuntu 22.04 或更高版本）
  - Windows 用户可考虑使用 WSL 2
- 编译器：g++ 12.0.0 或更高版本（需要 C++20）
- Python 3.6+
- Makefile
- Git

请使用 Git 管理你的代码，养成经常提交的好习惯。

## 快速开始

```bash
# 克隆仓库
git clone https://github.com/RUCICS/LinkLab-2024.git
cd LinkLab-2024

# 构建项目
make

# 运行测试（此时应该会失败，这是正常的）
make test_1  # 运行任务一的测试
make test    # 运行所有测试
```

## 项目结构

```
LinkLab-2024/
├── include/                    # 头文件
│   └── fle.hpp                # FLE 格式定义（请仔细阅读）
├── src/
│   ├── base/                  # 基础框架（助教提供）
│   │   ├── cc.cpp            # 编译器前端，生成 FLE 文件
│   │   └── exec.cpp          # 程序加载器，运行生成的程序
│   └── student/              # 你需要完成的代码
│       ├── nm.cpp            # 任务一：符号表查看器
│       └── ld.cpp            # 任务二~六：链接器主程序
└── tests/                    # 测试用例
    └── cases/               # 按任务分类的测试
        ├── 1-nm-test/      # 任务一：符号表显示
        ├── 2-ld-test/      # 任务二：基础链接
        └── ...             # 更多测试用例
    └── common/              # 测试用例的公共库
        └── minilibc.c       # 迷你 libc 的实现
```

每个任务都配有完整的测试用例，包括：

- 源代码：用于生成测试输入
- 期望输出：用于验证你的实现
- 配置文件：定义测试参数

你可以：

1. 阅读测试代码了解具体要求
2. 运行测试检查实现是否正确
3. 修改测试探索更多可能性

## 任务零：理解目标文件格式

在开始写链接器之前，我们需要先理解它处理的文件格式。传统的 ELF 格式虽然强大，但细节太多。为了让你专注于链接的核心概念，我们设计了 FLE (Friendly Linking Executable) 格式。

让我们通过一个简单的例子来了解它：

```c
// test.c
int message[2] = {1, 2};  // 全局数组

static int helper(int x) { // 静态函数
    return x + message[0];
}

int main() {             // 程序入口
    return helper(42);
}
```

通过 `./cc test.c -o test.o`，可以生成一个 FLE 文件：

```json5
{
    "type": ".obj", // 这是一个目标文件
    "shdrs": [
        // 各个段的元数据
        {
            "name": ".text", // 段名
            "type": 1, // 段类型
            "flags": 1, // 段权限
            "addr": 0, // 段在内存中的起始地址
            "offset": 0, // 段在文件中的偏移量
            "size": 36  // 段的大小
        },
        {
            "name": ".data",
            "type": 1,
            "flags": 1,
            "addr": 0,
            "offset": 0,
            "size": 8
        },
        {
            "name": ".bss",
            "type": 8,
            "flags": 9,
            "addr": 0,
            "offset": 0,
            "size": 0
        }
    ],
    ".text": [
        "🏷️: helper 20 0", // 局部符号
        "🔢: 55 48 89 e5 89 7d fc 8b 15", // 机器码
        "❓: .rel(message - 4)", // 需要重定位
        "🔢: 8b 45 fc 01 d0 5d c3", // 机器码
        "📤: main 16 20", // 全局符号
        "🔢: 55 48 89 e5 bf 2a 00 00 00 e8 de ff ff ff 5d c3" // 机器码
    ],
    ".data": [
        "📤: message 8 0", // 全局符号
        "🔢: 01 00 00 00 02 00 00 00" // 数据
    ],
    ".bss": []
}
```

FLE 使用表情符号来标记不同类型的信息：

1. 机器码：十六进制表示
  - 🔢 原始的机器码或数据
2. 符号：`类型: 符号名 大小 段内偏移`
  - 🏷️ 文件内部的局部符号
  - 📤 可以被其他文件引用的全局符号
  - 📎 可以被覆盖的弱符号
3. 重定位：`❓: 重定位类型(目标符号名 [+/-] 偏移量)`
  - ❓ 需要重定位的地方

这些信息在内存中用 C++ 结构体表示（定义在 `include/fle.hpp` 中）：

```cpp
struct FLEObject {
    std::string type;                           // 文件类型
    std::map<std::string, FLESection> sections; // 各个段
    std::vector<Symbol> symbols;                // 符号表
    std::vector<ProgramHeader> phdrs;           // 程序头
    std::vector<SectionHeader> shdrs;           // 段头
    size_t entry = 0;                           // 入口点
};
```

注意，FLE 文件的格式和内存中的表示，虽然大部分内容是对应的，但其中数据段的表示是不同的：

1. 重定位和符号定义是内联的（比如，符号定义的地方直接用 📤 标记，而不是分离的符号表）
2. 没有占位字节（需要重定位的地方直接用 ❓ 标记，而不是用 0 占位）

在接下来的任务中，你将逐步实现处理这种格式的工具链，从最基本的符号表查看器开始，最终实现一个完整的链接器。

准备好了吗？让我们开始第一个任务！

## 任务一：窥探程序的符号表

你有没有遇到过这样的错误？

```
undefined reference to `printf'
multiple definition of `main'
```

这些都与符号（symbol）有关。符号就像程序中的"名字"，代表了函数、变量等。让我们通过一个例子来理解：

```c
static int counter = 0;        // 静态变量：文件内可见
int shared = 42;              // 全局变量：其他文件可见
extern void print(int x);     // 外部函数：需要其他文件提供

void count() {                // 全局函数
    counter++;                // 访问静态变量
    print(counter);           // 调用外部函数
}
```

这段代码中包含了几种不同的符号：

- `counter`：静态符号，只在文件内可见
- `shared`：全局符号，可以被其他文件引用
- `print`：未定义符号，需要在链接时找到（但为方便起见，我们忽略所有未定义符号，所以不需要处理）
- `count`：全局函数符号

你的第一个任务是写一个工具（nm）来查看这些符号。对于上面的代码，它应该输出：

```
0000000000000000 T count    # 全局函数在 text 段
0000000000000000 b counter  # 静态变量在 bss 段
0000000000000000 D shared   # 全局变量在 data 段
```

每一行包含：

- 地址：符号在其所在段中的偏移量
- 类型：表示符号的类型和位置
  - 大写（T、D、B、R）：全局符号，分别表示在代码段、数据段、BSS 段、只读数据段
  - 小写（t、d、b、r）：局部符号，分别表示在代码段、数据段、BSS 段、只读数据段
  - W、V：弱符号，分别表示在代码段、还是在数据段或 BSS 段
- 名称：符号的名字

要实现这个工具，你需要：

1. 遍历符号表
2. 确定每个符号的类型
3. 按格式打印信息

提示：

- 使用 `std::setw` 和 `std::setfill` 格式化输出
- 根据 section 字段判断符号位置
- 未定义符号的 section 为空

### 验证

运行测试：

```bash
make test_1
```

## 任务二：实现基础链接器

让我们从最简单的情况开始。假设有这样一个程序：

```c
// message.c
int magic = 42;    // 一个全局变量
---
// main.c
extern int magic;  // 声明：magic 在别处定义
int main() {
    return magic;  // 需要找到 magic 的实际位置
}
```

链接器的工作就是把这些文件"拼接"在一起。具体来说，它需要：

1. 收集所有的代码和数据
2. 解决符号之间的引用（"这个变量在那个文件里"）
3. 调整代码中的地址（因为所有东西的位置都变了）

为了让你更容易理解这个过程，我们先采用最简单的方案：把所有内容都放在一个叫 `.load` 的段里。编译器已经帮我们把源代码变成了这样的目标文件：

```json
// message.fle
{
    "type": ".obj",
    "shdrs": [
        ... // 段头，我们在这里可以先忽略
    ],
    ".text": [],
    ".data": [
        "📤: magic 4 0", // 全局变量 magic
        "🔢: 2a 00 00 00" // 值：42
    ],
    ".bss": []
}
---
// main.fle
{
    "type": ".obj",
    "shdrs": [
        ... // 段头，我们在这里可以先忽略
    ],
    ".text": [
        "📤: main 16 0", // main 函数在 text 段
        "🔢: f3 0f 1e fa 55 48 89 e5 8b 05", // 机器码
        "❓: .rel(magic - 4)", // 需要 magic 的地址
        "🔢: 5d c3" // 机器码
    ],
    ".data": [],
    ".bss": []
}
```

你的任务是把这些目标文件链接成一个可执行文件，其**内存状态**应该类似为：

```json
{
    .type = "exe",              // 这是一个可执行文件
    .sections = {
        .".load" = {              // 所有内容都在这个段里
            .data = [...],      // 合并后的数据
            .relocs = []        // 重定位已完成，这里是空的
        }
    },
    .phdrs = [{                 // 程序头
        .name = ".load",
        .vaddr = 0x400000,     // 固定的加载地址
        .size = <总大小>,
        .flags = 7             // 可读、可写、可执行，在后续的任务中会修改为更 fine-grained 的权限设置
    }]
    .entry = <入口地址> // 程序的入口点
}
```

在这个阶段，我们只需要处理最简单的重定位类型：`R_X86_64_32`（32 位绝对地址）。它告诉链接器："在这里填入符号的绝对地址"。但是，你在支持 `R_X86_64_32` 重定位时，应一并支持 `R_X86_64_32S` 重定位，他们的关系是：

- `R_X86_64` 的意思是，现在需要把重定位给解析到一个 64 位地址的符号
- `32` 的意思是，这个重定位位置需要填入一个 32 位的地址
- 根据后缀的不同，重定位的含义也不同：
  - `R_X86_64_32` 表示目标的 64 位地址的前 32 位是 0，此处填写的是其后 32 位（即做零扩展恢复到 64 位）
  - `R_X86_64_32S` 表示目标的 64 位地址的前 32 位是 1，此处填写的是其后 32 位（即做符号扩展恢复到 64 位）

> [!TIP]
>
> 在实际实现中，你可能不需要花太大精力去区分 `R_X86_64_32` 和 `R_X86_64_32S` 这两种情况……

`entry` 是程序的入口点，在 x86-64 中，它通常是 `_start` 函数的地址。你可以在最后拼接好的数据中，找到 `_start` 函数的地址，并填入 `entry` 字段。

提示：

1. 先处理最简单的情况：只有一个输入文件
2. 用 readfle 工具检查你的输出是否正确
3. 打印调试信息，帮助你理解重定位过程
4. 记得更新符号的位置信息

### 验证

运行测试：

```bash
make test_2
```

## 任务三：实现相对重定位

在任务二中，我们只处理了最简单的重定位类型（`R_X86_64_32`），用于访问全局变量。但在实际程序中，最常见的其实是函数调用。让我们看一个例子：

```c
// lib.c
int add(int x) {    // 一个简单的函数
    return x + 1;
}
---
// main.c
extern int add(int);  // 声明：add 函数在别处定义
int main() {
    return add(41);   // 调用 add 函数
}
```

编译器会把它们变成：

```json
// lib.fle
{
    "type": ".obj",
    ".text": [
        "📤: add 19 0", // add 函数
        "🔢: f3 0f 1e fa 55 48 89 e5 89 7d fc 8b 45 fc 83 c0", // 机器码
        "🔢: 01 5d c3" // 机器码
    ],
    ... // 其他字段
}
---
// main.fle
{
    "type": ".obj",
    ".text": [
        "📤: main 20 0",
        "🔢: f3 0f 1e fa 55 48 89 e5 bf 29 00 00 00 e8",
        "❓: .rel(add - 4)",
        "🔢: 5d c3"
    ],
    ... // 其他字段
}
```

注意那个 `.rel(add - 4)` —— 它应该被变成一个 4 字节的地址，与前一字节 `e8` 一起，构成一条 `call` 指令。x86-64 的 `call` 指令使用相对寻址（`R_X86_64_PC32` 类型的重定位）。也就是说，它存储的不是目标函数的绝对地址，而是"要跳多远"。这样的好处是：

1. 代码可以加载到内存的任何位置（天然支持位置无关，而不需要动态链接器运行时修正地址）
2. 跳转指令更短（代码体积只要不超过 4GB，就只需要 32 位偏移量）

计算公式是：

```
偏移量 = S + A - P
```

其中:
- S (Symbol): 目标符号的地址
- A (Addend): 重定位项中的附加值
- P (Place): 要被修改的操作数的地址（重定位位置）

比如，对于一个call指令:

- 指令在 0x400034，操作数在 0x400035 (P)
- 目标函数在 0x4000A0 (S) 
- Addend 是 -4 (A)

那么：

```
偏移量 = 0x4000A0 + (-4) - 0x400035 = 0x67
```

所以 `call` 指令会变成：

```asm
E8 67 00 00 00   ; call target
```

你的任务是实现这种重定位。需要：

1. 识别 `R_X86_64_PC32` 类型的重定位
2. 正确计算偏移量（使用 S + A - P 公式）
3. 将偏移量写入到指令中

提示：

1. `call` 指令的格式是 `E8` 后跟 4 字节偏移量
2. 重定位项的 addend 通常是 -4
3. 用调试打印验证你的计算

> [!TIP]
>
> P 在这里是重定位位置的地址（即**操作数**的地址）而不是指令的地址。好在我们的实验框架会帮你计算出准确的重定位位置地址，所以你不需要过于关注这一细节。
>
> 你可以参考 [`include/fle.hpp`](./include/fle.hpp) 中的 `Relocation` 结构体，来了解重定位条目的结构。

### 验证

运行测试：

```bash
make test_3
```

从 `test_3` 开始，我们开始包含 [`tests/common/minilibc.h`](./tests/common/minilibc.h) 库，这个库模拟了标准 C 库的一部分功能，提供预定义的 `_start` 函数，会将控制权交给 `main` 函数。

## 任务四：处理符号冲突

在前面的任务中，我们假设每个符号只在一个地方定义。但实际编程中经常会遇到同名符号，比如：

```c
// config.c
int debug_level = 0;  // 默认配置
---
// main.c
int debug_level = 1;  // 自定义配置
```

这时链接器该怎么办？用哪个 `debug_level`？

为了解决这个问题，C 语言允许程序员指定符号的"强度"：

- 强符号（GLOBAL）：普通的全局符号
- 弱符号（WEAK）：可以被覆盖的备选项

最常见的用法是，用弱符号来提供可覆盖的默认实现：

```c
// logger.c
__attribute__((weak))        // 标记为弱符号
void init_logger() {         // 默认的初始化函数
    // 使用默认配置
}
---
// main.c
void init_logger() {         // 强符号会覆盖默认实现
    // 使用自定义配置
}
```

链接规则很简单：

1. 强符号必须唯一

   ```c
   // a.c
   int x = 1;        // 强符号
   ---
   // b.c
   int x = 2;        // 错误：重复定义！
   ```

2. 强符号优先于弱符号

   ```c
   // a.c
   __attribute__((weak)) int v = 1;  // 弱符号
   ---
   // b.c
   int v = 2;                        // 强符号，这个会被使用
   ```

3. 多个弱符号时取任意一个，我们在此选择第一个
   ```c
   // a.c
   __attribute__((weak)) int mode = 1;  // 这个会被使用
   ---
   // b.c
   __attribute__((weak)) int mode = 2;  // 这个会被忽略
   ```

你的任务是实现这些规则。具体来说：

1. 收集所有符号
2. 检查是否有重复的强符号（报错）
3. 在强弱符号冲突时选择强符号
4. 在多个弱符号时选择任意一个

提示：

1. 使用 `std::map` 按名字分组符号
2. 仔细检查每个符号的 `SymbolType`
3. 保持良好的错误信息

### 验证

运行测试：

```bash
make test_4
```

## 任务五：处理 64 位地址

到目前为止，我们处理的都是 32 位的地址（`R_X86_64_32` 和 `R_X86_64_PC32`）。但在 64 位系统中，有时我们需要完整的 64 位地址。比如：

```c
// 一个全局数组
int numbers[] = {1, 2, 3, 4};

// 一个指向这个数组的指针
int *ptr = &numbers[0];  // 需要完整的 64 位地址！
```

为什么这里需要 64 位地址？因为：

1. 指针本身是 64 位的（8 字节）
2. 程序可能被加载到高地址区域
3. 32 位地址最多只能访问 4GB 空间

这种情况下，编译器会生成一个新的重定位类型（`R_X86_64_64`）：

```json
{
    "type": ".obj",
    ".data": [
        "📤: numbers 16", // numbers 数组
        "🔢: 01 00 00 00", // 1
        "🔢: 02 00 00 00", // 2
        "🔢: 03 00 00 00", // 3
        "🔢: 04 00 00 00"  // 4
    ],
    ".data.rel.local": [
        "📤: ptr 8",
        "❓: .abs64(numbers + 0)" // 需要 numbers 的完整地址
    ]
    ...
}
```

注意那个 `.abs64` —— 这是一个新的重定位类型（`R_X86_64_64`），它告诉链接器："在这里填入符号的完整 64 位地址"。

你的任务是支持这种重定位。需要注意：

1. 写入完整的 64 位地址（8 字节）
2. 考虑字节序（x86 是小端）
3. 地址要加上基地址（0x400000）

提示：

1. 使用 64 位整数存储地址
2. 小心整数溢出
3. 用 readfle 检查输出

### 验证

运行测试：

```bash
make test_5
```

## 任务六：分离代码和数据

到目前为止，我们把所有内容都放在一个段里。这看起来很方便，但正如我们在汇编一章所学到的那样，这给了攻击者篡改代码的机会。比如：

```c
void hack() {
    // 1. 修改代码
    void* code = (void*)hack;
    *(char*)code = 0x90;     // 如果代码段可写，程序可以被篡改！

    // 2. 执行数据
    char shellcode[] = {...}; // 一些恶意代码
    ((void(*)())shellcode)(); // 如果数据段可执行，这就是漏洞！
}
```

为了防止这些攻击，现代系统采用分段机制：

1. 代码段（.text）：只读且可执行
2. 只读数据段（.rodata）：只读
3. 数据段（.data）：可读写
4. BSS 段（.bss）：可读写，但不占文件空间

编译器已经帮我们分好了段：

```json
{
  "type": ".obj",

  ".text": [
    "📤: main 40",
    "🔢: f3 0f 1e fa 55 48 89 e5 be 00 00 00 00 48 8d 05",
    "❓: .rel(.rodata - 4)",
    "🔢: 48 89 c7 b8 00 00 00 00 e8",
    "❓: .rel(print - 4)",
    "🔢: b8 00 00 00 00 5d c3"
  ],
  ".data": [
    "📤: counter 4",
    "🔢: 03 00 00 00" // 已初始化数据
  ],
  ".bss": [
    "📤: buffer 4096" // 未初始化数据，只记录大小
  ],
  ".rodata": [
    "🏷️: .rodata 0",
    "🔢: 48 65 6c 6c 6f 00" // "Hello"
  ]
}
```

你的任务是：

1. 保持段的独立性（不要合并）
2. 设置正确的权限：
   - `.text`: r-x（读/执行）
   - `.rodata`: r--（只读）
   - `.data`: rw-（读/写）
   - `.bss`: rw-（读/写）
3. 优化内存布局：
   - 4KB 对齐（页面大小）
   - 相似权限的段放在一起
   - BSS 段不需要文件内容

BSS（Block Started by Symbol）段是一个独特的段，它存储了程序中所有未初始化的全局变量和静态变量。与其他段不同，BSS 段在文件中不占用实际空间，只记录大小信息。

BSS 段的链接过程需要特别注意以下几点：

1. 计算总大小：

   - 遍历所有输入文件，累加它们的 BSS 段大小
   - 得到最终可执行文件中 BSS 段所需的空间大小
   - 这个大小将用于内存分配

2. 符号重定位：
   - 收集所有在 BSS 段中定义的符号
   - 计算每个符号在合并后的 BSS 段中的新位置
   - 更新所有引用这些符号的重定位条目
   - 确保重定位后的地址正确指向 BSS 段中的目标位置

> [!TIP]
>
> 在处理 `.bss` 段的符号时，你可能需要特别关注 `Symbol` 结构体中的 `offset` 字段。它表示一个符号在它所属的节中的偏移量。
>
> 参见 [`include/fle.hpp`](./include/fle.hpp) 中的 `Symbol` 结构体。

最终的可执行文件应该是这样的：

```json
{
    "type": "exe",
    "phdrs": [
        {
            "name": ".text",
            "vaddr": 0x400000,
            "size": <代码大小>,
            "flags": 5        // r-x
        },
        {
            "name": ".rodata",
            "vaddr": <对齐后的地址>,
            "size": <常量大小>,
            "flags": 4        // r--
        },
        // ...其他段...
    ],
    "entry": <入口地址>,
    ".text": { ... }, // 代码
    ".rodata": { ... }, // 常量
    ".data": { ... }, // 已初始化数据
    ".bss": { ... } // 未初始化数据
}
```

提示：

1. 段的地址要适当对齐，否则会影响性能，且会影响 loader 中 `mmap` 的分配
2. 注意更新所有重定位，因为地址都变了
3. BSS 段只需要分配空间，不需要数据，节省了空间
4. 考虑把相似权限的段合并到一个程序头中

修改 `src/student/ld.cpp`，实现内存布局的优化。

### 验证

运行测试：

```bash
make test_6
```

## 任务七：验证与总结

恭喜你完成了所有基础任务！现在让我们验证整个链接器的功能。

### 完整性检查

```bash
make test
```

你应该看到：

```
Preparing FLE Tools...
✓ FLE Tools compiled successfully
Preparing minilibc...
✓ minilibc compiled successfully

Running 12 test cases...

✓ nm Tool Test [2/2]: Passed
✓ Single File Test [3/3]: Passed
✓ Absolute Addressing Test [3/3]: Passed
✓ Absolute + Relative Addressing Test [4/4]: Passed
✓ Position Independent Executable Test [5/5]: Passed
✓ Strong Symbol Conflict Test [3/3]: Passed
✓ Weak Symbol Override Test [4/4]: Passed
✓ Multiple Weak Symbol Warning [5/5]: Passed
✓ Local Symbol Access Test [3/3]: Passed
✓ 64-bit Absolute Relocation Test [3/3]: Passed
✓ BSS Section Linking Test [5/5]: Passed
✓ Section Permission Control Test [10/10]: Passed
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┳━━━━━━━━┳━━━━━━━┳━━━━━━━━━━━┳━━━━━━━━━━━━━━━━━━━━━┓
┃ Test Case                           ┃ Result ┃  Time ┃     Score ┃ Message             ┃
┡━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━╇━━━━━━━━╇━━━━━━━╇━━━━━━━━━━━╇━━━━━━━━━━━━━━━━━━━━━┩
│ nm Tool Test                        │  PASS  │ 0.36s │ 10.0/10.0 │ All steps completed │
│ Single File Test                    │  PASS  │ 0.27s │ 10.0/10.0 │ All steps completed │
│ Absolute Addressing Test            │  PASS  │ 0.29s │ 10.0/10.0 │ All steps completed │
│ Absolute + Relative Addressing Test │  PASS  │ 0.60s │ 10.0/10.0 │ All steps completed │
│ Position Independent Executable     │  PASS  │ 0.93s │ 10.0/10.0 │ All steps completed │
│ Test                                │        │       │           │                     │
│ Strong Symbol Conflict Test         │  PASS  │ 0.70s │ 10.0/10.0 │ All steps completed │
│ Weak Symbol Override Test           │  PASS  │ 0.78s │ 10.0/10.0 │ All steps completed │
│ Multiple Weak Symbol Warning        │  PASS  │ 0.85s │ 10.0/10.0 │ All steps completed │
│ Local Symbol Access Test            │  PASS  │ 0.46s │ 10.0/10.0 │ All steps completed │
│ 64-bit Absolute Relocation Test     │  PASS  │ 0.37s │ 10.0/10.0 │ All steps completed │
│ BSS Section Linking Test            │  PASS  │ 0.77s │ 10.0/10.0 │ All steps completed │
│ Section Permission Control Test     │  PASS  │ 1.27s │ 10.0/10.0 │ All steps completed │
└─────────────────────────────────────┴────────┴───────┴───────────┴─────────────────────┘

╭────────────────────────────────────────────────────────────────────────────────────────╮
│ Total Score: 120.0/120.0 (100.0%)                                                      │
╰────────────────────────────────────────────────────────────────────────────────────────╯

```

### 实验报告要求

请参考[报告模板通用指南](./report/README.md)，基于[实验报告模板](./report/report.md)，完成实验报告。

## 完成本实验

### 提交

我们使用 GitHub Classroom 进行提交：

1. 确保所有代码已提交到你的仓库
2. GitHub Actions 会自动运行测试
3. 我们将以 Actions 的输出作为评分依据

### 评分标准

分数由正确性得分和代码质量得分组成，正确性得分占 80%，代码质量得分占 20%，即：

$$
\text{总分} = \frac{\text{正确性得分}} {\text{正确性满分}} \times 80\% + \text{代码质量得分} \times 20\%
$$

其中，正确性得分由 GitHub Classroom 自动计算，代码质量得分由助教根据代码风格和实验报告评分，具体衡量因素为：

- 代码风格
  - 代码表达能力强，逻辑清晰，代码简洁，仅在必要时添加注释
  - 积极使用 C++ 标准库，不重复造轮子
  - 防御性编程，进行多层次的错误检查
- 实验报告
  - 实验报告内容完整，格式规范，排版美观
  - 实验报告内容与代码一致，无明显矛盾，体现出对实验考察知识的基本了解
  - 有思考，有总结，有反思，有创新
  - 对实验提供有价值的反馈和建议

## 调试建议

1. 仔细阅读 `include/fle.hpp` 中的数据结构定义
2. 使用 `readfle` 工具查看 FLE 文件的内容
3. 测试用例提供了很好的参考实例
4. 链接器的调试技巧：
   - 使用 `objdump` 查看生成的文件
   - 打印中间过程的重要信息
   - 分步骤实现，先确保基本功能正确

## 进阶内容

完成了基本任务后，你可以尝试：

1. 支持更多的重定位类型
2. 实现共享库加载
3. 添加符号版本控制
4. 优化链接性能
5. 支持增量链接

这些内容不会计入本次实验的评分，学有余力的同学可以自行探索。

## 参考资料

1. [CSAPP: Computer Systems A Programmer's Perspective](https://csapp.cs.cmu.edu/)
2. [System V ABI](https://refspecs.linuxbase.org/elf/x86_64-abi-0.99.pdf)
3. [Linkers & Loaders](https://linker.iecc.com/)
4. [How To Write Shared Libraries](https://www.akkadia.org/drepper/dsohowto.pdf)
