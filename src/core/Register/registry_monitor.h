// registry_monitor.h
#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <map>
#include <windows.h>
#include "../../utils/encode.h"
#include <iostream>
#include <fstream>
#include <set>
#include <algorithm>
#include <sstream>
#include <codecvt>
#include <locale>
#include <iomanip>

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
    std::string category;  // Category identifier
    std::string autoStartType;  // New: auto-start item type

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

//added by Li Yongheng 20251017
// 注册表文件信息结构
struct RegFileInfo {
    std::string fileName;              // 文件名
    long long fileSize;           // 文件大小（字节）
};

// 备份文件夹信息结构
struct BackupInfo {
    std::string folderName;                    // 文件夹名称
    std::string folderPath;                    // 文件夹完整路径
    time_t createTime;                    // 创建时间
    std::vector<RegFileInfo> regFiles;         // 该文件夹下的注册表文件列表
    long long totalSize;                  // 文件夹中所有注册表文件的总大小
};

struct RegistrySnapshot {
    // std::vector<RegistryKey> systemInfoKeys;
    // std::vector<RegistryKey> softwareKeys;
    // std::vector<RegistryKey> networkKeys;
    // std::vector<RegistryKey> autoStartKeys;
    BackupInfo backupInfo;
    uint64_t timestamp;
};
struct RegistryVal {
    std::string name;
    std::string type;
    std::string data;

    bool operator==(const RegistryVal& other) const {
        return name == other.name && type == other.type && data == other.data;
    }

    bool operator<(const RegistryVal& other) const {
        if (name != other.name) return name < other.name;
        if (type != other.type) return type < other.type;
        return data < other.data;
    }
};

struct RegistryKy {
    std::string path;
    std::map<std::string, RegistryVal> values;

    bool operator==(const RegistryKy& other) const {
        return values == other.values;
    }
};

struct FileInfo {
    std::string path;
    std::string filename;
    std::string prefix;
};

// 定义比较结果结构
struct FileComparisonResult {
    std::string file1;
    std::string file2;
    std::vector<std::string> addedKeys;
    std::vector<std::string> removedKeys;
    std::vector<std::string> modifiedKeys;

    // 添加每个键的详细变化
    std::map<std::string, std::pair<std::string, std::string>> keyChanges; // 键路径 -> (旧值, 新值)
};

struct FolderComparisonResult {
    std::string folder1;
    std::string folder2;
    std::vector<std::string> filesOnlyInFolder1; // 只在文件夹1中的文件
    std::vector<std::string> filesOnlyInFolder2; // 只在文件夹2中的文件
    std::map<std::string, FileComparisonResult> comparedFiles; // 前缀 -> 文件比较结果

    // 统计信息
    int totalComparedFiles;
    int totalAddedKeys;
    int totalRemovedKeys;
    int totalModifiedKeys;
};
//

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

    //added by Li Yongheng 20251017
    int SaveReg();
    
    std::map<std::string, BackupInfo> GetAllBackupInfo();
    void PrintBackupInfo(const std::map<std::string, BackupInfo>& backupInfo);
    
    FolderComparisonResult compareFolders(const std::string& relativeFolder1, const std::string& relativeFolder2);
    FileComparisonResult compare(const std::string& file1, const std::string& file2);
    //

private:
    // Private helper methods
    RegistryKey FilterAutoStartServices(const RegistryKey& servicesKey);
    std::string GetAutoStartType(HKEY root, const std::string& path);
    
    std::string WideToUTF8(const std::wstring& wideStr);
    std::string SanitizeUTF8(const std::string& str);
    bool SafeOpenKey(HKEY root, const std::string& subKey, HKEY& hKey);
    void SafeCloseKey(HKEY hKey);
    
    // Predefined paths of interest
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
    
    // Auto-start item paths
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

    //added by Li Yongheng 20251017
    BOOL ExportRegistryWithTimestamp(LPCSTR registryKey, LPCSTR backupDir, int callCount);
    BOOL SaveRegistryWithRegExe(LPCSTR registryKey, LPCSTR outputFile);
    void GetCurrentTimestamp(char* timestamp, size_t bufferSize);
    BOOL CombinePath(char* fullPath, size_t bufferSize, LPCSTR dir, LPCSTR fileName);
    void GenerateRegistryName(LPCSTR registryKey, char* regName, size_t bufferSize);
    BOOL GetExeDirectory(char* exeDir, size_t bufferSize);
    
    BOOL CreateBackupDirectory(LPCSTR baseDir, char* backupDir, size_t bufferSize);
    std::string FormatFileSize(long long fileSize);
    
    std::vector<FileInfo> getRegFiles(const std::string& folder);
    std::string extractPrefix(const std::string& filename);
    std::map<std::string, FileInfo> groupFilesByPrefix(const std::vector<FileInfo>& files);
    std::string readRegFile(const std::string& filename);
    std::map<std::string, RegistryKy> parseRegFile(const std::string& filename);
    RegistryVal parseValueLine(const std::string& line);
    FileComparisonResult findDifferences(const std::map<std::string, RegistryKy>& reg1,const std::map<std::string, RegistryKy>& reg2);
    void printResults(const FileComparisonResult& result);
    void compareKeyValues(const RegistryKy& key1, const RegistryKy& key2,std::map<std::string, std::pair<std::string, std::string>>& keyChanges);
    
    //
};

} // namespace sysmonitor