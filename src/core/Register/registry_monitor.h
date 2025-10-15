// registry_monitor.h
#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <map>
#include <windows.h>
#include "../../utils/encode.h"

namespace sysmonitor {

struct RegistryValue {
    std::string name;
    std::string type;
    std::string data;
    uint32_t size;

    bool operator==(const RegistryValue& other) const {
        return name == other.name &&
               type == other.type &&
               data == other.data &&
               size == other.size;
    }
    
    bool operator!=(const RegistryValue& other) const {
        return !(*this == other);
    }

    RegistryValue& convertToUTF8() {
        name = util::EncodingUtil::SafeString(name);
        type = util::EncodingUtil::SafeString(type);
        data = util::EncodingUtil::SafeString(data);
        return *this;
    }
};

struct RegistryKey {
    std::string path;
    std::vector<RegistryValue> values;
    std::vector<std::string> subkeys;
    uint64_t lastModified;
    std::string category;  // 分类标识
    std::string autoStartType;  // 新增：自启动项类型

    bool operator==(const RegistryKey& other) const {
        return path == other.path &&
               values == other.values &&
               subkeys == other.subkeys &&
               category == other.category &&
               autoStartType == other.autoStartType;
    }
    
    bool operator!=(const RegistryKey& other) const {
        return !(*this == other);
    }

    RegistryKey& convertToUTF8() {
        path = util::EncodingUtil::SafeString(path);
        category = util::EncodingUtil::SafeString(category);
        autoStartType = util::EncodingUtil::SafeString(autoStartType);
        
        for (auto& value : values) {
            value.convertToUTF8();
        }
        
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
    std::vector<RegistryKey> autoStartKeys;
    uint64_t timestamp;
};

class RegistryMonitor {
public:
    RegistryMonitor();
    
    RegistrySnapshot GetRegistrySnapshot();
    RegistryKey GetRegistryKey(HKEY root, const std::string& subKey);
    std::vector<RegistryKey> GetSystemInfoKeys();
    std::vector<RegistryKey> GetInstalledSoftware();
    std::vector<RegistryKey> GetNetworkConfig();
    std::vector<RegistryKey> GetAutoStartItems();
    bool SafeQueryValue(const std::string& fullPath, RegistryValue& value);

private:
    // 私有辅助方法
    RegistryKey FilterAutoStartServices(const RegistryKey& servicesKey);
    std::string GetAutoStartType(HKEY root, const std::string& path);
    
    std::string WideToUTF8(const std::wstring& wideStr);
    std::string SanitizeUTF8(const std::string& str);
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
    
    // 自启动项路径
    const std::vector<std::pair<HKEY, std::string>> autoStartPaths = {
        {HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"},
        {HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce"},
        {HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"},
        {HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce"},
        {HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnceEx"},
        {HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services"},
        {HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\TaskCache\\Tree"},
        {HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Browser Helper Objects"},
        {HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\UserInit"},
        {HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\Shell"},
        {HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\Run"},
        {HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\Run"},
        {HKEY_LOCAL_MACHINE, "SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Run"},
        {HKEY_LOCAL_MACHINE, "SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\RunOnce"}
    };
};

} // namespace sysmonitor