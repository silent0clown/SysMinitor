#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <map>
#include <windows.h>  // ȷ������windows.h��ʹ��ԭ�е�HKEY����

namespace sysmonitor {

// ɾ�����enum class���壬ֱ��ʹ��Windows��HKEY����
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

    // �����ȱȽ������
    bool operator==(const RegistryValue& other) const {
        return name == other.name &&
               type == other.type &&
               data == other.data &&
               size == other.size;
    }
    
    // ��Ӳ���ȱȽ����������ѡ�����Ƽ���
    bool operator!=(const RegistryValue& other) const {
        return !(*this == other);
    }
};

struct RegistryKey {
    std::string path;
    std::vector<RegistryValue> values;
    std::vector<std::string> subkeys;
    uint64_t lastModified;

    // �����ȱȽ������
    bool operator==(const RegistryKey& other) const {
        return path == other.path &&
               values == other.values &&
               subkeys == other.subkeys;
    }
    
    // ��Ӳ���ȱȽ����������ѡ�����Ƽ���
    bool operator!=(const RegistryKey& other) const {
        return !(*this == other);
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
    
    // ��ȡע������
    RegistrySnapshot GetRegistrySnapshot();
    
    // ��ȡ�ض�ע����� - �޸Ĳ�������ΪHKEY
    RegistryKey GetRegistryKey(HKEY root, const std::string& subKey);
    
    // ��ȡϵͳ��Ϣ��ص�ע���
    std::vector<RegistryKey> GetSystemInfoKeys();
    
    // ��ȡ�Ѱ�װ�����Ϣ
    std::vector<RegistryKey> GetInstalledSoftware();
    
    // ��ȡ����������Ϣ
    std::vector<RegistryKey> GetNetworkConfig();
    
    // ��ȫ��ע����ѯ����������
    bool SafeQueryValue(const std::string& fullPath, RegistryValue& value);

private:
    // ɾ��ConvertRootToHKEY������ֱ��ʹ��HKEY
    // HKEY ConvertRootToHKEY(RegistryRoot root);
    
    std::string HKEYToString(HKEY hkey);
    std::string RegistryTypeToString(DWORD type);
    std::string RegistryDataToString(const BYTE* data, DWORD size, DWORD type);
    
    std::string WideToUTF8(const std::wstring& wideStr);
    std::string SanitizeUTF8(const std::string& str);
    // ��ȫ��ע������ - �޸Ĳ�������ΪHKEY
    bool SafeOpenKey(HKEY root, const std::string& subKey, HKEY& hKey);
    void SafeCloseKey(HKEY hKey);
    
    // Ԥ����Ĺ�ע·��
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