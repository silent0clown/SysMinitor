#pragma once
// #include "../core/SystemSnapshot.h"
// #include "../core/SnapshotManager.h"
// #include "../core/SnapshotComparator.h"
#include "../core/CPUInfo/system_info.h"
#include "../core/CPUInfo/cpu_monitor.h"
#include "../core/Process/process_monitor.h"
#include "../core/Disk/disk_monitor.h"
// #include "../core/Register/register_minitor.h"
#include "../core/Register/registry_monitor.h"
#include "../core/Driver/driver_monitor.h"
#include "../third_party/httplib.h"
#include "../third_party/nlohmann/json.hpp"
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>

using json = nlohmann::json;

namespace sysmonitor {

class HttpServer {
public:
    HttpServer();
    ~HttpServer();

    bool Start(int port = 8080);
    void Stop();
    bool IsRunning() const { return isRunning_; }

private:
    void SetupRoutes();
    void StartBackgroundMonitoring();
    
    // HTTP·�ɴ����� CPU���
    void HandleGetCPUInfo(const httplib::Request& req, httplib::Response& res);
    void HandleGetCPUUsage(const httplib::Request& req, httplib::Response& res);
    void HandleGetSystemInfo(const httplib::Request& req, httplib::Response& res);
    void HandleStreamCPUUsage(const httplib::Request& req, httplib::Response& res);

    // ����µ�HTTP·�ɴ����� �������
    void HandleGetProcesses(const httplib::Request& req, httplib::Response& res);
    void HandleGetProcessInfo(const httplib::Request& req, httplib::Response& res);
    void HandleFindProcesses(const httplib::Request& req, httplib::Response& res);
    void HandleTerminateProcess(const httplib::Request& req, httplib::Response& res);

    // �������·��
    
    // ��Ӵ�����ص�HTTP·�ɴ�����
    void HandleGetDiskInfo(const httplib::Request& req, httplib::Response& res);
    void HandleGetDiskPerformance(const httplib::Request& req, httplib::Response& res);

    // ���ע�����ص�HTTP·�ɴ�����
    void HandleGetRegistrySnapshot(const httplib::Request& req, httplib::Response& res);
    void HandleSaveRegistrySnapshot(const httplib::Request& req, httplib::Response& res);
    void HandleGetSavedSnapshots(const httplib::Request& req, httplib::Response& res);
    void HandleCompareSnapshots(const httplib::Request& req, httplib::Response& res);
    void HandleDeleteSnapshot(const httplib::Request& req, httplib::Response& res);

    void HandleGetDriverSnapshot(const httplib::Request& req, httplib::Response& res);
    void HandleGetDriverDetail(const httplib::Request& req, httplib::Response& res);

    // �޸ĺ������ƣ������ͻ
    json CompareRegistrySnapshots(const std::vector<RegistryKey>& keys1, const std::vector<RegistryKey>& keys2);
    std::string GetCurrentTimeString();
    std::vector<RegistryKey> ParseRegistryKeysFromJson(const json& json_array);
    std::string SafeJsonToString(const json& json_value);
    std::string SanitizeUTF8(const std::string& str);

private:
    std::unique_ptr<httplib::Server> server_;
    std::unique_ptr<std::thread> serverThread_;
    std::atomic<bool> isRunning_{false};
    
    CPUMonitor cpuMonitor_;
    CPUInfo cpuInfo_;
    std::atomic<double> currentUsage_{0.0};
    
    // ��ӽ��̼����
    ProcessMonitor processMonitor_;
    
    DiskMonitor diskMonitor_;  // ��Ӵ��̼����

    RegistryMonitor registryMonitor_;  // ���ע�������
    std::map<std::string, RegistrySnapshot> registrySnapshots_;  // �洢�������
    // ������������
    DriverMonitor driverMonitor_;
    
    int port_;
};

} // namespace snapshot