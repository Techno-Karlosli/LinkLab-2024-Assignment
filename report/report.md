# LinkLab 报告

姓名：李弋
学号：2022202657

## Part A: 思路简述



1. 核心方法的流程

   按照任务二的框架，核心方法为三次扫描，并最终处理文件

   **第一次扫描**：遍历目标文件，按段名合并段数据，将数据插入合并段，并调整重定位项偏移。

   ```c++
   merged_section.data.insert(merged_section.data.end(), section.data.begin(), section.data.end());
   updated_reloc.offset += section_start;
   ```

   **第二次扫描**：收集全局符号表，解析符号定义并处理冲突（强符号与弱符号），为符号添加基址偏移，确保符号地址绝对化：

   ```c++
   updated_symbol.offset += base_address;
   global_symbols[symbol.name] = updated_symbol;
   ```

   **第三次扫描**：根据符号表处理重定位项，计算绝对地址并回填到段数据。但实际上似乎这段有非常多错误。

2. 关键数据结构的设计，如何组织和管理符号表

  **符号表**：使用 `std::map<std::string, Symbol>` 存储全局符号，方便查找和冲突处理。

  **段管理**：`std::map<std::string, FLESection>` 按段名组织段数据及重定位信息。

  **程序头**：`std::vector<ProgramHeader>` 记录段的虚拟地址、大小和权限，为生成最终执行文件提供布局信息。

## Part B: 具体实现分析

### 符号解析实现
1. 如何处理不同类型的符号(全局/局部/弱符号)

   **全局符号**：

   - 遍历每个输入文件的符号表（`obj.symbols`），将全局符号添加到全局符号表（`global_symbols`）。

   - 对全局符号计算绝对地址，添加基址偏移：

     ```c++
     updated_symbol.offset += base_address;
     global_symbols[symbol.name] = updated_symbol;
     ```

   **局部符号**：

   - 局部符号不参与跨文件解析，仅在文件内部使用，直接忽略。

   **弱符号**：

   - 如果弱符号在全局符号表中已存在强符号，则忽略弱符号，否则将其添加到符号表。就是一系列的条件语句。

2. 如何解决符号冲突

   若一个符号在多个文件中被定义，则按以下规则处理：

   - 强符号覆盖弱符号

   - 如果存在多个强符号定义，则输出一个异常提示：

     ```c++
     if (existing_symbol.type == SymbolType::GLOBAL && symbol.section != existing_symbol.section) {
         throw std::runtime_error("Multiple definition of strong symbol: " + symbol.name);
     }
     ```

3. 实现中的关键优化

   提前检查是否需要解析符号，减少无效的遍历。虽然似乎意义不大，但GPT告诉我这么优化会有用，至少看起来确实有用。

4. 关键的错误处理，一些边界情况与 sanity check

  **未定义符号**：如果符号被引用但未定义，抛出异常：

  ```c++
  if (global_symbols.find(entry_symbol) == global_symbols.end()) {
      throw std::runtime_error("Undefined entry point: " + entry_symbol);
  }
  ```

  **符号表冲突**：检测并报告符号多定义的错误。

  **空文件处理**：对无符号或无段的输入文件正确跳过，避免空指针或未初始化访问。

### 重定位处理
1. 支持的重定位类型：

   **`R_X86_64_32`**：直接将符号的绝对地址填入目标位置。

   **`R_X86_64_32S`**：与 `R_X86_64_32` 类似，用于符号地址加偏移。

2. 重定位计算方法

   遍历所有合并段的重定位项。

   查找全局符号表中对应的符号，获取其绝对地址。

   根据重定位类型和偏移量（`reloc.addend`）计算目标值：

   ```c++
   relocation_value = symbol_address + reloc.addend;
   ```

   将计算结果以小端序形式写入段数据：

   （我觉得C++语法应该是这么写的，既然没报错我就假装是对的了）

   ```c++
   for (size_t i = 0; i < 4; ++i) {
       section.data[reloc.offset + i] = static_cast<uint8_t>(relocation_value >> (i * 8));
   }
   ```

3. 关键的错误处理

  **未定义符号**：若重定位项引用的符号在全局符号表中不存在，则抛出异常：

  ```c++
  if (global_symbols.find(reloc.symbol) == global_symbols.end()) {
      throw std::runtime_error("Undefined symbol: " + reloc.symbol);
  }
  ```

### 段合并策略
1. 如何组织和合并各类段

   用 `std::map<std::string, FLESection>` 按段名组织段数据：

   - 如果目标段不存在于 `merged_sections` 中，创建新的段。
   - 将输入段的数据通过 `std::vector::insert` 追加到目标段。
   - 合并重定位项（`relocs`），并调整每项的偏移量

2. 内存布局的考虑

   固定基址（`base_address`），例如 `0x400000`，统一从此地址开始布局。

   按段的大小逐步增加偏移量（`current_offset`），确保段不重叠：

   ```c++
   header.vaddr = base_address + current_offset;
   current_offset += section.data.size();
   ```

   （我认为测试过不了应该主要问题在这里，但不知道该咋改）

   

## Part C: 关键难点解决
- 没实现什么难点，感觉所有困难都能客服我，尽量写一点还有点成就感的：

  **重复符号处理**

  - **难点**：多个目标文件中定义了同名符号时，需要根据规则处理强弱符号冲突
  - 解决方案
    - 使用 `std::map<std::string, Symbol>` 存储全局符号表。
    - 在解析符号时，如果发现冲突：
      - 若当前符号为强符号，直接覆盖弱符号。
      - 若两个符号均为强符号，则抛出异常。
    - 这个部分应该是能够正常工作的

  **重定位越界检查**

  - **难点**：重定位项的偏移可能超出段数据范围

  - 解决方案

    - 在写入重定位值前，检查偏移量是否在段数据范围内：

      ```c++
      if (reloc.offset + 4 > section.data.size()) {
          throw std::runtime_error("Relocation offset out of bounds");
      }
      ```

还有一个重要成就，解决了一个不知道什么原因的SSH不能用的问题。原先可以通过SSH的方式push，今天不知道为什么不行了，最终尝试重设了一遍。

## Part D: 实验反馈

<!-- 芝士 5202 年研发的船新实验，你的反馈对我们至关重要
可以从实验设计，实验文档，框架代码三个方面进行反馈，具体衡量：
1. 实验设计：

   实验难度是否合适：写得一如既往地绝望，估计我的反馈不会很有参考价值qwq，又是一个只能写出第一题的lab啊（叹

   实验工作量是否合理：也许可以提供一个底层视角，我从尝试理解这个lab要我做什么，一直到重新在wsl上处理了一遍github和ssh的事情，再到研究C++，和ChatGPT大战三百回合，花了至少16个小时（CacheLab完全不会，这个还能赶得上，能写一点是一点），当个最大值吧，仅供师兄们参考

   是否让你更加理解链接器：多理解了一点点，难度还是远远超出我的理解范围了，以及由于完全不懂C++，前期花了非常多没意义的时间在尝试学习语法上，现在想来就该直接问GPT

2. 实验文档：文档是否清晰，哪些地方需要补充说明

3. 框架代码：我最后一次尝试时，代码框架是没有问题的。但是在先前尝试时，不知道因为什么原因，在`make test_1`之后一直要求我重新配置一个python环境。这个问题花掉了我大约两个小时，在删掉lab文件夹重新克隆一次后仍有这个问题，配置完python环境后好了；但是再克隆第三遍又没有这个问题，不明白是什么原因。 

## 参考资料 （可不填）
