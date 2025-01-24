// #include "fle.hpp"
// #include <iomanip>
// #include <iostream>

// void FLE_nm(const FLEObject& obj)
// {
//     // TODO: 实现符号表显示工具
//     // 1. 遍历所有符号
//     //    - 跳过未定义符号 (section为空的情况)
//     //    - 使用16进制格式显示符号地址

//     // 2. 确定符号类型字符
//     //    - 处理弱符号: 代码段用'W'，其他段用'V'
//     //    - 根据段类型(.text/.data/.bss/.rodata)和符号类型(GLOBAL/LOCAL)确定显示字符
//     //    - 全局符号用大写字母，局部符号用小写字母

//     // 3. 按格式输出
//     //    - [地址] [类型] [符号名]
//     //    - 地址使用16位十六进制，左侧补0

//     throw std::runtime_error("Not implemented");
// }

#include "fle.hpp"
#include <iomanip>
#include <iostream>
#include <stdexcept>

void FLE_nm(const FLEObject& obj)
{
    // 遍历符号表
    for (const auto& sym : obj.symbols) {
        // 1. 跳过未定义符号
        if (sym.section.empty()) {
            continue; // 未定义符号的 section 是空的
        }

        // 2. 计算符号地址
        size_t address = sym.offset; // 直接使用符号的 offset 作为地址

        // 3. 确定符号的类型字符
        char type_char;
        if (sym.type == SymbolType::WEAK) {
            // 弱符号
            if (sym.section == ".text") {
                type_char = 'W'; // 弱符号在代码段
            } else {
                type_char = 'V'; // 弱符号在其他段
            }
        } else {
            // 非弱符号，根据节类型和符号作用域确定
            if (sym.section == ".text") {
                type_char = (sym.type == SymbolType::GLOBAL) ? 'T' : 't';
            } else if (sym.section == ".data") {
                type_char = (sym.type == SymbolType::GLOBAL) ? 'D' : 'd';
            } else if (sym.section == ".bss") {
                type_char = (sym.type == SymbolType::GLOBAL) ? 'B' : 'b';
            } else if (sym.section == ".rodata") {
                type_char = (sym.type == SymbolType::GLOBAL) ? 'R' : 'r';
            } else {
                type_char = '?'; // 未知节类型
            }
        }

        // 4. 按格式输出符号信息
        std::cout << std::setw(16) << std::setfill('0') << std::hex << address << ' '
                  << type_char << ' ' << sym.name << std::endl;
    }
}