#pragma once
#include "../core/SystemSnapshot.h"
#include "../core/SnapshotManager.h"
// #include "../core/SnapshotComparator.h"
#include "../core/CPUInfo/system_info.h"
#include "../core/CPUInfo/cpu_monitor.h"
#include "../core/Memory/memory_monitor.h"
#include "../core/Process/process_monitor.h"
#include "../core/Disk/disk_monitor.h"
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
    

    void HandleGetCPUInfo(const httplib::Request& req, httplib::Response& res);
    void HandleGetCPUUsage(const httplib::Request& req, httplib::Response& res);
    void HandleGetSystemInfo(const httplib::Request& req, httplib::Response& res);
    void HandleStreamCPUUsage(const httplib::Request& req, httplib::Response& res);

    void HandleGetMemoryUsage(const httplib::Request& req, httplib::Response& res);

    void HandleGetCPUHistory(const httplib::Request& req, httplib::Response& res);
    void HandleGetMemoryHistory(const httplib::Request& req, httplib::Response& res);

    void HandleGetProcesses(const httplib::Request& req, httplib::Response& res);
    void HandleGetProcessInfo(const httplib::Request& req, httplib::Response& res);
    void HandleFindProcesses(const httplib::Request& req, httplib::Response& res);
    void HandleTerminateProcess(const httplib::Request& req, httplib::Response& res);


    void HandleGetDiskInfo(const httplib::Request& req, httplib::Response& res);
    void HandleGetDiskPerformance(const httplib::Request& req, httplib::Response& res);

    void HandleGetRegistrySnapshot(const httplib::Request& req, httplib::Response& res);
    void HandleSaveRegistrySnapshot(const httplib::Request& req, httplib::Response& res);
    void HandleGetSavedSnapshots(const httplib::Request& req, httplib::Response& res);
    void HandleCompareSnapshots(const httplib::Request& req, httplib::Response& res);
    void HandleDeleteSnapshot(const httplib::Request& req, httplib::Response& res);

    void HandleGetDriverSnapshot(const httplib::Request& req, httplib::Response& res);
    void HandleGetDriverDetail(const httplib::Request& req, httplib::Response& res);

    void HandleCreateSystemSnapshot(const httplib::Request& req, httplib::Response& res);
    void HandleListSystemSnapshots(const httplib::Request& req, httplib::Response& res);
    void HandleSaveSystemSnapshot(const httplib::Request& req, httplib::Response& res);
    void HandleGetSystemSnapshot(const httplib::Request& req, httplib::Response& res);
    void LoadSnapshotsFromDisk();
    void HandleDeleteSystemSnapshot(const httplib::Request& req, httplib::Response& res);

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
    

    MemoryMonitor memoryMonitor_;

    MemoryUsage memoryUsage_{}; // value-initialize to zeros
    std::mutex memoryUsageMutex_; // protect memoryUsage_

    struct Sample { uint64_t timestamp; double value; };
    std::vector<Sample> cpuHistory_;
    std::vector<Sample> memoryHistory_;
    std::mutex cpuHistoryMutex_;
    std::mutex memoryHistoryMutex_;
    size_t maxHistorySamples_ = 3600; 

    ProcessMonitor processMonitor_;
    
    DiskMonitor diskMonitor_;  

    RegistryMonitor registryMonitor_;  
    std::map<std::string, RegistrySnapshot> registrySnapshots_; 
    DriverMonitor driverMonitor_;

    std::map<std::string, std::string> systemSnapshotsJson_; // name -> json
    std::mutex systemSnapshotsMutex_;
    // Persistent store
    std::unique_ptr<snapshot::SnapshotManager> snapshotStore_;
    bool snapshotsLoadedFromDisk_ = false;
    
    int port_;
};

} // namespace snapshot