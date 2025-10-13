#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <string>
#include <vector>
#include <chrono>
#include <windows.h>
#include "../../utils/encode.h"
struct DriverVersion {
    std::string fileVersion;        // �ļ��汾
    std::string productVersion;     // ��Ʒ�汾
    std::string companyName;        // ��˾����
    std::string fileDescription;    // �ļ�����
    std::string legalCopyright;     // ��Ȩ��Ϣ
    std::string originalFilename;   // ԭʼ�ļ���
};

struct DriverDetail {
    std::string name;                   // ��������
    std::string displayName;            // ��ʾ����
    std::string description;            // ����
    std::string state;                  // ״̬
    std::string startType;              // ��������
    std::string binaryPath;             // �����ļ�·��
    std::string serviceType;            // ��������
    std::string errorControl;           // �������
    std::string account;                // �����˻�
    std::string group;                  // ������
    std::string tagId;                  // ��ǩID
    std::string driverType;             // ��������
    std::string hardwareClass;          // Ӳ�����
    DWORD pid;                          // ����ID
    std::string exitCode;               // �˳�����
    std::string win32ExitCode;          // Win32�˳�����
    std::string serviceSpecificExitCode;// �����ض��˳�����
    DriverVersion version;              // �汾��Ϣ
    std::chrono::system_clock::time_point installTime;
    
    DriverDetail() : pid(0) {}
    
    DriverDetail& convertToUTF8() {
        name = util::EncodingUtil::SafeString(name);
        displayName = util::EncodingUtil::SafeString(displayName);
        description = util::EncodingUtil::SafeString(description);
        state = util::EncodingUtil::SafeString(state);
        startType = util::EncodingUtil::SafeString(startType);
        binaryPath = util::EncodingUtil::SafeString(binaryPath);
        serviceType = util::EncodingUtil::SafeString(serviceType);
        errorControl = util::EncodingUtil::SafeString(errorControl);
        account = util::EncodingUtil::SafeString(account);
        group = util::EncodingUtil::SafeString(group);
        tagId = util::EncodingUtil::SafeString(tagId);
        driverType = util::EncodingUtil::SafeString(driverType);
        hardwareClass = util::EncodingUtil::SafeString(hardwareClass);
        exitCode = util::EncodingUtil::SafeString(exitCode);
        win32ExitCode = util::EncodingUtil::SafeString(win32ExitCode);
        serviceSpecificExitCode = util::EncodingUtil::SafeString(serviceSpecificExitCode);
        
        // ����汾��Ϣ
        version.fileVersion = util::EncodingUtil::SafeString(version.fileVersion);
        version.productVersion = util::EncodingUtil::SafeString(version.productVersion);
        version.companyName = util::EncodingUtil::SafeString(version.companyName);
        version.fileDescription = util::EncodingUtil::SafeString(version.fileDescription);
        version.legalCopyright = util::EncodingUtil::SafeString(version.legalCopyright);
        version.originalFilename = util::EncodingUtil::SafeString(version.originalFilename);
        
        return *this;
    }
    
    bool operator==(const DriverDetail& other) const {
        return name == other.name;
    }
};

struct DriverSnapshot {
    uint64_t timestamp;
    std::vector<DriverDetail> kernelDrivers;        // �ں�����
    std::vector<DriverDetail> fileSystemDrivers;    // �ļ�ϵͳ����
    std::vector<DriverDetail> hardwareDrivers;      // Ӳ������
    std::vector<DriverDetail> runningDrivers;       // ����������
    std::vector<DriverDetail> stoppedDrivers;       // ��ֹͣ����
    std::vector<DriverDetail> autoStartDrivers;     // �Զ���������
    std::vector<DriverDetail> thirdPartyDrivers;    // ����������
    
    // ��Ӳ��������
    std::vector<DriverDetail> displayDrivers;       // ��ʾ����������
    std::vector<DriverDetail> audioDrivers;         // ��Ƶ����
    std::vector<DriverDetail> networkDrivers;       // ��������������
    std::vector<DriverDetail> inputDrivers;         // �����豸����
    std::vector<DriverDetail> storageDrivers;       // �洢����������
    std::vector<DriverDetail> printerDrivers;       // ��ӡ������
    std::vector<DriverDetail> usbDrivers;           // USB����
    std::vector<DriverDetail> bluetoothDrivers;     // ��������
    
    struct {
        size_t totalDrivers;
        size_t runningCount;
        size_t stoppedCount;
        size_t kernelCount;
        size_t fileSystemCount;
        size_t hardwareCount;
        size_t autoStartCount;
        size_t thirdPartyCount;
        // Ӳ������ͳ��
        size_t displayCount;
        size_t audioCount;
        size_t networkCount;
        size_t inputCount;
        size_t storageCount;
        size_t printerCount;
        size_t usbCount;
        size_t bluetoothCount;
    } stats;
    
    DriverSnapshot() : timestamp(0) {
        stats.totalDrivers = 0;
        stats.runningCount = 0;
        stats.stoppedCount = 0;
        stats.kernelCount = 0;
        stats.fileSystemCount = 0;
        stats.hardwareCount = 0;
        stats.autoStartCount = 0;
        stats.thirdPartyCount = 0;
        stats.displayCount = 0;
        stats.audioCount = 0;
        stats.networkCount = 0;
        stats.inputCount = 0;
        stats.storageCount = 0;
        stats.printerCount = 0;
        stats.usbCount = 0;
        stats.bluetoothCount = 0;
    }
};

class DriverMonitor {
public:
    DriverMonitor();
    ~DriverMonitor();
    
    DriverSnapshot GetDriverSnapshot();
    DriverDetail GetDriverDetail(const std::string& driverName);
    bool IsDriverRunning(const std::string& driverName);

private:
    std::vector<DriverDetail> GetAllDriversViaSCM();
    std::vector<DriverDetail> GetHardwareDriversViaSetupAPI();
    std::vector<DriverDetail> GetDriversViaWMI();
    
    // �汾��Ϣ��ȡ
    DriverVersion GetDriverVersionInfo(const std::string& filePath);
    std::string GetFileVersion(const std::string& filePath);
    
    // Ӳ����������
    void CategorizeHardwareDrivers(DriverSnapshot& snapshot, const std::vector<DriverDetail>& hardwareDrivers);
    void CategorizeDrivers(DriverSnapshot& snapshot, const std::vector<DriverDetail>& allDrivers);
    void CalculateStatistics(DriverSnapshot& snapshot);
    void SanitizeAllDrivers(DriverSnapshot& snapshot);
    
    // ���ߺ���
    bool IsHardwareDriver(const DriverDetail& driver);
    std::string DetectHardwareClass(const DriverDetail& driver);
};