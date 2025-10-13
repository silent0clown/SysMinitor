#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <string>
#include <vector>
#include <chrono>
#include <windows.h>
#include "../../utils/encode.h"

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
    std::string driverType;             // �������� (Kernel/FileSystem)
    DWORD pid;                          // ����ID����������У�
    std::string exitCode;               // �˳�����
    std::string win32ExitCode;          // Win32�˳�����
    std::string serviceSpecificExitCode;// �����ض��˳�����
    std::chrono::system_clock::time_point installTime; // ��װʱ��
    
    // ת��ΪUTF-8����
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
        exitCode = util::EncodingUtil::SafeString(exitCode);
        win32ExitCode = util::EncodingUtil::SafeString(win32ExitCode);
        serviceSpecificExitCode = util::EncodingUtil::SafeString(serviceSpecificExitCode);
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
    std::vector<DriverDetail> runningDrivers;       // �������е�����
    std::vector<DriverDetail> stoppedDrivers;       // ��ֹͣ������
    std::vector<DriverDetail> autoStartDrivers;     // �Զ�����������
    std::vector<DriverDetail> thirdPartyDrivers;    // ����������
    
    // ͳ����Ϣ
    struct {
        size_t totalDrivers;
        size_t runningCount;
        size_t stoppedCount;
        size_t kernelCount;
        size_t fileSystemCount;
        size_t autoStartCount;
        size_t thirdPartyCount;
    } stats;
};

class DriverMonitor {
public:
    DriverMonitor();
    ~DriverMonitor();
    
    /**
     * @brief ��ȡ��������
     * @return ��������������
     */
    DriverSnapshot GetDriverSnapshot();
    
    /**
     * @brief ��ȡ�ض���������ϸ��Ϣ
     * @param driverName ��������
     * @return ������ϸ��Ϣ
     */
    DriverDetail GetDriverDetail(const std::string& driverName);
    
    /**
     * @brief ��������Ƿ���������
     * @param driverName ��������
     * @return true-��������
     */
    bool IsDriverRunning(const std::string& driverName);

private:
    /**
     * @brief ͨ��SCM��ȡ��������
     */
    std::vector<DriverDetail> GetAllDriversViaSCM();
    
    /**
     * @brief ��ȡ�ں�����
     */
    std::vector<DriverDetail> GetKernelDrivers();
    
    /**
     * @brief ��ȡ�ļ�ϵͳ����
     */
    std::vector<DriverDetail> GetFileSystemDrivers();
    
    /**
     * @brief ��ȡ�������е�����
     */
    std::vector<DriverDetail> GetRunningDrivers();
    
    /**
     * @brief ��ȡ��ֹͣ������
     */
    std::vector<DriverDetail> GetStoppedDrivers();
    
    /**
     * @brief ��ȡ�Զ�����������
     */
    std::vector<DriverDetail> GetAutoStartDrivers();
    
    /**
     * @brief ��ȡ����������
     */
    std::vector<DriverDetail> GetThirdPartyDrivers();
    
    /**
     * @brief ��������
     */
    void CategorizeDrivers(DriverSnapshot& snapshot, const std::vector<DriverDetail>& allDrivers);
    
    /**
     * @brief ����ͳ����Ϣ
     */
    void CalculateStatistics(DriverSnapshot& snapshot);
    
    /**
     * @brief �������������ı���
     */
    void SanitizeAllDrivers(DriverSnapshot& snapshot);
};