#include "fle.hpp"
#include <iomanip>
#include <iostream>

void FLE_nm(const FLEObject& obj)
{
    // TODO: 实现符号表显示工具
    // 1. 遍历所有符号
    //    - 处理未定义符号 (section为空的情况)
    //    - 使用16进制格式显示符号地址

    // 2. 确定符号类型字符
    //    - 处理弱符号: 代码段用'W'，其他段用'V'
    //    - 根据段类型(.text/.data/.bss/.rodata)和符号类型(GLOBAL/LOCAL)确定显示字符
    //    - 全局符号用大写字母，局部符号用小写字母

    // 3. 按格式输出
    //    - [地址] [类型] [符号名]
    //    - 地址使用16位十六进制，左侧补0

    throw std::runtime_error("Not implemented");
}