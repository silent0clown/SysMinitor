#include "registry_monitor.h"
#define NOMAXMIN
#include <windows.h>
#include <iostream>
#include <sstream>
#include <iomanip>

namespace sysmonitor {

RegistryMonitor::RegistryMonitor() {}

 std::string RegistryMonitor::WideToUTF8(const std::wstring& wideStr) {
        if (wideStr.empty()) return "";
        
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
        
        // 回退方案：过滤非ASCII字符
        std::string result;
        for (wchar_t wc : wideStr) {
            if (wc <= 0x7F && wc != 0) {
                result += static_cast<char>(wc);
            } else {
                result += '?'; // 替换无法转换的字符
            }
        }
        return result;
    }
    
    std::string RegistryMonitor::SanitizeUTF8(const std::string& str) {
        std::string result;
        for (size_t i = 0; i < str.length(); ) {
            unsigned char c = str[i];
            if (c <= 0x7F) {
                // ASCII字符，直接保留
                result += c;
                i++;
            } else if ((c & 0xE0) == 0xC0) {
                // 2字节UTF-8
                if (i + 1 < str.length() && (str[i+1] & 0xC0) == 0x80) {
                    result += str.substr(i, 2);
                    i += 2;
                } else {
                    result += '?';
                    i++;
                }
            } else if ((c & 0xF0) == 0xE0) {
                // 3字节UTF-8
                if (i + 2 < str.length() && (str[i+1] & 0xC0) == 0x80 && (str[i+2] & 0xC0) == 0x80) {
                    result += str.substr(i, 3);
                    i += 3;
                } else {
                    result += '?';
                    i++;
                }
            } else if ((c & 0xF8) == 0xF0) {
                // 4字节UTF-8
                if (i + 3 < str.length() && (str[i+1] & 0xC0) == 0x80 && (str[i+2] & 0xC0) == 0x80 && (str[i+3] & 0xC0) == 0x80) {
                    result += str.substr(i, 4);
                    i += 4;
                } else {
                    result += '?';
                    i++;
                }
            } else {
                // 无效的UTF-8字节
                result += '?';
                i++;
            }
        }
        return result;
    }

RegistrySnapshot RegistryMonitor::GetRegistrySnapshot() {
    RegistrySnapshot snapshot;
    snapshot.timestamp = GetTickCount64();
    
    try {
        snapshot.systemInfoKeys = GetSystemInfoKeys();
        snapshot.softwareKeys = GetInstalledSoftware();
        snapshot.networkKeys = GetNetworkConfig();
        
        for (auto& systenInfoKey : snapshot.systemInfoKeys) {
            systenInfoKey.convertToUTF8();
        }
        for (auto& softwareKey : snapshot.softwareKeys) {
            softwareKey.convertToUTF8();
        }
        for (auto& networkKey : snapshot.networkKeys) {
            networkKey.convertToUTF8();
        }
        std::cout << "获取注册表快照成功 - "
                  << "系统信息: " << snapshot.systemInfoKeys.size() << " 项, "
                  << "软件信息: " << snapshot.softwareKeys.size() << " 项, "
                  << "网络配置: " << snapshot.networkKeys.size() << " 项" << std::endl;
                  
    } catch (const std::exception& e) {
        std::cerr << "获取注册表快照时发生异常: " << e.what() << std::endl;
        // 确保返回一个有效的快照对象，即使部分数据获取失败
    }
    
    return snapshot;
}

std::vector<RegistryKey> RegistryMonitor::GetSystemInfoKeys() {
    std::vector<RegistryKey> keys;
    
    for (const auto& path : systemInfoPaths) {
        try {
            // 直接使用HKEY_LOCAL_MACHINE
            RegistryKey key = GetRegistryKey(HKEY_LOCAL_MACHINE, path);
            if (!key.values.empty() || !key.subkeys.empty()) {
                keys.push_back(key);
            }
            key.convertToUTF8();
        } catch (const std::exception& e) {
            std::cerr << "获取系统信息注册表失败: " << path << " - " << e.what() << std::endl;
        }
    }
    
    return keys;
}

std::vector<RegistryKey> RegistryMonitor::GetInstalledSoftware() {
    std::vector<RegistryKey> keys;
    
    for (const auto& path : softwarePaths) {
        try {
            // 直接使用HKEY_LOCAL_MACHINE
            RegistryKey key = GetRegistryKey(HKEY_LOCAL_MACHINE, path);
            key.convertToUTF8();
            if (!key.values.empty() || !key.subkeys.empty()) {
                keys.push_back(key);
            }
        } catch (const std::exception& e) {
            std::cerr << "获取软件信息注册表失败: " << path << " - " << e.what() << std::endl;
        }
    }
    
    // 获取当前用户的软件信息 - 使用HKEY_CURRENT_USER
    try {
        RegistryKey userKey = GetRegistryKey(HKEY_CURRENT_USER, 
                                           "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall");
        userKey.convertToUTF8();
        if (!userKey.values.empty() || !userKey.subkeys.empty()) {

            keys.push_back(userKey);
        }
    } catch (const std::exception& e) {
        std::cerr << "获取用户软件信息注册表失败: " << e.what() << std::endl;
    }
    
    return keys;
}

std::vector<RegistryKey> RegistryMonitor::GetNetworkConfig() {
    std::vector<RegistryKey> keys;
    
    for (const auto& path : networkPaths) {
        try {
            // 直接使用HKEY_LOCAL_MACHINE
            RegistryKey key = GetRegistryKey(HKEY_LOCAL_MACHINE, path);
            key.convertToUTF8();
            if (!key.values.empty() || !key.subkeys.empty()) {
                keys.push_back(key);
            }
        } catch (const std::exception& e) {
            std::cerr << "获取网络配置注册表失败: " << path << " - " << e.what() << std::endl;
        }
    }
    
    return keys;
}

RegistryKey RegistryMonitor::GetRegistryKey(HKEY root, const std::string& subKey) {
    RegistryKey key;
    key.path = HKEYToString(root) + "\\" + subKey;
    
    HKEY hKey;
    if (!SafeOpenKey(root, subKey, hKey)) {
        return key;
    }
    
    // 枚举值
    DWORD valueCount, maxValueNameLen, maxValueDataLen;
    LONG result = RegQueryInfoKeyA(hKey, NULL, NULL, NULL, NULL, NULL, NULL,
                                 &valueCount, &maxValueNameLen, &maxValueDataLen,
                                 NULL, NULL);
    
    if (result == ERROR_SUCCESS && valueCount > 0) {
        for (DWORD i = 0; i < valueCount; i++) {
            std::vector<char> valueName(maxValueNameLen + 1, 0);
            std::vector<BYTE> valueData;
            DWORD valueNameSize = maxValueNameLen + 1;
            DWORD valueDataSize = 0;
            DWORD valueType;
            
            // 第一次调用获取数据大小
            result = RegEnumValueA(hKey, i, valueName.data(), &valueNameSize,
                                 NULL, &valueType, NULL, &valueDataSize);
            
            if (result == ERROR_SUCCESS) {
                try {
                    valueData.resize(valueDataSize + 2, 0); // 额外空间确保安全
                    result = RegEnumValueA(hKey, i, valueName.data(), &valueNameSize,
                                         NULL, &valueType, valueData.data(), &valueDataSize);
                    
                    if (result == ERROR_SUCCESS) {
                        RegistryValue value;
                        value.name = (valueName[0] == '\0') ? "(Default)" : SanitizeUTF8(valueName.data());
                        value.type = RegistryTypeToString(valueType);
                        value.data = RegistryDataToString(valueData.data(), valueDataSize, valueType);
                        value.size = valueDataSize;
                        
                        key.values.push_back(value);
                    }
                } catch (const std::exception& e) {
                    std::cerr << "处理注册表值时发生异常: " << e.what() << std::endl;
                    // 跳过有问题的值，继续处理其他值
                }
            }
        }
    }
    
    // 枚举子键（保持不变）
    DWORD subkeyCount, maxSubkeyLen;
    result = RegQueryInfoKeyA(hKey, NULL, NULL, NULL, &subkeyCount, &maxSubkeyLen,
                            NULL, NULL, NULL, NULL, NULL, NULL);
    
    if (result == ERROR_SUCCESS && subkeyCount > 0) {
        std::vector<char> subkeyName(maxSubkeyLen + 1, 0);
        
        for (DWORD i = 0; i < subkeyCount; i++) {
            DWORD subkeyNameSize = maxSubkeyLen + 1;
            result = RegEnumKeyExA(hKey, i, subkeyName.data(), &subkeyNameSize,
                                 NULL, NULL, NULL, NULL);
            
            if (result == ERROR_SUCCESS) {
                key.subkeys.push_back(SanitizeUTF8(subkeyName.data()));
            }
        }
    }
    
    SafeCloseKey(hKey);
    return key;
}

std::string RegistryMonitor::HKEYToString(HKEY hkey) {
    if (hkey == HKEY_CLASSES_ROOT)    return "HKEY_CLASSES_ROOT";
    if (hkey == HKEY_CURRENT_USER)    return "HKEY_CURRENT_USER";
    if (hkey == HKEY_LOCAL_MACHINE)   return "HKEY_LOCAL_MACHINE";
    if (hkey == HKEY_USERS)           return "HKEY_USERS";
    if (hkey == HKEY_CURRENT_CONFIG)  return "HKEY_CURRENT_CONFIG";
    return "UNKNOWN";
}

std::string RegistryMonitor::RegistryTypeToString(DWORD type) {
    switch (type) {
        case REG_SZ:        return "STRING";
        case REG_EXPAND_SZ: return "EXPAND_SZ";
        case REG_BINARY:    return "BINARY";
        case REG_DWORD:     return "DWORD";
        // case REG_DWORD_LITTLE_ENDIAN: return "DWORD";
        case REG_DWORD_BIG_ENDIAN:    return "DWORD_BE";
        case REG_LINK:      return "LINK";
        case REG_MULTI_SZ:  return "MULTI_SZ";
        case REG_QWORD:     return "QWORD";
        default:            return "UNKNOWN";
    }
}

std::string RegistryMonitor::RegistryDataToString(const BYTE* data, DWORD size, DWORD type) {
    if (data == nullptr || size == 0) return "";
    
    try {
        switch (type) {
            case REG_SZ:
            case REG_EXPAND_SZ: {
                // 安全处理字符串数据
                if (size == 0) return "";
                
                const char* strData = reinterpret_cast<const char*>(data);
                std::string result(strData, strnlen(strData, size));
                return SanitizeUTF8(result);
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
                
            case REG_MULTI_SZ: {
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
                        result += SanitizeUTF8(utf8Str);
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
                
            case REG_BINARY: {
                std::stringstream ss;
                ss << std::hex << std::setfill('0');
                DWORD outputSize = (size < 16) ? size : 16; // 限制输出长度
                for (DWORD i = 0; i < outputSize; i++) {
                    ss << std::setw(2) << static_cast<int>(data[i]) << " ";
                }
                if (size > 16) ss << "...";
                return ss.str();
            }
                
            default:
                return "[Unsupported Type]";
        }
    } catch (const std::exception& e) {
        std::cerr << "处理注册表数据时发生异常: " << e.what() << std::endl;
        return "[Error Processing Data]";
    }
    
    return "[Binary Data]";
}

bool RegistryMonitor::SafeOpenKey(HKEY root, const std::string& subKey, HKEY& hKey) {
    LONG result = RegOpenKeyExA(root, subKey.c_str(), 0, KEY_READ, &hKey);
    
    if (result != ERROR_SUCCESS) {
        std::cerr << "无法打开注册表键: " << HKEYToString(root) << "\\" << subKey 
                  << " - 错误: " << result << std::endl;
        return false;
    }
    
    return true;
}

void RegistryMonitor::SafeCloseKey(HKEY hKey) {
    if (hKey) {
        RegCloseKey(hKey);
    }
}

bool RegistryMonitor::SafeQueryValue(const std::string& fullPath, RegistryValue& value) {
    // 解析完整路径
    size_t pos = fullPath.find('\\');
    if (pos == std::string::npos) {
        return false;
    }
    
    std::string rootStr = fullPath.substr(0, pos);
    std::string restPath = fullPath.substr(pos + 1);
    
    // 转换根键
    HKEY hRoot;
    if (rootStr == "HKEY_LOCAL_MACHINE") hRoot = HKEY_LOCAL_MACHINE;
    else if (rootStr == "HKEY_CURRENT_USER") hRoot = HKEY_CURRENT_USER;
    else if (rootStr == "HKEY_CLASSES_ROOT") hRoot = HKEY_CLASSES_ROOT;
    else if (rootStr == "HKEY_USERS") hRoot = HKEY_USERS;
    else if (rootStr == "HKEY_CURRENT_CONFIG") hRoot = HKEY_CURRENT_CONFIG;
    else return false;
    
    // 分离键路径和值名称
    pos = restPath.find_last_of('\\');
    if (pos == std::string::npos) {
        return false;
    }
    
    std::string keyPath = restPath.substr(0, pos);
    std::string valueName = restPath.substr(pos + 1);
    
    HKEY hKey;
    if (RegOpenKeyExA(hRoot, keyPath.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return false;
    }
    
    DWORD type, size;
    if (RegQueryValueExA(hKey, valueName.c_str(), NULL, &type, NULL, &size) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return false;
    }
    
    std::vector<BYTE> data(size);
    if (RegQueryValueExA(hKey, valueName.c_str(), NULL, &type, data.data(), &size) == ERROR_SUCCESS) {
        value.name = valueName;
        value.type = RegistryTypeToString(type);
        value.data = RegistryDataToString(data.data(), size, type);
        value.size = size;
    }
    
    RegCloseKey(hKey);
    return true;
}

} // namespace sysmonitor