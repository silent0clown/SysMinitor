// registry_encoding_utils.h
#pragma once
#include <string>
#include <vector>
#include <windows.h>
#include <sstream>
#include <iomanip>
#include "encode.h"

namespace sysmonitor {
namespace encoding {

class RegistryEncodingUtils {
public:
    // 使用现有的 EncodingUtil
    using StringUtil = util::EncodingUtil;

    // HKEY转字符串
    static std::string HKEYToString(HKEY hkey);
    
    // 注册表数据类型转字符串
    static std::string RegistryTypeToString(DWORD type);
    
    // 注册表数据转字符串（使用安全的UTF-8转换）
    static std::string RegistryDataToString(const BYTE* data, DWORD size, DWORD type);
    
    // 宽字符串转UTF-8（使用现有的工具）
    static std::string WideToUTF8(const std::wstring& wideStr);
    
    // 安全的字符串处理
    static std::string SafeString(const std::string& str);
    
    // 清理和验证UTF-8字符串
    static std::string SanitizeUTF8(const std::string& str);

private:
    // 处理多字符串类型
    static std::string ProcessMultiSZ(const BYTE* data, DWORD size);
    
    // 处理二进制数据
    static std::string ProcessBinaryData(const BYTE* data, DWORD size);
};

} // namespace encoding
} // namespace sysmonitor