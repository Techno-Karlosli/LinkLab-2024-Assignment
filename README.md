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
- 编译器：g++ 12.0.0 或更高版本（需要 C++17）
- Python 3.6+
- Makefile
- Git

请使用 Git 管理你的代码，养成经常提交的好习惯。

## 快速开始

```bash
# 克隆仓库
git clone your-assignment-repo-url
cd your-assignment-repo-name

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
    std::string type;                           // 文件类型（.obj/.exe）
    std::map<std::string, FLESection> sections; // 各个段
    std::vector<Symbol> symbols;                // 符号表
    std::vector<ProgramHeader> phdrs;           // 程序头
    std::vector<SectionHeader> shdrs;           // 段头
    size_t entry = 0;                           // 入口点（仅可执行文件）
};

struct FLESection {
    std::vector<uint8_t> data;      // 段数据（按字节存储）
    std::vector<Relocation> relocs; // 重定位表
    bool has_symbols;               // 是否包含符号
};
```

我们的实验框架会自动将 FLE 文件加载为内存中的数据结构 `FLEObject`。

> [!WARNING]
> 注意，FLE 文件的格式和 `FLEObject` 中的表示，虽然大部分内容是对应的，但其中数据段的表示是不同的。FLE 文件更强调可读性，而 `FLEObject` 则更类似标准的 ELF 格式。具体来说：
>
> - 在 FLE 文件中，重定位和符号定义是内联的。符号定义的地方直接用 📤/📎/🏷️ 标记。但 `FLEObject` 中有单独的符号表，在每个 `FLESection` 中则有单独的重定位表。
> - 在 FLE 文件中，没有占位字节，需要重定位的地方直接用 ❓ 标记。但在每个 `FLESection` 的 `data` 字段中，需要重定位的地方会用 0 占位。
>
> 对于你来说，在实现链接器时，可能更多需要关注 `FLEObject` 中的表示。FLE 文件的格式主要是为了方便查看和调试。

段头（section headers）是目标文件中的重要元数据，它描述了每个段的属性和位置信息。在我们的 FLE 格式中，每个段头包含：

- `name`：段的名称（如 `.text`、`.data` 等）
- `type`：段的类型（如代码、数据等）
- `flags`：段的权限标志（如可读、可写、可执行）
- `addr`：段在内存中的起始地址
- `offset`：段在文件中的偏移量
- `size`：段的大小

这些信息对链接器来说非常重要 —— 它们不仅告诉链接器每个段的位置和大小，还指示了段的性质和访问权限。在后面的任务中，你会逐步理解如何利用这些信息来正确地组织程序的内存布局。

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

你的第一个任务是补充 [src/student/nm.cpp](src/student/nm.cpp) 中的 `FLE_nm` 方法，实现一个工具（nm）来查看这些符号。对于上面的代码，它应该输出：

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

让我们从一个简单的例子开始理解链接器的工作原理。假设有这样一个程序：

```c
// foo.c
const char str[] = "Hello, World!";  // 一个全局字符串常量

// main.c
extern const char str[];  // 声明：str 在别处定义

void _start()
{
    int ans = 0;
    for (int i = 0; i < 10; i++) {
        ans += str[i];  // 需要找到 str 的实际位置
    }
    // ... 退出系统调用 ...
}
```

编译器会将这些源文件编译成目标文件。在我们的 FLE 格式中，目标文件有两种表示形式：磁盘上的 JSON 格式和内存中的数据结构。以 `foo.fle` 为例，它在磁盘上的 JSON 格式是：

```json5
{
    "type": ".obj",
    "shdrs": [ /* ... 段头（略） ... */ ],
    ".text": [],
    ".rodata": [
        "📤: str 14 0",    // 定义一个全局符号 str，大小为 14 字节
        "🔢: 48 65 6c 6c 6f 2c 20 57 6f 72 6c 64 21 00"   // "Hello, World!"的字节表示
    ]
}
```

当这个文件被加载到内存时，它会被解析成一个 `FLEObject` 结构。其中符号 `str` 会被解析为一个 `Symbol` 结构：

