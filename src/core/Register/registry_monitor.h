#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <map>
#include <windows.h>  // 确保包含windows.h以使用原有的HKEY定义
#include "../../utils/encode.h"

namespace sysmonitor {

// 删除这个enum class定义，直接使用Windows的HKEY类型
// enum class RegistryRoot {
//     HKEY_CLASSES_ROOT,
//     HKEY_CURRENT_USER,
//     HKEY_LOCAL_MACHINE,
//     HKEY_USERS,
//     HKEY_CURRENT_CONFIG
// };

struct RegistryValue {
    std::string name;
    std::string type;  // "STRING", "DWORD", "QWORD", "BINARY", "MULTI_SZ"
    std::string data;
    uint32_t size;

    // 添加相等比较运算符
    bool operator==(const RegistryValue& other) const {
        return name == other.name &&
               type == other.type &&
               data == other.data &&
               size == other.size;
    }
    
    // 添加不相等比较运算符（可选，但推荐）
    bool operator!=(const RegistryValue& other) const {
        return !(*this == other);
    }

    RegistryValue& convertToUTF8() {
        name = util::EncodingUtil::ToUTF8(name);
        type = util::EncodingUtil::ToUTF8(type);
        data = util::EncodingUtil::ToUTF8(data);
        return *this;
    }
};

struct RegistryKey {
    std::string path;
    std::vector<RegistryValue> values;
    std::vector<std::string> subkeys;
    uint64_t lastModified;

    // 添加相等比较运算符
    bool operator==(const RegistryKey& other) const {
        return path == other.path &&
               values == other.values &&
               subkeys == other.subkeys;
    }
    
    // 添加不相等比较运算符（可选，但推荐）
    bool operator!=(const RegistryKey& other) const {
        return !(*this == other);
    }

    RegistryKey& convertToUTF8() {
        path = util::EncodingUtil::SafeString(path);
        
        // 转换所有值
        for (auto& value : values) {
            value.convertToUTF8();
        }
        
        // 转换所有子键名称
        for (auto& subkey : subkeys) {
            subkey = util::EncodingUtil::SafeString(subkey);
        }
        
        return *this;
    }
};

struct RegistrySnapshot {
    std::vector<RegistryKey> systemInfoKeys;
    std::vector<RegistryKey> softwareKeys;
    std::vector<RegistryKey> networkKeys;
    uint64_t timestamp;
};

class RegistryMonitor {
public:
    RegistryMonitor();
    
    // 获取注册表快照
    RegistrySnapshot GetRegistrySnapshot();
    
    // 获取特定注册表项 - 修改参数类型为HKEY
    RegistryKey GetRegistryKey(HKEY root, const std::string& subKey);
    
    // 获取系统信息相关的注册表
    std::vector<RegistryKey> GetSystemInfoKeys();
    
    // 获取已安装软件信息
    std::vector<RegistryKey> GetInstalledSoftware();
    
    // 获取网络配置信息
    std::vector<RegistryKey> GetNetworkConfig();
    
    // 安全的注册表查询（带错误处理）
    bool SafeQueryValue(const std::string& fullPath, RegistryValue& value);

private:
    // 删除ConvertRootToHKEY函数，直接使用HKEY
    // HKEY ConvertRootToHKEY(RegistryRoot root);
    
    std::string HKEYToString(HKEY hkey);
    std::string RegistryTypeToString(DWORD type);
    std::string RegistryDataToString(const BYTE* data, DWORD size, DWORD type);
    
    std::string WideToUTF8(const std::wstring& wideStr);
    std::string SanitizeUTF8(const std::string& str);
    // 安全的注册表操作 - 修改参数类型为HKEY
    bool SafeOpenKey(HKEY root, const std::string& subKey, HKEY& hKey);
    void SafeCloseKey(HKEY hKey);
    
    // 预定义的关注路径
    const std::vector<std::string> systemInfoPaths = {
        "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
        "HARDWARE\\DESCRIPTION\\System",
        "SYSTEM\\CurrentControlSet\\Control\\ComputerName\\ComputerName",
        "SYSTEM\\CurrentControlSet\\Control\\TimeZoneInformation"
    };
    
    const std::vector<std::string> softwarePaths = {
        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce"
    };
    
    const std::vector<std::string> networkPaths = {
        "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters",
        "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces",
        "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkList\\Signatures\\Unmanaged"
    };
};

} // namespace sysmonitor