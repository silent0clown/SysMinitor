// registry_encoding_utils.cpp
#include "registry_encode.h"
#include <iostream>

namespace sysmonitor {
namespace encoding {

std::string RegistryEncodingUtils::HKEYToString(HKEY hkey) {
    if (hkey == HKEY_CLASSES_ROOT)    return "HKEY_CLASSES_ROOT";
    if (hkey == HKEY_CURRENT_USER)    return "HKEY_CURRENT_USER";
    if (hkey == HKEY_LOCAL_MACHINE)   return "HKEY_LOCAL_MACHINE";
    if (hkey == HKEY_USERS)           return "HKEY_USERS";
    if (hkey == HKEY_CURRENT_CONFIG)  return "HKEY_CURRENT_CONFIG";
    return "UNKNOWN";
}

std::string RegistryEncodingUtils::RegistryTypeToString(DWORD type) {
    switch (type) {
        case REG_SZ:        return "STRING";
        case REG_EXPAND_SZ: return "EXPAND_SZ";
        case REG_BINARY:    return "BINARY";
        case REG_DWORD:     return "DWORD";
        case REG_DWORD_BIG_ENDIAN: return "DWORD_BE";
        case REG_LINK:      return "LINK";
        case REG_MULTI_SZ:  return "MULTI_SZ";
        case REG_QWORD:     return "QWORD";
        default:            return "UNKNOWN";
    }
}

std::string RegistryEncodingUtils::RegistryDataToString(const BYTE* data, DWORD size, DWORD type) {
    if (data == nullptr || size == 0) return "";
    
    try {
        switch (type) {
            case REG_SZ:
            case REG_EXPAND_SZ: {
                if (size == 0) return "";
                const char* strData = reinterpret_cast<const char*>(data);
                std::string result(strData, strnlen(strData, size));
                return SafeString(result);
            }
                
            case REG_DWORD:
                if (size >= sizeof(DWORD)) {
                    DWORD value = *reinterpret_cast<const DWORD*>(data);
                    return std::to_string(value);
                }
                break;
                
            case REG_QWORD:
                if (size >= sizeof(uint64_t)) {
                    uint64_t value = *reinterpret_cast<const uint64_t*>(data);
                    return std::to_string(value);
                }
                break;
                
            case REG_MULTI_SZ:
                return ProcessMultiSZ(data, size);
                
            case REG_BINARY:
                return ProcessBinaryData(data, size);
                
            default:
                return "[Unsupported Type]";
        }
    } catch (const std::exception& e) {
        std::cerr << "处理注册表数据时发生异常: " << e.what() << std::endl;
        return "[Error Processing Data]";
    }
    
    return "[Binary Data]";
}

std::string RegistryEncodingUtils::ProcessMultiSZ(const BYTE* data, DWORD size) {
    std::string result;
    const wchar_t* wstr = reinterpret_cast<const wchar_t*>(data);
    size_t totalChars = size / sizeof(wchar_t);
    
    for (size_t i = 0; i < totalChars && wstr[i]; ) {
        size_t len = 0;
        while (i + len < totalChars && wstr[i + len] != L'\0') {
            len++;
        }
        
        if (len > 0) {
            std::wstring wideSubStr(wstr + i, len);
            std::string utf8Str = WideToUTF8(wideSubStr);
            result += SafeString(utf8Str);
            result += "; ";
            i += len + 1;
        } else {
            break;
        }
    }
    
    if (!result.empty() && result.length() >= 2) {
        result = result.substr(0, result.length() - 2);
    }
    return result;
}

std::string RegistryEncodingUtils::ProcessBinaryData(const BYTE* data, DWORD size) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    DWORD outputSize = (size < 16) ? size : 16; // 限制输出长度
    for (DWORD i = 0; i < outputSize; i++) {
        ss << std::setw(2) << static_cast<int>(data[i]) << " ";
    }
    if (size > 16) ss << "...";
    return ss.str();
}

std::string RegistryEncodingUtils::WideToUTF8(const std::wstring& wideStr) {
    if (wideStr.empty()) return "";
    
#ifdef _WIN32
    try {
        int utf8Length = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (utf8Length > 0) {
            std::vector<char> buffer(utf8Length);
            WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, buffer.data(), utf8Length, nullptr, nullptr);
            return std::string(buffer.data());
        }
    } catch (...) {
        // 转换失败时的回退处理
    }
#endif
    
    // 回退方案：使用现有的工具或简单转换
    std::string result;
    for (wchar_t wc : wideStr) {
        if (wc <= 0x7F && wc != 0) {
            result += static_cast<char>(wc);
        } else {
            result += '?'; // 替换无法转换的字符
        }
    }
    return StringUtil::SafeString(result);
}

std::string RegistryEncodingUtils::SafeString(const std::string& str) {
    return StringUtil::SafeString(str);
}

std::string RegistryEncodingUtils::SanitizeUTF8(const std::string& str) {
    return StringUtil::SanitizeString(str);
}

} // namespace encoding
} // namespace sysmonitor