```cpp
Symbol {
    .type = SymbolType::GLOBAL,  // 对应文件中的 📤
    .section = ".rodata",        // 符号所在的段
    .offset = 0,                 // 在段内的偏移
    .size = 14,                  // 符号大小
    .name = "str"                // 符号名称
}
```

而 `.rodata` 段的内容会被存储在 `FLESection` 结构中：

```cpp
FLESection {
    .data = { 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x2c, 0x20, 0x57, 
              0x6f, 0x72, 0x6c, 0x64, 0x21, 0x00 }  // "Hello, World!"
}
```

类似地，`main.fle` 在磁盘上的表示是：

```json5
{
    "type": ".obj",
    ".text": [
        "📤: _start 63 0",    // 定义一个全局符号 _start
        "🔢: 55 48 89 e5 c7 45 fc 00 00 00 00 c7 45 f8 00 00",  // 机器码
        "🔢: 00 00 eb 16 8b 45 f8 48 98 0f b6 80",
        "❓: .abs32s(str + 0)",  // 需要重定位：填入 str 的地址
        "🔢: 0f be c0 01 45 fc 83 45 f8 01 83 7d f8 09 7e e4",
        "🔢: 8b 55 fc 89 d7 b8 3c 00 00 00 0f 05 90 5d c3"
    ]
}
```

其中的重定位信息会被解析成 `Relocation` 结构：

```cpp
Relocation {
    .type = RelocationType::R_X86_64_32S,  // 32位有符号绝对寻址
    .offset = 28,                          // 重定位在段内的位置
    .symbol = "str",                       // 目标符号
    .addend = 0                            // 偏移量
}
```

在这个阶段，我们采用最简单的方案：将所有内容合并到一个叫 `.load` 的段中。这与真实的可执行文件有所不同 —— 实际的可执行文件通常会将代码（`.text`）、数据（`.data`）等放在不同的段中，并赋予它们不同的权限。但为了简化问题，我们先把所有内容放在一个段中。

链接器需要完成以下工作：

首先，将所有目标文件中的段（`.text`、`.rodata` 等）按顺序合并到 `.load` 段中。在合并的过程中，需要记录每个符号在合并后的新位置。比如 `str` 符号原本在 `foo.fle` 的 `.rodata` 段开头，合并后它的位置需要加上一个偏移量。这个过程会更新内存中 `Symbol` 结构的 `offset` 字段。

接着，处理所有的重定位项。在上面的例子中，`main.fle` 中有一个对 `str` 的引用需要重定位。

在这个阶段，我们只需要处理 `R_X86_64_32` 和 `R_X86_64_32S` 类型的重定位 —— 它们都是将符号的绝对地址填入重定位位置。

> [!TIP]
> `R_X86_64_32` 和 `R_X86_64_32S` 都会将 64 位地址截断为 32 位，区别在于链接器如何验证这个截断是否合法：
> - `R_X86_64_32`：要求截断掉的高 32 位必须为 0，这样通过零扩展可以还原出原始的 64 位值
> - `R_X86_64_32S`：要求高 32 位的值与截断后的符号位（即第 32 位）相同（全 0 或全 1），这样才能通过符号扩展还原出原始的 64 位值

链接器需要：
1. 在合并后的符号表中查找 `str` 符号，得到其最终地址（基地址 + 段偏移 + 符号偏移）
2. 将这个地址加上 `addend`（0）得到最终值
3. 根据重定位类型（`R_X86_64_32S`）将这个值截断为 32 位
4. 将计算得到的值写入到需要重定位的位置（在重定位表中指定的偏移量处）

例如，如果 `str` 最终位于地址 0x401000，那么：
- 最终值 = 0x401000 + 0 = 0x401000
- 截断为 32 位后 = 0x401000 & 0xFFFFFFFF = 0x401000
- 验证截断后的值是否合法
- 将 0x401000 写入重定位位置

最后，生成可执行文件。在内存中，我们需要创建一个新的 `FLEObject`，设置其 `type` 为 `.exe`，并生成正确的程序头（`ProgramHeader` 结构）以便加载器能够正确地加载程序。

在这个阶段，我们只生成一个程序头来描述 `.load` 段。它需要指定：
- 段名（name）：`.load`
- 加载地址（vaddr）：我们使用 0x400000
- 段的大小（size）：合并后的总大小
- 权限标志（flags）：在这个阶段，我们简单地赋予 RWX（可读、可写、可执行）权限（0b111，即十进制的 7）

