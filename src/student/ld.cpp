// #include "fle.hpp"
// #include <cassert>
// #include <iostream>
// #include <map>
// #include <stdexcept>
// #include <vector>

// FLEObject FLE_ld(const std::vector<FLEObject>& objects)
// {
//     // TODO: 实现链接器
//     // 1. 收集和合并段
//     //    - 遍历所有输入对象的段
//     //    - 按段名分组并计算偏移量
//     //    - 设置段的属性（读/写/执行）

//     // 2. 处理符号
//     //    - 收集所有全局符号和局部符号
//     //    - 处理符号冲突（强符号/弱符号）

//     // 3. 重定位
//     //    - 遍历所有重定位项
//     //    - 计算并填充重定位值
//     //    - 注意不同重定位类型的处理

//     // 4. 生成可执行文件
//     //    - 设置程序入口点（_start）
//     //    - 确保所有必要的段都已正确设置

//     throw std::runtime_error("Not implemented");
// }

#include "fle.hpp"
#include <cassert>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

FLEObject FLE_ld(const std::vector<FLEObject>& objects)
{
    // 记录每个段的合并内容和布局
    std::map<std::string, FLESection> merged_sections; // 按段名分组
    size_t current_offset = 0; // 跟踪当前偏移量

    // 符号表
    std::map<std::string, Symbol> global_symbols; // 全局符号

    // 入口点符号
    std::string entry_symbol = "_start";
    size_t entry_address = 0;

    // 程序头表
    std::vector<ProgramHeader> program_headers;

    // 第一次扫描：合并所有段
    for (const auto& obj : objects) {
        for (const auto& [section_name, section] : obj.sections) {
            // 如果段不存在，则创建新段
            if (merged_sections.find(section_name) == merged_sections.end()) {
                merged_sections[section_name] = FLESection{
                    .name = section_name, 
                    .data = {},
                    .relocs = {},
                    .has_symbols = section.has_symbols
                };
            }

            // 合并段数据
            auto& merged_section = merged_sections[section_name];
            size_t section_start = merged_section.data.size();

            // 复制数据
            merged_section.data.insert(
                merged_section.data.end(),
                section.data.begin(),
                section.data.end()
            );

            // 更新重定位项：调整偏移量以反映新位置
            for (const auto& reloc : section.relocs) {
                Relocation updated_reloc = reloc;
                updated_reloc.offset += section_start; // 调整重定位项偏移
                merged_section.relocs.push_back(updated_reloc);
            }
        }
    }

    // 第二次扫描：收集并解析符号
    for (const auto& obj : objects) {
        for (const auto& symbol : obj.symbols) {
            if (symbol.type == SymbolType::GLOBAL) {
                if (global_symbols.find(symbol.name) == global_symbols.end()) {
                    // 如果是全局符号且未定义，添加到符号表
                    global_symbols[symbol.name] = symbol;
                } else {
                    // 处理符号冲突：强符号必须唯一
                    const auto& existing_symbol = global_symbols[symbol.name];
                    if (existing_symbol.type == SymbolType::GLOBAL &&
                        symbol.section != existing_symbol.section) {
                        throw std::runtime_error("Multiple definition of strong symbol: " + symbol.name);
                    }
                }
            }
        }
    }

    // 检查入口点符号
    if (global_symbols.find(entry_symbol) == global_symbols.end()) {
        throw std::runtime_error("Undefined entry point: " + entry_symbol);
    }
    entry_address = global_symbols[entry_symbol].offset;

    // 第三次扫描：重定位处理
    for (auto& [section_name, section] : merged_sections) {
        for (const auto& reloc : section.relocs) {
            if (global_symbols.find(reloc.symbol) == global_symbols.end()) {
                throw std::runtime_error("Undefined symbol: " + reloc.symbol);
            }

            // 获取符号的绝对地址
            size_t symbol_address = global_symbols[reloc.symbol].offset;

            // 计算重定位值
            size_t relocation_value = 0;
            switch (reloc.type) {
                case RelocationType::R_X86_64_32:
                case RelocationType::R_X86_64_32S:
                    relocation_value = symbol_address + reloc.addend;
                    break;
                default:
                    throw std::runtime_error("Unsupported relocation type");
            }

            // 写入重定位值（假设小端序）
            for (size_t i = 0; i < 4; ++i) {
                section.data[reloc.offset + i] = static_cast<uint8_t>(relocation_value >> (i * 8));
            }
        }
    }
    // 创建程序头
size_t base_address = 0x400000; // 固定基址
for (const auto& [section_name, section] : merged_sections) {
    ProgramHeader header = {
        .name = section_name,
        .vaddr = base_address + current_offset, // 段的虚拟地址
        .size = section.data.size(),           // 段大小
        .flags = static_cast<uint32_t>(
            static_cast<std::underlying_type<PHF>::type>(PHF::R) |
            static_cast<std::underlying_type<PHF>::type>(PHF::W) |
            static_cast<std::underlying_type<PHF>::type>(PHF::X)
        ) // 可读、可写、可执行
    };
    program_headers.push_back(header);
    current_offset += section.data.size();
}



    // 创建最终的 FLEObject
    FLEObject exe_object;
    exe_object.type = ".exe"; // 标记为可执行文件
    exe_object.sections = std::move(merged_sections); // 设置合并后的段
    exe_object.symbols = {}; // 未来可以添加解析后的符号表（如需要）
    exe_object.entry = entry_address; // 设置程序入口点
    exe_object.phdrs = std::move(program_headers); // 设置程序头

    return exe_object;
}
