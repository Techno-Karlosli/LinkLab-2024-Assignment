#include "fle.hpp"
#include <cassert>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

FLEObject FLE_ld(const std::vector<FLEObject>& objects)
{
    // TODO: 实现链接器
    // 1. 收集和合并段
    //    - 遍历所有输入对象的段
    //    - 按段名分组并计算偏移量
    //    - 设置段的属性（读/写/执行）

    // 2. 处理符号
    //    - 收集所有全局符号和局部符号
    //    - 处理符号冲突（强符号/弱符号）

    // 3. 重定位
    //    - 遍历所有重定位项
    //    - 计算并填充重定位值
    //    - 注意不同重定位类型的处理

    // 4. 生成可执行文件
    //    - 设置程序入口点（_start）
    //    - 确保所有必要的段都已正确设置

    throw std::runtime_error("Not implemented");
}