> [!TIP]
> 注意程序头（Program Headers）和节头（Section Headers）的区别：
> - 节头（Section Headers）描述了文件中各个节的属性，主要用于链接和调试。
> - 程序头（Program Headers）描述了程序运行时如何将文件映射到内存，是加载程序（loader）必需的信息。
>
> 在最终的可执行文件中，节头并不是必须的，程序头是必须的。

此外，可执行文件必须指定一个入口点（entry point），也就是程序开始执行的位置。在 x86-64 程序中，这个入口点通常是名为 `_start` 的函数。链接器需要在合并后的符号表中找到 `_start` 符号，并将其地址（基地址 + 符号偏移）设置为程序的入口点。如果找不到 `_start` 符号，应该报错，因为这意味着程序缺少必需的入口点。

在完成这些工作后，我们就得到了一个完整的 `FLEObject` 结构体，它描述了整个程序的内存布局和执行流程。在 `FLE_ld` 方法返回这个结构体后，我们的实验框架会将其序列化为 JSON 格式的 FLE 文件：

```json5
{
    "type": ".exe",           // 表明这是一个可执行文件
    "phdrs": [{               // 程序头
        "name": ".load",
        "vaddr": 0x400000,    // 固定的加载地址
        "size": <总大小>,      // 合并后的总大小
        "flags": 7            // 可读、可写、可执行
    }],
    ".load": [/* ... */],     // 合并后的 .load 段内容，应该只包含机器码
    "entry": <入口地址>        // 程序的入口点
}
```

这样我们就得到了一个完整的可执行文件。

> [!TIP]
> 在实现链接器的过程中，一种可行的思路是采用多遍扫描的方法：
> 1. 第一次扫描：合并所有段的内容
> 2. 第二次扫描：收集并解析所有符号
> 3. 第三次扫描：从上到下依次处理所有重定位项，将计算出的地址回填
> 
> 最后生成程序头，并设置入口点，得到最终的 `FLEObject`。
> 
> 你也可以思考有没有更好的方法。

在实现过程中，建议先处理只有一个输入文件的简单情况。你可以使用 `readfle` 工具检查输出文件的格式是否正确并打印调试信息（比如段的合并过程、符号的新地址、重定位的处理过程）也会对调试很有帮助。

完成之后，你可以用以下命令来测试你的链接器：

```bash
make test_2
```

## 任务三：实现相对重定位

在任务二中，我们处理了最简单的重定位类型 - 访问全局变量时使用的绝对地址。但在实际程序中，最常见的其实是函数调用。让我们看一个例子:

```c
// lib.c
int add(int x) {    // 一个简单的函数
    return x + 1;
}

// main.c
extern int add(int);  // 声明：add 函数在别处定义

int main() {
    return add(41);   // 调用 add 函数
}
```

> [!TIP]
> 从任务三开始，我们开始包含 [`tests/common/minilibc.h`](./tests/common/minilibc.h) 库，这个库模拟了标准 C 库的一部分功能，提供预定义的 `_start` 函数，会将控制权交给 `main` 函数。

编译器会将这些源文件编译成目标文件。这个函数调用在我们的 FLE 格式中，表示为:

```json5
{
    "type": ".obj",
    ".text": [
        "📤: main 20 0",
        "🔢: f3 0f 1e fa 55 48 89 e5 bf 29 00 00 00 e8",
        "❓: .rel(add - 4)",  // 链接器需要将这里替换成 call 指令的目标地址
        "🔢: 5d c3"
    ]
}
```

> [!TIP]
> x86-64 的 call 指令格式是：
> - 一个字节的操作码（0xE8）
> - 四个字节的相对偏移量（小端序）

当这个文件被加载到内存时，重定位信息会被解析成一个 `Relocation` 结构:

```cpp
Relocation {
    .type = RelocationType::R_X86_64_PC32,  // 32位相对寻址
    .offset = 14,                           // 重定位位置（在段内的偏移）
    .symbol = "add",                        // 目标符号
    .addend = -4                            // 偏移量调整值
}
```

注意这里的 `.rel(add - 4)` 表示一个相对重定位（`R_X86_64_PC32` 类型）。与任务二中的绝对重定位不同，这里存储的不是函数的绝对地址，而是"要跳多远"。这种设计有两个重要的优点：

1. 代码可以加载到内存的任何位置，因为跳转距离是相对的
2. 指令更短，因为偏移量只需要 32 位（而不是完整的 64 位地址）

计算公式很简单：

```
偏移量 = 目标地址 + 附加值 - 重定位位置
      = S + A - P
```

其中：
- S (Symbol)：目标符号的地址（比如 add 函数的位置）
- A (Addend)：重定位项中的附加值（通常是 -4）
- P (Place)：重定位位置的地址（call 指令操作数的位置）

比如，如果：
- add 函数在 0x4000A0
- call 指令的操作数在 0x400035
- 附加值是 -4

那么：
```
偏移量 = 0x4000A0 + (-4) - 0x400035 = 0x67
```

最终的机器码就是：
```
E8 67 00 00 00   ; call add
```

> [!TIP]
> 为什么需要 addend（附加值）？这与 x86-64 的程序计数器 %rip 的工作方式有关：
> 1. call 指令中的偏移量是相对于 %rip 的，而 %rip 总是指向下一条指令
> 2. 所以当 CPU 执行到这个 call 指令时：
>    - %rip 已经指向了下一条指令（当前位置 + 5）
>    - 而我们需要的是从当前位置到目标的距离
> 3. 因此需要 addend = -4 来修正这个差异：
>    - 实际距离 = 目标地址 - 当前地址
>    - CPU 计算 = 目标地址 - (%rip = 当前地址 + 5)
>    - 所以偏移量 = 实际距离 - 4，即需要 addend = -4

要实现这种重定位，你需要：

1. 识别 `Relocation` 结构中的 `R_X86_64_PC32` 类型
2. 从符号表中找到目标符号的地址（S）
3. 从重定位项的 `offset` 字段获取重定位位置（P）
4. 使用 `addend` 字段作为附加值（A）
5. 计算偏移量：S + A - P
6. 将 32 位的偏移量写入到 `FLESection` 的 `data` 数组中（注意字节序）

> [!TIP]
> 调试相对重定位时，打印以下信息会很有帮助：
> - 目标符号的地址（S）
> - 重定位位置的地址（P）
> - 计算得到的偏移量
> - 最终写入的字节

### 验证

运行测试：

```bash
make test_3
```

## 任务四：处理符号冲突和访问限制

在前面的任务中，我们假设每个符号只在一个地方定义。但在实际编程中，经常会遇到同名符号。让我们看一个例子：

```c
// config.c
int debug_level = 0;  // 默认配置

// main.c
int debug_level = 1;  // 自定义配置
```

这时链接器该怎么办？应该用哪个 `debug_level`？

除了处理同名符号的问题，链接器还需要正确处理符号的可见性。在C语言中，我们可以用 `static` 关键字将符号声明为局部的：

```c
// foo.c
static int counter = 0;  // 局部符号，只在本文件可见

// main.c
extern int counter;      // 错误：不能访问其他文件的局部符号！
```

这种访问应该在链接时被发现并报错，因为这违反了局部符号的访问限制。

为了解决符号冲突和可见性问题，C语言提供了几种不同"强度"的符号：

- 强符号（GLOBAL）：普通的全局符号
- 弱符号（WEAK）：可以被覆盖的备选项
- 局部符号（LOCAL）：只在定义它的目标文件内可见

最常见的用法是，用弱符号来提供可覆盖的默认实现：

```c
// logger.c
__attribute__((weak))        // 标记为弱符号
void init_logger() {         // 默认的初始化函数
    // 使用默认配置
}

// main.c
void init_logger() {         // 强符号会覆盖默认实现
    // 使用自定义配置
}
```

在 FLE 格式中，弱符号用 📎 表示：

```json5
{
    "type": ".obj",
    ".text": [
        "📎: init_logger 20 0",  // 弱符号
        "🔢: 55 48 89 e5 ..."
    ]
}
```

链接时，符号冲突的处理规则很简单：

1. 强符号必须唯一，否则报错：
```c
// a.c
int max(int x, int y) { return x > y ? x : y; }

// b.c
int max(int x, int y) { return x > y ? x : y; }  // 错误：重复定义！
```

2. 强符号优先于弱符号：
```c
// lib.c
__attribute__((weak))
void init() { /* 默认实现 */ }

// main.c
void init() { /* 自定义实现，会覆盖默认版本 */ }
```

3. 多个弱符号时随机选择：
```c
// a.c
__attribute__((weak))
int debug = 0;

// b.c
__attribute__((weak))
int debug = 1;
```

要实现这些规则和访问限制，你需要：

1. 收集所有符号时，检查每个符号的 `SymbolType`
2. 用 `std::map<std::string, Symbol>` 按名字分组符号
3. 当遇到同名符号时：
   - 如果都是强符号，抛出异常：`"Multiple definition of strong symbol: " + sym.name`
   - 如果一个强一个弱，保留强符号
   - 如果都是弱符号，随机选择一个
4. 对于局部符号（LOCAL）：
   - 记录它们所属的目标文件
   - 当其他文件尝试引用局部符号时，抛出异常：`"Undefined symbol: " + sym.name`
   - 注意：同名的局部符号如果在不同文件中是允许的，因为它们的作用域是独立的

> [!TIP]
> 在实际代码中，除了显式声明的同名局部符号之外，你还可能会遇到一些特殊的同名局部符号情况。例如不同文件中的字符串字面量可能会生成同名的局部符号。
>
> 为了处理这些情况，建议：
> - 在局部符号名前添加文件名作为前缀，确保不同文件的同名符号能够区分
> - 使用完整的符号名（包含前缀）进行重定位查找
> - 在调试输出中显示完整的符号名，便于排查问题

完成后，你的链接器就能正确处理符号冲突和访问限制了。运行测试来验证：

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

```json5
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

当这个重定位被加载到内存时，它会被解析成：

```cpp
Relocation {
    .type = RelocationType::R_X86_64_64,  // 64位绝对寻址
    .offset = 0,                          // 重定位位置
    .symbol = "numbers",                  // 目标符号
    .addend = 0                          // 偏移量
}
```

你的任务是支持这种重定位。处理这种重定位时，计算公式与 32 位绝对寻址相同，但写入时需要写入完整的 8 字节

你可能还需要注意：

1. 考虑字节序（x86 是小端）
2. 地址要加上基地址（0x400000）

提示：

1. 使用 64 位整数存储地址
2. 小心整数溢出
3. 用 readfle 检查输出

完成后，你的链接器就能处理 64 位地址了。运行测试来验证：

```bash
make test_5
```

## 任务六：分离代码和数据

到目前为止，我们把所有内容都放在一个段中。这看起来很方便，但正如我们在汇编章所学到的那样，这给了攻击者篡改代码的机会。比如：

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

为了防止这些攻击，现代系统采用分段机制。在 FLE 格式中，编译器已经帮我们分好了段：

```json5
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

这些段在内存中有不同的权限和特性：
- `.text`: 只读且可执行，存放程序代码
- `.rodata`: 只读，存放常量数据
- `.data`: 可读写，存放已初始化的全局变量
- `.bss`: 可读写，存放未初始化的全局变量

链接器需要：
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

> [!TIP]
> BSS 段的处理需要特别注意：
> - BSS 段存放未初始化或初始化为 0 的全局变量
> - 输入文件中，Symbol 的 offset 是该符号在**其所在文件的 BSS 段**中的偏移量
> - 链接时需要重新计算每个符号在**最终文件**中的偏移量
> - BSS 段的总大小由所有输入文件的 BSS 段大小累加得到

最终生成的可执行文件应该包含这样的程序头：

```json5
{
    "type": ".exe",
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
        }
    ],
    // 其他段……
}
```

完成后，你的链接器就能生成具有正确内存布局和访问权限的可执行文件了。运行测试来验证：

```bash
make test_6
```

## 任务七：验证与总结

恭喜你完成了所有基础任务！现在让我们验证整个链接器的功能。

### 完整测试

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
✓ Local Symbol Access Test [7/7]: Passed
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
