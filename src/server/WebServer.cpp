#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "WebServer.h"
#include "../utils/AsyncLogger.h"
#include "../utils/util_time.h"
#include "../third_party/nlohmann/json.hpp"
#include <sstream>
#include <iomanip>
#include "../core/SystemSnapshotCollector.h"

using json = nlohmann::json;

namespace sysmonitor {

HttpServer::HttpServer() : port_(8080) {
    cpuInfo_ = SystemInfo::GetCPUInfo();
}

HttpServer::~HttpServer() {
    Stop();
}

bool HttpServer::Start(int port) {
    if (isRunning_) return true;
    
    port_ = port;
    server_ = std::make_unique<httplib::Server>();
    
    // Set up routes
    SetupRoutes();
    
    // Initialize CPU monitoring
    if (!cpuMonitor_.Initialize()) {
        std::cerr << "CPU monitor initialization failed" << std::endl;
        return false;
    }
    
    // Start background monitoring
    StartBackgroundMonitoring();
    
    // Start HTTP server
    serverThread_ = std::make_unique<std::thread>([this]() {
        std::cout << "Starting HTTP server on port: " << port_ << std::endl;
        std::cout << "API endpoints:" << std::endl;
        std::cout << "  GET /api/cpu/info     - Get CPU information" << std::endl;
        std::cout << "  GET /api/cpu/usage    - Get current CPU usage" << std::endl;
        std::cout << "  GET /api/system/info  - Get system information" << std::endl;
        std::cout << "  GET /api/cpu/stream   - Real-time streaming CPU usage" << std::endl;
        
        isRunning_ = true;
        server_->listen("0.0.0.0", port_);
        isRunning_ = false;
    });
    
    // Wait for server to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return true;
}

void HttpServer::Stop() {
    if (server_) {
        server_->stop();
    }
    
    cpuMonitor_.StopMonitoring();
    
    if (serverThread_ && serverThread_->joinable()) {
        serverThread_->join();
    }
    
    isRunning_ = false;
}

void HttpServer::SetupRoutes() {
    // Static file service (for frontend pages)
    // server_->set_mount_point("/", "www");//Test page
    server_->set_mount_point("/", "webclient");

    // API routes - CPU related
    server_->Get("/api/cpu/info", [this](const httplib::Request& req, httplib::Response& res) {
        HandleGetCPUInfo(req, res);
    });
    
    server_->Get("/api/cpu/usage", [this](const httplib::Request& req, httplib::Response& res) {
        HandleGetCPUUsage(req, res);
    });
    
    server_->Get("/api/system/info", [this](const httplib::Request& req, httplib::Response& res) {
        HandleGetSystemInfo(req, res);
    });
    
    server_->Get("/api/cpu/stream", [this](const httplib::Request& req, httplib::Response& res) {
        HandleStreamCPUUsage(req, res);
    });
    
    // API routes - Memory related
    server_->Get("/api/memory/usage", [this](const httplib::Request& req, httplib::Response& res) {
        HandleGetMemoryUsage(req, res);
    });

    // Historical data routes
    server_->Get("/api/cpu/history", [this](const httplib::Request& req, httplib::Response& res) {
        HandleGetCPUHistory(req, res);
    });

    server_->Get("/api/memory/history", [this](const httplib::Request& req, httplib::Response& res) {
        HandleGetMemoryHistory(req, res);
    });

    // Add new API routes - Process related
    server_->Get("/api/processes", [this](const httplib::Request& req, httplib::Response& res) {
        HandleGetProcesses(req, res);
    });
    
    server_->Get("/api/process/(\\d+)", [this](const httplib::Request& req, httplib::Response& res) {
        HandleGetProcessInfo(req, res);
    });
    
    server_->Get("/api/process/find", [this](const httplib::Request& req, httplib::Response& res) {
        HandleFindProcesses(req, res);
    });
    
    // server_->Post("/api/process/(\\d+)/terminate", [this](const httplib::Request& req, httplib::Response& res) {
    //     HandleTerminateProcess(req, res);
    // });


    // Disk related routes
    server_->Get("/api/disk/info", [this](const httplib::Request& req, httplib::Response& res) {
        HandleGetDiskInfo(req, res);
    });
    
    server_->Get("/api/disk/performance", [this](const httplib::Request& req, httplib::Response& res) {
        HandleGetDiskPerformance(req, res);
    });

    // Add OPTIONS request handling (for CORS preflight)
    server_->Options("/api/disk/info", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.status = 200;
    });
    
    server_->Options("/api/disk/performance", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.status = 200;
    });


    // Registry related APIs
    server_->Get("/api/registry/snapshot", [this](const httplib::Request& req, httplib::Response& res) {
        // HandleGetRegistrySnapshot(req, res);
    });
    
    server_->Get("/api/registry/snapshot/save", [this](const httplib::Request& req, httplib::Response& res) {
        // HandleSaveRegistrySnapshot(req, res);
        HandleSaveRegistry(req, res);
    });
    
    server_->Get("/api/registry/snapshot/Info", [this](const httplib::Request& req, httplib::Response& res) {
        // HandleGetSavedSnapshots(req, res);
        HandleGetRegistryInfo(req, res);
    });
    
    server_->Post("/api/registry/snapshot/compare", [this](const httplib::Request& req, httplib::Response& res) {
        // HandleCompareSnapshots(req, res);
        HandleCompareFolders(req, res);
    });
    
    server_->Delete("/api/registry/snapshots/delete/([^/]+)", [this](const httplib::Request& req, httplib::Response& res) {
    HandleDeleteSnapshot(req, res);
    });


    // Driver information
    server_->Get("/api/drivers/snapshot", [this](const httplib::Request& req, httplib::Response& res) {
        HandleGetDriverSnapshot(req, res);
    });
    
    server_->Get("/api/drivers/detail", [this](const httplib::Request& req, httplib::Response& res) {
        HandleGetDriverDetail(req, res);
    });

    // SystemSnapshot
    server_->Post("/api/system/snapshot/create", [this](const httplib::Request& req, httplib::Response& res) {
        HandleCreateSystemSnapshot(req, res);
    });

    server_->Get("/api/system/snapshot/list", [this](const httplib::Request& req, httplib::Response& res) {
        HandleListSystemSnapshots(req, res);
    });

    server_->Get("/api/system/snapshot/get", [this](const httplib::Request& req, httplib::Response& res) {
        HandleGetSystemSnapshot(req, res);
    });

    server_->Post("/api/system/snapshot/save", [this](const httplib::Request& req, httplib::Response& res) {
        HandleSaveSystemSnapshot(req, res);
    });

    server_->Post("/api/system/snapshot/compare", [this](const httplib::Request& req, httplib::Response& res) {
        HandleCompareSystemSnapshots(req, res);
    });

    server_->Delete("/api/system/snapshot/delete/([^/]+)", [this](const httplib::Request& req, httplib::Response& res) {
        HandleDeleteSystemSnapshot(req, res);
    });

    // 默认路由
    server_->Get("/", [](const httplib::Request& req, httplib::Response& res) {
        res.set_redirect("/index.html");
    });
}

void HttpServer::StartBackgroundMonitoring() {
    // Set CPU usage callback and record historical samples
    cpuMonitor_.SetUsageCallback([this](const CPUUsage& usage) {
        currentUsage_.store(usage.totalUsage);

        Sample s{GET_LOCAL_TIME_MS(), usage.totalUsage};
        {
            std::lock_guard<std::mutex> lk(cpuHistoryMutex_);
            cpuHistory_.push_back(s);
            if (cpuHistory_.size() > maxHistorySamples_) {
                cpuHistory_.erase(cpuHistory_.begin(), cpuHistory_.begin() + (cpuHistory_.size() - maxHistorySamples_));
            }
        }
    });

    // Start CPU monitoring
    cpuMonitor_.StartMonitoring(1000);

    // Set memory usage callback and record historical samples
    memoryMonitor_.SetUsageCallback([this](const MemoryUsage& usage) {
        {
            std::lock_guard<std::mutex> lk(memoryUsageMutex_);
            memoryUsage_ = usage;
        }

        Sample s{usage.timestamp ? usage.timestamp : GET_LOCAL_TIME_MS(), usage.usedPercent};
        {
            std::lock_guard<std::mutex> lk(memoryHistoryMutex_);
            memoryHistory_.push_back(s);
            if (memoryHistory_.size() > maxHistorySamples_) {
                memoryHistory_.erase(memoryHistory_.begin(), memoryHistory_.begin() + (memoryHistory_.size() - maxHistorySamples_));
            }
        }
    });

    // Start memory monitoring
    memoryMonitor_.StartMonitoring(1000);
}

void HttpServer::HandleGetCPUInfo(const httplib::Request& req, httplib::Response& res) {
    json response;
    
    response["name"] = cpuInfo_.name;
    response["vendor"] = cpuInfo_.vendor;
    response["architecture"] = cpuInfo_.architecture;
    response["physicalCores"] = cpuInfo_.physicalCores;
    response["logicalCores"] = cpuInfo_.logicalCores;
    response["packages"] = cpuInfo_.packages;
    response["baseFrequency"] = cpuInfo_.baseFrequency;
    response["maxFrequency"] = cpuInfo_.maxFrequency;
    
    res.set_content(response.dump(), "application/json");
}

void HttpServer::HandleGetCPUUsage(const httplib::Request& req, httplib::Response& res) {
    json response;
    
    double usage = currentUsage_.load();
    // 保留小数点后2位
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << usage;
    double rounded_usage = std::stod(oss.str());
    
    response["usage"] = rounded_usage;
    response["timestamp"] = GET_LOCAL_TIME_MS();
    response["unit"] = "percent";
    
    res.set_content(response.dump(), "application/json");
}

void HttpServer::HandleGetCPUHistory(const httplib::Request& req, httplib::Response& res) {
    try {
        json arr = json::array();
        std::lock_guard<std::mutex> lk(cpuHistoryMutex_);
        for (const auto &s : cpuHistory_) {
            arr.push_back({{"timestamp", s.timestamp}, {"value", s.value}});
        }
        res.set_content(arr.dump(), "application/json");
    } catch (const std::exception& e) {
        json error;
        error["error"] = e.what();
        res.status = 500;
        res.set_content(error.dump(), "application/json");
    }
}

void HttpServer::HandleGetMemoryHistory(const httplib::Request& req, httplib::Response& res) {
    try {
        json arr = json::array();
        std::lock_guard<std::mutex> lk(memoryHistoryMutex_);
        for (const auto &s : memoryHistory_) {
            arr.push_back({{"timestamp", s.timestamp}, {"value", s.value}});
        }
        res.set_content(arr.dump(), "application/json");
    } catch (const std::exception& e) {
        json error;
        error["error"] = e.what();
        res.status = 500;
        res.set_content(error.dump(), "application/json");
    }
}

void HttpServer::HandleGetMemoryUsage(const httplib::Request& req, httplib::Response& res) {
    json response;

    MemoryUsage snapshot = memoryMonitor_.GetCurrentUsage();
    response["totalPhysical"] = snapshot.totalPhysical;
    response["availablePhysical"] = snapshot.availablePhysical;
    response["usedPhysical"] = snapshot.usedPhysical;
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << snapshot.usedPercent;
    double rounded_usage = std::stod(oss.str());

    response["usedPercent"] = rounded_usage;
    response["timestamp"] = snapshot.timestamp;
    response["unit"] = "bytes";

    res.set_content(response.dump(), "application/json");
}

void HttpServer::HandleGetSystemInfo(const httplib::Request& req, httplib::Response& res) {
    json response;
    
    // Basic system information
    response["cpu"]["info"] = {
        {"name", cpuInfo_.name},
        {"cores", cpuInfo_.physicalCores},
        {"threads", cpuInfo_.logicalCores}
    };
    
    response["cpu"]["currentUsage"] = currentUsage_.load();
    response["timestamp"] = GET_LOCAL_TIME_MS();
    
    res.set_content(response.dump(), "application/json");
}

void HttpServer::HandleStreamCPUUsage(const httplib::Request& req, httplib::Response& res) {
    // Set Server-Sent Events (SSE) headers
    res.set_header("Content-Type", "text/event-stream");
    res.set_header("Cache-Control", "no-cache");
    res.set_header("Connection", "keep-alive");
    res.set_header("Access-Control-Allow-Origin", "*");
    
    // Send initial data
    std::stringstream initialData;
    initialData << "data: " << json{{"usage", currentUsage_.load()}, {"timestamp", GET_LOCAL_TIME_MS()}}.dump() << "\n\n";
    res.set_content(initialData.str(), "text/event-stream");
    
    // Note: This is a simplified streaming implementation, production environment requires more complex connection management
}

void HttpServer::HandleGetProcesses(const httplib::Request& req, httplib::Response& res) {
    try {
        auto snapshot = processMonitor_.GetProcessSnapshot();
        json response;
        
        // 数值字段直接赋值
        response["timestamp"] = snapshot.timestamp;
        response["totalProcesses"] = snapshot.totalProcesses;
        response["totalThreads"] = snapshot.totalThreads;
        response["totalHandles"] = snapshot.totalHandles;
        response["totalGdiObjects"] = snapshot.totalGdiObjects;
        response["totalUserObjects"] = snapshot.totalUserObjects;
        
        json processesJson = json::array();
        for (const auto& process : snapshot.processes) {
            try {
                json processJson;
                
                // 数值字段直接赋值
                processJson["pid"] = process.pid;
                processJson["parentPid"] = process.parentPid;
                processJson["threadCount"] = process.threadCount;
                processJson["cpuUsage"] = process.cpuUsage;
                processJson["memoryUsage"] = process.memoryUsage;
                processJson["workingSetSize"] = process.workingSetSize;
                processJson["pagefileUsage"] = process.pagefileUsage;
                processJson["createTime"] = process.createTime;
                processJson["priority"] = process.priority;
                processJson["handleCount"] = process.handleCount;   // 获取指定进程当前打开的句柄总数,用于评估进程的资源使用情况，排查句柄泄漏问题
                processJson["gdiCount"] = process.gdiCount;         // 获取进程使用的 GDI 对象数量,监控 GDI 数量有助于检测图形资源泄漏
                processJson["userCount"] = process.userCount;       // 获取进程使用的 USER 对象数量,监控 USER 数量有助于检测窗口泄漏或 UI 资源泄漏
                
                // 字符串字段需要 UTF-8 清理
                processJson["name"] = util::EncodingUtil::ToUTF8(process.name);
                processJson["fullPath"] = util::EncodingUtil::ToUTF8(process.fullPath);
                processJson["state"] = util::EncodingUtil::ToUTF8(process.state);
                processJson["username"] = util::EncodingUtil::ToUTF8(process.username);
                processJson["commandLine"] = util::EncodingUtil::ToUTF8(process.commandLine);
                
                // 确保没有空值，提供默认值
                if (processJson["name"].get<std::string>().empty()) {
                    processJson["name"] = "Unknown";
                }
                if (processJson["state"].get<std::string>().empty()) {
                    processJson["state"] = "Unknown";
                }
                if (processJson["username"].get<std::string>().empty()) {
                    processJson["username"] = "Unknown";
                }
                
                processesJson.push_back(processJson);
                
            } catch (const std::exception& e) {
                std::cerr << "Error serializing process " << process.pid 
                          << ": " << e.what() << std::endl;
                // 跳过有问题的进程，继续处理下一个
                continue;
            }
        }
        
        response["processes"] = processesJson;
        
        // 设置正确的 Content-Type 头部
        res.set_header("Content-Type", "application/json; charset=utf-8");
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        std::cerr << "HandleGetProcesses exception: " << e.what() << std::endl;
        json error;
        error["error"] = SanitizeUTF8("Failed to get process list: " + std::string(e.what()));
        res.set_header("Content-Type", "application/json; charset=utf-8");
        res.status = 500;
        res.set_content(error.dump(), "application/json");
    }
}


// // Add new handler function implementations
// void HttpServer::HandleGetProcesses(const httplib::Request& req, httplib::Response& res) {
//     try {
//         auto snapshot = processMonitor_.GetProcessSnapshot();
//         json response;
        
//         response["timestamp"] = snapshot.timestamp;
//         response["totalProcesses"] = snapshot.totalProcesses;
//         response["totalThreads"] = snapshot.totalThreads;
//         response["totalHandles"] = snapshot.totalHandles;        // 新�??
//         response["totalGdiObjects"] = snapshot.totalGdiObjects;  // 新�??
//         response["totalUserObjects"] = snapshot.totalUserObjects; // 新�??
        
//         json processesJson = json::array();
//         for (const auto& process : snapshot.processes) {
//             json processJson;
//             processJson["pid"] = process.pid;
//             processJson["parentPid"] = process.parentPid;
//             processJson["name"] = process.name;
//             processJson["fullPath"] = process.fullPath;
//             processJson["state"] = process.state;
//             processJson["username"] = process.username;
//             processJson["cpuUsage"] = process.cpuUsage;
//             processJson["memoryUsage"] = process.memoryUsage;
//             processJson["workingSetSize"] = process.workingSetSize;
//             processJson["pagefileUsage"] = process.pagefileUsage;
//             processJson["createTime"] = process.createTime;
//             processJson["priority"] = process.priority;
//             processJson["threadCount"] = process.threadCount;
//             processJson["commandLine"] = process.commandLine;
//             processJson["handleCount"] = process.handleCount;    // 新�??
//             processJson["gdiCount"] = process.gdiCount;          // 新�??
//             processJson["userCount"] = process.userCount;        // 新�??
            
//             processesJson.push_back(processJson);
//         }
        
//         response["processes"] = processesJson;
//         res.set_content(response.dump(), "application/json");
        
//     } catch (const std::exception& e) {
//         json error;
//         error["error"] = e.what();
//         res.status = 500;
//         res.set_content(error.dump(), "application/json");
//     }
// }

void HttpServer::HandleGetProcessInfo(const httplib::Request& req, httplib::Response& res) {
    try {
        uint32_t pid = std::stoi(req.matches[1]);
        auto processInfo = processMonitor_.GetProcessInfo(pid);
        
        if (processInfo.pid == 0) {
            res.status = 404;
            json error;
            error["error"] = "Process not found";
            res.set_content(error.dump(), "application/json");
            return;
        }
        
        json response;
        response["pid"] = processInfo.pid;
        response["parentPid"] = processInfo.parentPid;
        response["name"] = processInfo.name;
        response["fullPath"] = processInfo.fullPath;
        response["state"] = processInfo.state;
        response["username"] = processInfo.username;
        response["cpuUsage"] = processInfo.cpuUsage;
        response["memoryUsage"] = processInfo.memoryUsage;
        response["workingSetSize"] = processInfo.workingSetSize;
        response["pagefileUsage"] = processInfo.pagefileUsage;
        response["createTime"] = processInfo.createTime;
        response["priority"] = processInfo.priority;
        response["threadCount"] = processInfo.threadCount;
        response["commandLine"] = processInfo.commandLine;
        response["handleCount"] = processInfo.handleCount;   // 获取指定进程当前打开的句柄总数,用于评估进程的资源使用情况，排查句柄泄漏问题
        response["gdiCount"] = processInfo.gdiCount;         // 获取进程使用的 GDI 对象数量,监控 GDI 数量有助于检测图形资源泄漏
        response["userCount"] = processInfo.userCount;  

        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        json error;
        error["error"] = e.what();
        res.status = 500;
        res.set_content(error.dump(), "application/json");
    }
}

void HttpServer::HandleFindProcesses(const httplib::Request& req, httplib::Response& res) {
    try {
        std::string name = req.get_param_value("name");
        if (name.empty()) {
            res.status = 400;
            json error;
            error["error"] = "Name parameter is required";
            res.set_content(error.dump(), "application/json");
            return;
        }
        
        auto processes = processMonitor_.FindProcessesByName(name);
        json response = json::array();
        
        for (const auto& process : processes) {
            json processJson;
            processJson["pid"] = process.pid;
            processJson["name"] = process.name;
            processJson["cpuUsage"] = process.cpuUsage;
            processJson["memoryUsage"] = process.memoryUsage;
            processJson["username"] = process.username;
            processJson["threadCount"] = process.threadCount;
            processJson["handleCount"] = process.handleCount;   // 获取指定进程当前打开的句柄总数,用于评估进程的资源使用情况，排查句柄泄漏问题
            processJson["gdiCount"] = process.gdiCount;         // 获取进程使用的 GDI 对象数量,监控 GDI 数量有助于检测图形资源泄漏
            processJson["userCount"] = process.userCount;              
        
                response.push_back(processJson);
        }
        
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        json error;
        error["error"] = e.what();
        res.status = 500;
        res.set_content(error.dump(), "application/json");
    }
}

void HttpServer::HandleTerminateProcess(const httplib::Request& req, httplib::Response& res) {
    try {
        uint32_t pid = std::stoi(req.matches[1]);
        uint32_t exitCode = 0;
        
        // Parse optional exit code
        auto exitCodeParam = req.get_param_value("exitCode");
        if (!exitCodeParam.empty()) {
            exitCode = std::stoi(exitCodeParam);
        }
        
        bool success = processMonitor_.TerminateProcess(pid, exitCode);
        
        json response;
        response["success"] = success;
        response["pid"] = pid;
        
        if (!success) {
            res.status = 500;
            response["error"] = "Failed to terminate process";
        }
        
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        json error;
        error["error"] = e.what();
        res.status = 500;
        res.set_content(error.dump(), "application/json");
    }
}

// Add disk information handler functions
void HttpServer::HandleGetDiskInfo(const httplib::Request& req, httplib::Response& res) {
    try {
        // Set CORS headers
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        
        auto snapshot = diskMonitor_.GetDiskSnapshot();
        json response;
        
        response["timestamp"] = snapshot.timestamp;
        
        // Disk drive information - use safer JSON building approach
        json drivesJson = json::array();
        for (const auto& drive : snapshot.drives) {
            try {
                json driveJson;
                // Ensure all strings are valid UTF-8
                driveJson["model"] = drive.model.empty() ? "Unknown" : drive.model;
                driveJson["serialNumber"] = drive.serialNumber.empty() ? "" : drive.serialNumber;
                driveJson["interfaceType"] = drive.interfaceType.empty() ? "Unknown" : drive.interfaceType;
                driveJson["mediaType"] = drive.mediaType.empty() ? "Unknown" : drive.mediaType;
                driveJson["totalSize"] = drive.totalSize;
                driveJson["bytesPerSector"] = drive.bytesPerSector;
                driveJson["status"] = drive.status.empty() ? "Unknown" : drive.status;
                driveJson["deviceId"] = drive.deviceId.empty() ? "Unknown" : drive.deviceId;
                
                // Verify JSON object can be serialized
                std::string test = driveJson.dump();
                drivesJson.push_back(driveJson);
            } catch (const std::exception& e) {
                std::cerr << "Skipping problematic drive data: " << e.what() << std::endl;
                continue;
            }
        }
        response["drives"] = drivesJson;
        
        // Partition information - use safer JSON building approach
        json partitionsJson = json::array();
        for (const auto& partition : snapshot.partitions) {
            try {
                json partitionJson;
                partitionJson["driveLetter"] = partition.driveLetter.empty() ? "Unknown" : partition.driveLetter;
                partitionJson["label"] = partition.label.empty() ? "Local Disk" : partition.label;
                partitionJson["fileSystem"] = partition.fileSystem.empty() ? "Unknown" : partition.fileSystem;
                partitionJson["totalSize"] = partition.totalSize;
                partitionJson["freeSpace"] = partition.freeSpace;
                partitionJson["usedSpace"] = partition.usedSpace;
                partitionJson["usagePercentage"] = partition.usagePercentage;
                partitionJson["serialNumber"] = partition.serialNumber;
                
                // Verify JSON object can be serialized
                std::string test = partitionJson.dump();
                partitionsJson.push_back(partitionJson);
            } catch (const std::exception& e) {
                std::cerr << "Skipping problematic partition data: " << e.what() << std::endl;
                continue;
            }
        }
        response["partitions"] = partitionsJson;
        
        std::string responseStr = response.dump();
        std::cout << "Returning disk information, drive count: " << snapshot.drives.size() 
                  << ", partition count: " << snapshot.partitions.size() 
                  << ", JSON length: " << responseStr.length() << std::endl;
        
        res.set_content(responseStr, "application/json");
        
    } catch (const std::exception& e) {
        std::cerr << "HandleGetDiskInfo exception: " << e.what() << std::endl;
        json error;
        error["error"] = "Internal server error";
        res.status = 500;
        res.set_content(error.dump(), "application/json");
    }
}

void HttpServer::HandleGetDiskPerformance(const httplib::Request& req, httplib::Response& res) {
    try {
        // Set CORS headers
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        
        auto snapshot = diskMonitor_.GetDiskSnapshot();
        json response;
        
        response["timestamp"] = snapshot.timestamp;
        
        json performanceJson = json::array();
        for (const auto& perf : snapshot.performance) {
            json perfJson;
            perfJson["driveLetter"] = perf.driveLetter;
            perfJson["readSpeed"] = perf.readSpeed;
            perfJson["writeSpeed"] = perf.writeSpeed;
            perfJson["readBytesPerSec"] = perf.readBytesPerSec;
            perfJson["writeBytesPerSec"] = perf.writeBytesPerSec;
            perfJson["readCountPerSec"] = perf.readCountPerSec;
            perfJson["writeCountPerSec"] = perf.writeCountPerSec;
            perfJson["queueLength"] = perf.queueLength;
            perfJson["usagePercentage"] = perf.usagePercentage;
            perfJson["responseTime"] = perf.responseTime;
            performanceJson.push_back(perfJson);
        }
        response["performance"] = performanceJson;
        
        std::cout << "Returning disk performance data, performance counter count: " << snapshot.performance.size() << std::endl;
        
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        std::cerr << "HandleGetDiskPerformance exception: " << e.what() << std::endl;
        json error;
        error["error"] = e.what();
        res.status = 500;
        res.set_content(error.dump(), "application/json");
    }
}

// Helper function: Get current time string
std::string HttpServer::GetCurrentTimeString() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

// void HttpServer::HandleGetRegistrySnapshot(const httplib::Request& req, httplib::Response& res) {
//     try {
//         res.set_header("Access-Control-Allow-Origin", "*");
//         res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
//         res.set_header("Access-Control-Allow-Headers", "Content-Type");
        
//         auto snapshot = registryMonitor_.GetRegistrySnapshot();
//         json response;
        
//         response["timestamp"] = snapshot.timestamp;
        
//         // System information keys
//         json systemInfoJson = json::array();
//         for (const auto& key : snapshot.systemInfoKeys) {
//             json keyJson;
//             keyJson["path"] = key.path;
            
//             json valuesJson = json::array();
//             for (const auto& value : key.values) {
//                 json valueJson;
//                 valueJson["name"] = value.name;
//                 valueJson["type"] = value.type;
//                 valueJson["data"] = value.data;
//                 valueJson["size"] = value.size;
//                 valuesJson.push_back(valueJson);
//             }
//             keyJson["values"] = valuesJson;
//             keyJson["subkeys"] = key.subkeys;
            
//             systemInfoJson.push_back(keyJson);
//         }
//         response["systemInfo"] = systemInfoJson;
        
//         // Software information keys
//         json softwareJson = json::array();
//         for (const auto& key : snapshot.softwareKeys) {
//             json keyJson;
//             keyJson["path"] = key.path;
            
//             json valuesJson = json::array();
//             for (const auto& value : key.values) {
//                 json valueJson;
//                 valueJson["name"] = value.name;
//                 valueJson["type"] = value.type;
//                 valueJson["data"] = value.data;
//                 valueJson["size"] = value.size;
//                 valuesJson.push_back(valueJson);
//             }
//             keyJson["values"] = valuesJson;
//             keyJson["subkeys"] = key.subkeys;
            
//             softwareJson.push_back(keyJson);
//         }
//         response["software"] = softwareJson;
        
//         // Network configuration keys
//         json networkJson = json::array();
//         for (const auto& key : snapshot.networkKeys) {
//             json keyJson;
//             keyJson["path"] = key.path;
            
//             json valuesJson = json::array();
//             for (const auto& value : key.values) {
//                 json valueJson;
//                 valueJson["name"] = value.name;
//                 valueJson["type"] = value.type;
//                 valueJson["data"] = value.data;
//                 valueJson["size"] = value.size;
//                 valuesJson.push_back(valueJson);
//             }
//             keyJson["values"] = valuesJson;
//             keyJson["subkeys"] = key.subkeys;
            
//             networkJson.push_back(keyJson);

//         }
//         response["network"] = networkJson;
        
//         // New: Auto-start items
//         json autoStartJson = json::array();
//         for (const auto& key : snapshot.autoStartKeys) {
//             json keyJson;
//             keyJson["path"] = key.path;
            
//             json valuesJson = json::array();
//             for (const auto& value : key.values) {
//                 json valueJson;
//                 valueJson["name"] = value.name;
//                 valueJson["type"] = value.type;
//                 valueJson["data"] = value.data;
//                 valueJson["size"] = value.size;
//                 valuesJson.push_back(valueJson);
//             }
//             keyJson["values"] = valuesJson;
//             keyJson["subkeys"] = key.subkeys;
            
//             // Add auto-start specific fields
//             if (!key.autoStartType.empty()) {
//                 keyJson["autoStartType"] = key.autoStartType;
//             }
//             if (!key.category.empty()) {
//                 keyJson["category"] = key.category;
//             }
            
//             autoStartJson.push_back(keyJson);
//         }
//         response["autoStart"] = autoStartJson;
        
//         std::string responseStr = response.dump();
//         std::cout << "Returning registry snapshot, system info keys: " << snapshot.systemInfoKeys.size()
//                   << ", software keys: " << snapshot.softwareKeys.size()
//                   << ", network keys: " << snapshot.networkKeys.size()
//                   << ", auto-start items: " << snapshot.autoStartKeys.size() << std::endl;
        
//         res.set_content(responseStr, "application/json");
        
//     } catch (const std::exception& e) {
//         std::cerr << "HandleGetRegistrySnapshot exception: " << e.what() << std::endl;
//         json error;
//         error["error"] = "Failed to get registry snapshot: " + std::string(e.what());
//         res.status = 500;
//         res.set_content(error.dump(), "application/json");
//     }
// }

void HttpServer::HandleSaveRegistry(const httplib::Request& req, httplib::Response& res) {
    try {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        
        json response;
        
        // 调用注册表备份函数
        int result = registryMonitor_.SaveReg();
        
        // 构建响应
        response["code"] = result;
        
        if (result == 0) {
            response["message"] = "All registry backups completed successfully!";
            response["status"] = "success";
        } else if (result == -1) {
            response["message"] = "Some registry backups failed. Please check permissions or registry paths.";
            response["status"] = "partial_failure";
        } else {
            response["message"] = "Registry backup failed to start. Unable to create backup directory!";
            response["status"] = "failed";
        }
        
        // 添加备份目录信息
        extern char g_backupDir[];
        response["backupDir"] = std::string(g_backupDir); 
        
        // 添加时间戳
        auto now = std::chrono::system_clock::now();
        response["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        
        std::string responseStr = response.dump();
        std::cout << "Registry backup completed with result: " << result 
                  << ", backup directory: " << response["backupDir"] << std::endl;
        
        res.set_content(responseStr, "application/json");
        
    } catch (const std::exception& e) {
        std::cerr << "HandleSaveRegistry exception: " << e.what() << std::endl;
        json error;
        error["code"] = -2;
        error["error"] = "Failed to execute registry backup: " + std::string(e.what());
        error["status"] = "exception";
        res.status = 500;
        res.set_content(error.dump(), "application/json");
    }
}

void HttpServer::HandleGetRegistryInfo(const httplib::Request& req, httplib::Response& res) {
    try {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        
        // 获取所有备份信息
        auto backupInfoMap = registryMonitor_.GetAllBackupInfo();
        json response;
        
        response["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        // 备份信息数组
        json backupsJson = json::array();
        
        for (const auto& [backupName, backupInfo] : backupInfoMap) {
            json backupJson;
            backupJson["folderName"] = backupInfo.folderName;
            backupJson["folderPath"] = backupInfo.folderPath;
            backupJson["createTime"] = backupInfo.createTime;
            backupJson["totalSize"] = backupInfo.totalSize;
            backupJson["fileCount"] = backupInfo.regFiles.size();
            
            // 注册表文件列表
            json regFilesJson = json::array();
            for (const auto& regFile : backupInfo.regFiles) {
                json fileJson;
                fileJson["fileName"] = regFile.fileName;
                fileJson["fileSize"] = regFile.fileSize;
                
                regFilesJson.push_back(fileJson);
            }
            backupJson["regFiles"] = regFilesJson;
            
            backupsJson.push_back(backupJson);
        }
        
        response["backups"] = backupsJson;
        response["totalBackups"] = backupInfoMap.size();
        
        std::string responseStr = response.dump();
        std::cout << "Returning backup summary, total backups: " << backupInfoMap.size() 
                  << ", timestamp: " << response["timestamp"] << std::endl;
        
        res.set_content(responseStr, "application/json");
        
    } catch (const std::exception& e) {
        std::cerr << "HandleGetRegistrySnapshot exception: " << e.what() << std::endl;
        json error;
        error["error"] = "Failed to get backup information: " + std::string(e.what());
        res.status = 500;
        res.set_content(error.dump(), "application/json");
    }
}

void HttpServer::HandleCompareFolders(const httplib::Request& req, httplib::Response& res) {
    try {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        
        json response;
        
        // 解析JSON请求体
        json jsonBody;
        try {
            jsonBody = json::parse(req.body);
        } catch (const json::parse_error& e) {
            std::cerr << "JSON parse error: " << e.what() << std::endl;
            json error;
            error["error"] = "Invalid JSON format in request body";
            res.status = 400;
            res.set_content(error.dump(), "application/json");
            return;
        }
        
        // 检查参数是否存在
        if (!jsonBody.contains("folder1") || !jsonBody.contains("folder2")) {
            json error;
            error["error"] = "Missing required parameters: folder1 and folder2 are required";
            res.status = 400;
            res.set_content(error.dump(), "application/json");
            return;
        }
        std::string folder1 = jsonBody["folder1"];
        std::string folder2 = jsonBody["folder2"];
        
        // 检查参数是否为空
        if (folder1.empty() || folder2.empty()) {
            json error;
            error["error"] = "Parameters cannot be empty: folder1 and folder2 must not be empty";
            res.status = 400;
            res.set_content(error.dump(), "application/json");
            return;
        }
        std::cout << "Comparing folders: " << folder1 << " and " << folder2 << std::endl;
        
        // 调用文件夹比较函数
        auto result = registryMonitor_.compareFolders(folder1, folder2);
        
        // 构建响应
        response["folder1"] = result.folder1;
        response["folder2"] = result.folder2;
        response["totalComparedFiles"] = result.totalComparedFiles;
        response["totalAddedKeys"] = result.totalAddedKeys;
        response["totalRemovedKeys"] = result.totalRemovedKeys;
        response["totalModifiedKeys"] = result.totalModifiedKeys;
        
        // 只在文件夹1中的文件
        json filesOnlyInFolder1Json = json::array();
        for (const auto& file : result.filesOnlyInFolder1) {
            filesOnlyInFolder1Json.push_back(file);
        }
        response["filesOnlyInFolder1"] = filesOnlyInFolder1Json;
        
        // 只在文件夹2中的文件
        json filesOnlyInFolder2Json = json::array();
        for (const auto& file : result.filesOnlyInFolder2) {
            filesOnlyInFolder2Json.push_back(file);
        }
        response["filesOnlyInFolder2"] = filesOnlyInFolder2Json;
        
        // 比较过的文件对及其差异
        json comparedFilesJson = json::array();
        for (const auto& [prefix, fileResult] : result.comparedFiles) {
            json fileComparisonJson;
            fileComparisonJson["prefix"] = prefix;
            fileComparisonJson["file1"] = fileResult.file1;
            fileComparisonJson["file2"] = fileResult.file2;
            fileComparisonJson["addedKeys"] = fileResult.addedKeys.size();
            fileComparisonJson["removedKeys"] = fileResult.removedKeys.size();
            fileComparisonJson["modifiedKeys"] = fileResult.modifiedKeys.size();
            
            // 详细的键变化信息
            json addedKeysJson = json::array();
            for (const auto& key : fileResult.addedKeys) {
                addedKeysJson.push_back(key);
            }
            fileComparisonJson["addedKeysDetails"] = addedKeysJson;
            
            json removedKeysJson = json::array();
            for (const auto& key : fileResult.removedKeys) {
                removedKeysJson.push_back(key);
            }
            fileComparisonJson["removedKeysDetails"] = removedKeysJson;
            
            json modifiedKeysJson = json::array();
            for (const auto& key : fileResult.modifiedKeys) {
                modifiedKeysJson.push_back(key);
            }
            fileComparisonJson["modifiedKeysDetails"] = modifiedKeysJson;
            
            comparedFilesJson.push_back(fileComparisonJson);
        }
        response["comparedFiles"] = comparedFilesJson;
        
        // 添加时间戳
        auto now = std::chrono::system_clock::now();
        response["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        
        std::string responseStr = response.dump();
        std::cout << "Folder comparison completed, compared " << result.totalComparedFiles 
                  << " file pairs" << std::endl;
        
        res.set_content(responseStr, "application/json");
        
    } catch (const std::exception& e) {
        std::cerr << "HandleCompareFolders exception: " << e.what() << std::endl;
        json error;
        error["error"] = "Failed to compare folders: " + std::string(e.what());
        res.status = 500;
        res.set_content(error.dump(), "application/json");
    }
}

// void HttpServer::HandleSaveRegistrySnapshot(const httplib::Request& req, httplib::Response& res) {
//     try {
//         res.set_header("Access-Control-Allow-Origin", "*");
//         res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
//         res.set_header("Access-Control-Allow-Headers", "Content-Type");
        
//         std::cout << "Received save registry snapshot request" << std::endl;
//         std::cout << "Request body size: " << req.body.size() << " bytes" << std::endl;
        
//         // Parse JSON data
//         json request_json;
//         try {
//             request_json = json::parse(req.body);
//             std::cout << "JSON parsing successful" << std::endl;
//         } catch (const json::exception& e) {
//             std::cerr << "JSON parsing failed: " << e.what() << std::endl;
//             json error;
//             error["success"] = false;
//             error["error"] = "Invalid JSON format";
//             res.status = 400;
//             res.set_content(error.dump(), "application/json");
//             return;
//         }
        
//         // Get snapshot name
//         std::string snapshotName;
//         if (request_json.contains("snapshotName") && request_json["snapshotName"].is_string()) {
//             snapshotName = request_json["snapshotName"];
//         }
//         if (snapshotName.empty()) {
//             snapshotName = "snapshot_" + std::to_string(GET_LOCAL_TIME_MS());
//         }
        
//         // Create snapshot object but don't parse detailed content (avoid encoding issues)
//         RegistrySnapshot snapshot;
//         snapshot.timestamp = GET_LOCAL_TIME_MS();
        
//         // Only record count, don't parse specific content
//         if (request_json.contains("systemInfo") && request_json["systemInfo"].is_array()) {
//             snapshot.systemInfoKeys.resize(request_json["systemInfo"].size());
//         }
        
//         if (request_json.contains("software") && request_json["software"].is_array()) {
//             snapshot.softwareKeys.resize(request_json["software"].size());
//         }
        
//         if (request_json.contains("network") && request_json["network"].is_array()) {
//             snapshot.networkKeys.resize(request_json["network"].size());
//         }
        
//         // Save snapshot
//         registrySnapshots_[snapshotName] = snapshot;
        
//         // Build response - ensure all strings are safe
//         json response;
//         response["success"] = true;
//         response["snapshotName"] = snapshotName; // snapshotName should be safe
//         response["timestamp"] = snapshot.timestamp;
//         response["systemInfoCount"] = snapshot.systemInfoKeys.size();
//         response["softwareCount"] = snapshot.softwareKeys.size();
//         response["networkCount"] = snapshot.networkKeys.size();
//         response["message"] = "Snapshot saved successfully";
        
//         std::cout << "Registry snapshot saved successfully: " << snapshotName 
//                   << " (System: " << snapshot.systemInfoKeys.size()
//                   << ", Software: " << snapshot.softwareKeys.size() 
//                   << ", Network: " << snapshot.networkKeys.size() << ")" << std::endl;
        
//         // Safely set response content
//         std::string response_str;
//         try {
//             response_str = response.dump();
//             res.set_content(response_str, "application/json");
//         } catch (const json::exception& e) {
//             std::cerr << "JSON serialization failed: " << e.what() << std::endl;
//             // Fallback: build a simple response
//             res.set_content("{\"success\":true,\"message\":\"Snapshot saved successfully\"}", "application/json");
//         }
        
//     } catch (const std::exception& e) {
//         std::cerr << "HandleSaveRegistrySnapshot exception: " << e.what() << std::endl;
//         // Use simple error response to avoid JSON issues
//         res.status = 500;
//         res.set_content("{\"success\":false,\"error\":\"Internal server error\"}", "application/json");
//     }
// }

// Add safe JSON parsing method
std::vector<RegistryKey> HttpServer::ParseRegistryKeysFromJson(const json& json_array) {
    std::vector<RegistryKey> keys;
    
    if (!json_array.is_array()) {
        return keys;
    }
    
    for (const auto& item : json_array) {
        try {
            RegistryKey key;
            
            // Safely extract path
            if (item.contains("path") && item["path"].is_string()) {
                key.path = SafeJsonToString(item["path"]);
            }
            
            // Safely extract values
            if (item.contains("values") && item["values"].is_array()) {
                for (const auto& value_item : item["values"]) {
                    RegistryValue value;
                    
                    if (value_item.contains("name") && value_item["name"].is_string()) {
                        value.name = SafeJsonToString(value_item["name"]);
                    }
                    if (value_item.contains("type") && value_item["type"].is_string()) {
                        value.type = SafeJsonToString(value_item["type"]);
                    }
                    if (value_item.contains("data") && !value_item["data"].is_null()) {
                        value.data = SafeJsonToString(value_item["data"]);
                    }
                    if (value_item.contains("size") && value_item["size"].is_number()) {
                        value.size = value_item["size"];
                    }
                    
                    key.values.push_back(value);
                }
            }
            
            // Safely extract subkeys
            if (item.contains("subkeys") && item["subkeys"].is_array()) {
                for (const auto& subkey_item : item["subkeys"]) {
                    if (subkey_item.is_string()) {
                        key.subkeys.push_back(SafeJsonToString(subkey_item));
                    }
                }
            }
            
            keys.push_back(key);
            
        } catch (const std::exception& e) {
            std::cerr << "Failed to parse registry key: " << e.what() << std::endl;
            // Skip problematic key, continue processing next
        }
    }
    
    return keys;
}

// Safely extract string from JSON
std::string HttpServer::SafeJsonToString(const json& json_value) {
    if (json_value.is_null()) {
        return "";
    }
    
    if (json_value.is_string()) {
        try {
            std::string str = json_value.get<std::string>();
            return SanitizeUTF8(str);
        } catch (const std::exception& e) {
            std::cerr << "JSON string conversion failed: " << e.what() << std::endl;
            return "[Invalid String]";
        }
    }
    
    if (json_value.is_number()) {
        return std::to_string(json_value.get<int64_t>());
    }
    
    if (json_value.is_boolean()) {
        return json_value.get<bool>() ? "true" : "false";
    }
    
    return "[Unsupported Type]";
}

// UTF-8 sanitization function (add to HttpServer class)
std::string HttpServer::SanitizeUTF8(const std::string& str) {
    std::string result;
    result.reserve(str.length());
    
    for (size_t i = 0; i < str.length(); ) {
        unsigned char c = static_cast<unsigned char>(str[i]);
        
        if (c <= 0x7F) {
            // ASCII character
            result += c;
            i++;
        } else if ((c & 0xE0) == 0xC0) {
            // 2-byte UTF-8 character
            if (i + 1 < str.length() && (str[i+1] & 0xC0) == 0x80) {
                result += str.substr(i, 2);
                i += 2;
            } else {
                result += '?';
                i++;
            }
        } else if ((c & 0xF0) == 0xE0) {
            // 3-byte UTF-8 character
            if (i + 2 < str.length() && (str[i+1] & 0xC0) == 0x80 && (str[i+2] & 0xC0) == 0x80) {
                result += str.substr(i, 3);
                i += 3;
            } else {
                result += '?';
                i++;
            }
        } else if ((c & 0xF8) == 0xF0) {
            // 4-byte UTF-8 character
            if (i + 3 < str.length() && (str[i+1] & 0xC0) == 0x80 && 
                (str[i+2] & 0xC0) == 0x80 && (str[i+3] & 0xC0) == 0x80) {
                result += str.substr(i, 4);
                i += 4;
            } else {
                result += '?';
                i++;
            }
        } else {
            // Invalid UTF-8 byte
            result += '?';
            i++;
        }
    }
    
    return result;
}

// void HttpServer::HandleGetSavedSnapshots(const httplib::Request& req, httplib::Response& res) {
//     try {
//         res.set_header("Access-Control-Allow-Origin", "*");
        
//         json response = json::array();
        
//         for (const auto& [name, snapshot] : registrySnapshots_) {
//             json snapshotJson;
//             snapshotJson["name"] = name;
//             snapshotJson["timestamp"] = snapshot.timestamp;
//             snapshotJson["systemInfoCount"] = snapshot.systemInfoKeys.size();
//             snapshotJson["softwareCount"] = snapshot.softwareKeys.size();
//             snapshotJson["networkCount"] = snapshot.networkKeys.size();
            
//             response.push_back(snapshotJson);
//         }
        
//         std::cout << "Returning saved snapshot list, count: " << registrySnapshots_.size() << std::endl;
        
//         res.set_content(response.dump(), "application/json");
        
//     } catch (const std::exception& e) {
//         std::cerr << "HandleGetSavedSnapshots exception: " << e.what() << std::endl;
//         json error;
//         error["error"] = "Failed to get snapshot list";
//         res.status = 500;
//         res.set_content(error.dump(), "application/json");
//     }
// }


// Helper function: Compare two registry key lists
json HttpServer::CompareRegistrySnapshots(const std::vector<RegistryKey>& keys1, const std::vector<RegistryKey>& keys2) {
    json changes = json::object();
    changes["added"] = json::array();
    changes["removed"] = json::array();
    changes["modified"] = json::array();
    
    std::map<std::string, RegistryKey> map1, map2;
    
    for (const auto& key : keys1) {
        map1[key.path] = key;
    }
    
    for (const auto& key : keys2) {
        map2[key.path] = key;
    }
    
    // Find added keys
    for (const auto& [path, key] : map2) {
        if (map1.find(path) == map1.end()) {
            changes["added"].push_back(path);
        }
    }
    
    // Find removed keys
    for (const auto& [path, key] : map1) {
        if (map2.find(path) == map2.end()) {
            changes["removed"].push_back(path);
        }
    }
    
    // Find modified keys
    for (const auto& [path, key1] : map1) {
        if (map2.find(path) != map2.end()) {
            const auto& key2 = map2[path];
            if (key1.values != key2.values || key1.subkeys != key2.subkeys) {
                changes["modified"].push_back(path);
            }
        }
    }
    
    return changes;
}
// void HttpServer::HandleCompareSnapshots(const httplib::Request& req, httplib::Response& res) {
//     try {
//         res.set_header("Access-Control-Allow-Origin", "*");
//         res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
//         res.set_header("Access-Control-Allow-Headers", "Content-Type");
//         res.set_header("Content-Type", "application/json; charset=utf-8");
        
//         // Parse JSON request body
//         json request_json;
//         try {
//             request_json = json::parse(req.body);
//         } catch (const json::exception& e) {
//             json error;
//             error["success"] = false;
//             error["error"] = "Invalid JSON format: " + std::string(e.what());
//             res.status = 400;
//             res.set_content(error.dump(), "application/json");
//             return;
//         }
        
//         // Get parameters from JSON
//         std::string snapshot1, snapshot2;
//         if (request_json.contains("snapshot1") && request_json["snapshot1"].is_string()) {
//             snapshot1 = request_json["snapshot1"];
//         }
//         if (request_json.contains("snapshot2") && request_json["snapshot2"].is_string()) {
//             snapshot2 = request_json["snapshot2"];
//         }
        
//         std::cout << "Received compare snapshots request: " << snapshot1 << " vs " << snapshot2 << std::endl;
        
//         if (snapshot1.empty() || snapshot2.empty()) {
//             json error;
//             error["success"] = false;
//             error["error"] = "Two snapshot names are required";
//             res.status = 400;
//             res.set_content(error.dump(), "application/json");
//             return;
//         }
        
//         // Check if snapshots exist
//         if (registrySnapshots_.find(snapshot1) == registrySnapshots_.end()) {
//             json error;
//             error["success"] = false;
//             error["error"] = "Snapshot does not exist: " + snapshot1;
//             res.status = 404;
//             res.set_content(error.dump(), "application/json");
//             return;
//         }
        
//         if (registrySnapshots_.find(snapshot2) == registrySnapshots_.end()) {
//             json error;
//             error["success"] = false;
//             error["error"] = "Snapshot does not exist: " + snapshot2;
//             res.status = 404;
//             res.set_content(error.dump(), "application/json");
//             return;
//         }
        
//         // Get snapshots
//         const auto& snap1 = registrySnapshots_[snapshot1];
//         const auto& snap2 = registrySnapshots_[snapshot2];
        
//         json response;
//         response["snapshot1"] = snapshot1;
//         response["snapshot2"] = snapshot2;
//         response["timestamp1"] = snap1.timestamp;
//         response["timestamp2"] = snap2.timestamp;
        
//         // Compare system information
//         response["systemInfoChanges"] = CompareRegistrySnapshots(snap1.systemInfoKeys, snap2.systemInfoKeys);
//         // Compare software information
//         response["softwareChanges"] = CompareRegistrySnapshots(snap1.softwareKeys, snap2.softwareKeys);
//         // Compare network configuration
//         response["networkChanges"] = CompareRegistrySnapshots(snap1.networkKeys, snap2.networkKeys);
        
//         std::cout << "Snapshot comparison completed: " << snapshot1 << " vs " << snapshot2 << std::endl;
        
//         res.set_content(response.dump(), "application/json");
        
//     } catch (const std::exception& e) {
//         std::cerr << "HandleCompareSnapshots exception: " << e.what() << std::endl;
//         json error;
//         error["error"] = e.what();
//         res.status = 500;
//         res.set_content(error.dump(), "application/json");
//     }
// }

void HttpServer::HandleDeleteSnapshot(const httplib::Request& req, httplib::Response& res) {
    try {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Content-Type", "application/json; charset=utf-8");
        
        // Get snapshot name from path parameter
        if (req.matches.size() < 2) {
            res.status = 400;
            res.set_content(R"({"success":false,"error":"Missing snapshot name parameter"})", "application/json");
            return;
        }
        
        std::string snapshotName = req.matches[1];
        
        std::cout << "Received delete snapshot request, original name: " << snapshotName << std::endl;
        
        if (snapshotName.empty()) {
            res.status = 400;
            res.set_content(R"({"success":false,"error":"Snapshot name cannot be empty"})", "application/json");
            return;
        }
        
        // Clean non-UTF-8 characters from snapshot name
        std::string cleanSnapshotName = SanitizeUTF8(snapshotName);
        std::cout << "Cleaned snapshot name: " << cleanSnapshotName << std::endl;
        
        // Find matching snapshot in map
        bool found = false;
        std::string actualSnapshotName;
        
        for (const auto& [name, snapshot] : registrySnapshots_) {
            std::string cleanName = SanitizeUTF8(name);
            if (cleanName == cleanSnapshotName) {
                actualSnapshotName = name;
                found = true;
                break;
            }
        }
        
        if (found) {
            // Delete using original name
            registrySnapshots_.erase(actualSnapshotName);
            
            // Build JSON response
            json response;
            response["success"] = true;
            response["message"] = "Snapshot deleted successfully";
            
            // Safely serialize JSON
            std::string response_str;
            try {
                response_str = response.dump();
            } catch (const json::exception& e) {
                std::cerr << "JSON serialization failed, using fallback response: " << e.what() << std::endl;
                response_str = R"({"success":true,"message":"Snapshot deleted successfully"})";
            }
            
            res.set_content(response_str, "application/json");
            std::cout << "Snapshot deleted successfully: " << actualSnapshotName << std::endl;
            
        } else {
            // Snapshot does not exist
            json error;
            error["success"] = false;
            error["error"] = "Snapshot does not exist: " + cleanSnapshotName;
            
            std::string error_str;
            try {
                error_str = error.dump();
            } catch (const json::exception& e) {
                std::cerr << "Error JSON serialization failed: " << e.what() << std::endl;
                error_str = R"({"success":false,"error":"Snapshot does not exist"})";
            }
            
            res.status = 404;
            res.set_content(error_str, "application/json");
            std::cout << "Failed to delete snapshot: Snapshot does not exist - " << cleanSnapshotName << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "HandleDeleteSnapshot exception: " << e.what() << std::endl;
        res.status = 500;
        res.set_content(R"({"success":false,"error":"Failed to delete snapshot"})", "application/json");
    }
}

void HttpServer::HandleGetDriverSnapshot(const httplib::Request& req, httplib::Response& res) {
    try {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        
        auto snapshot = driverMonitor_.GetDriverSnapshot();
        json response;
        
        response["timestamp"] = snapshot.timestamp;
        
        // Statistics
        json statsJson;
        statsJson["totalDrivers"] = snapshot.stats.totalDrivers;
        statsJson["runningCount"] = snapshot.stats.runningCount;
        statsJson["stoppedCount"] = snapshot.stats.stoppedCount;
        statsJson["kernelCount"] = snapshot.stats.kernelCount;
        statsJson["fileSystemCount"] = snapshot.stats.fileSystemCount;
        statsJson["autoStartCount"] = snapshot.stats.autoStartCount;
        statsJson["thirdPartyCount"] = snapshot.stats.thirdPartyCount;
        response["statistics"] = statsJson;
        
        // Kernel drivers
        json kernelDriversJson = json::array();
        for (const auto& driver : snapshot.kernelDrivers) {
            json driverJson;
            driverJson["name"] = driver.name;
            driverJson["displayName"] = driver.displayName;
            driverJson["description"] = driver.description;
            driverJson["state"] = driver.state;
            driverJson["startType"] = driver.startType;
            driverJson["binaryPath"] = driver.binaryPath;
            driverJson["serviceType"] = driver.serviceType;
            driverJson["errorControl"] = driver.errorControl;
            driverJson["account"] = driver.account;
            driverJson["driverType"] = driver.driverType;
            driverJson["pid"] = driver.pid;
            
            kernelDriversJson.push_back(driverJson);
        }
        response["kernelDrivers"] = kernelDriversJson;
        
        // File system drivers
        json fileSystemDriversJson = json::array();
        for (const auto& driver : snapshot.fileSystemDrivers) {
            json driverJson;
            driverJson["name"] = driver.name;
            driverJson["displayName"] = driver.displayName;
            driverJson["description"] = driver.description;
            driverJson["state"] = driver.state;
            driverJson["startType"] = driver.startType;
            driverJson["binaryPath"] = driver.binaryPath;
            driverJson["serviceType"] = driver.serviceType;
            driverJson["errorControl"] = driver.errorControl;
            driverJson["account"] = driver.account;
            driverJson["driverType"] = driver.driverType;
            driverJson["pid"] = driver.pid;
            
            fileSystemDriversJson.push_back(driverJson);
        }
        response["fileSystemDrivers"] = fileSystemDriversJson;
        
        // Running drivers
        json runningDriversJson = json::array();
        for (const auto& driver : snapshot.runningDrivers) {
            json driverJson;
            driverJson["name"] = driver.name;
            driverJson["displayName"] = driver.displayName;
            driverJson["state"] = driver.state;
            driverJson["startType"] = driver.startType;
            driverJson["binaryPath"] = driver.binaryPath;
            driverJson["pid"] = driver.pid;
            
            runningDriversJson.push_back(driverJson);
        }
        response["runningDrivers"] = runningDriversJson;
        
        // Stopped drivers
        json stoppedDriversJson = json::array();
        for (const auto& driver : snapshot.stoppedDrivers) {
            json driverJson;
            driverJson["name"] = driver.name;
            driverJson["displayName"] = driver.displayName;
            driverJson["state"] = driver.state;
            driverJson["startType"] = driver.startType;
            driverJson["binaryPath"] = driver.binaryPath;
            
            stoppedDriversJson.push_back(driverJson);
        }
        response["stoppedDrivers"] = stoppedDriversJson;
        
        // Auto-start drivers
        json autoStartDriversJson = json::array();
        for (const auto& driver : snapshot.autoStartDrivers) {
            json driverJson;
            driverJson["name"] = driver.name;
            driverJson["displayName"] = driver.displayName;
            driverJson["state"] = driver.state;
            driverJson["startType"] = driver.startType;
            driverJson["binaryPath"] = driver.binaryPath;
            
            autoStartDriversJson.push_back(driverJson);
        }
        response["autoStartDrivers"] = autoStartDriversJson;
        
        // Third-party drivers
        json thirdPartyDriversJson = json::array();
        for (const auto& driver : snapshot.thirdPartyDrivers) {
            json driverJson;
            driverJson["name"] = driver.name;
            driverJson["displayName"] = driver.displayName;
            driverJson["state"] = driver.state;
            driverJson["startType"] = driver.startType;
            driverJson["binaryPath"] = driver.binaryPath;
            
            thirdPartyDriversJson.push_back(driverJson);
        }
        response["thirdPartyDrivers"] = thirdPartyDriversJson;
        
        std::string responseStr = response.dump();
        std::cout << "Returning driver snapshot - "
                  << "Kernel drivers: " << snapshot.kernelDrivers.size() << ", "
                  << "File system drivers: " << snapshot.fileSystemDrivers.size() << ", "
                  << "Running: " << snapshot.runningDrivers.size() << std::endl;
        
        res.set_content(responseStr, "application/json");
        
    } catch (const std::exception& e) {
        std::cerr << "HandleGetDriverSnapshot exception: " << e.what() << std::endl;
        json error;
        error["error"] = "Failed to get driver snapshot: " + std::string(e.what());
        res.status = 500;
        res.set_content(error.dump(), "application/json");
    }
}

void HttpServer::HandleGetDriverDetail(const httplib::Request& req, httplib::Response& res) {
    try {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        
        std::string driverName = req.get_param_value("name");
        if (driverName.empty()) {
            res.status = 400;
            json error;
            error["error"] = "Missing driver name parameter";
            res.set_content(error.dump(), "application/json");
            return;
        }
        
        auto driverDetail = driverMonitor_.GetDriverDetail(driverName);
        json response;
        
        response["name"] = driverDetail.name;
        response["displayName"] = driverDetail.displayName;
        response["description"] = driverDetail.description;
        response["state"] = driverDetail.state;
        response["startType"] = driverDetail.startType;
        response["binaryPath"] = driverDetail.binaryPath;
        response["serviceType"] = driverDetail.serviceType;
        response["errorControl"] = driverDetail.errorControl;
        response["account"] = driverDetail.account;
        response["group"] = driverDetail.group;
        response["tagId"] = driverDetail.tagId;
        response["driverType"] = driverDetail.driverType;
        response["pid"] = driverDetail.pid;
        response["exitCode"] = driverDetail.exitCode;
        response["win32ExitCode"] = driverDetail.win32ExitCode;
        response["serviceSpecificExitCode"] = driverDetail.serviceSpecificExitCode;
        
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        std::cerr << "HandleGetDriverDetail exception: " << e.what() << std::endl;
        json error;
        error["error"] = "Failed to get driver details: " + std::string(e.what());
        res.status = 404;
        res.set_content(error.dump(), "application/json");
    }
}

void HttpServer::HandleCreateSystemSnapshot(const httplib::Request& req, httplib::Response& res) {
    try {
        auto snapshot = SystemSnapshotCollector::Collect(cpuMonitor_, memoryMonitor_, diskMonitor_, driverMonitor_, registryMonitor_, processMonitor_);
        json out = snapshot.ToJson();

        std::ostringstream nameoss;
        {
            auto now = std::chrono::system_clock::now();
            auto tt = std::chrono::system_clock::to_time_t(now);
            std::tm tm;
            localtime_s(&tm, &tt);
            nameoss << "snapshot_" << std::put_time(&tm, "%Y%m%d_%H%M%S");
        }
        std::string name = nameoss.str();
        if (!req.body.empty()) {
            try {
                auto body = json::parse(req.body);
                if (body.contains("name") && body["name"].is_string()) {
                    name = body["name"];
                }
            } catch (...) {}
        }

        std::string serialized = out.dump();
        {
            std::lock_guard<std::mutex> lk(systemSnapshotsMutex_);
            systemSnapshotsJson_[name] = serialized;
        }

        json resp;
        resp["success"] = true;
        resp["name"] = name;
        resp["timestamp"] = snapshot.timestamp;
        res.set_content(resp.dump(), "application/json");
    } catch (const std::exception& e) {
        json error;
        error["success"] = false;
        error["error"] = e.what();
        res.status = 500;
        res.set_content(error.dump(), "application/json");
    }
}

void HttpServer::LoadSnapshotsFromDisk() {
    if (snapshotsLoadedFromDisk_) return;
    snapshotsLoadedFromDisk_ = true;

    try {
        if (!snapshotStore_) snapshotStore_ = std::make_unique<snapshot::SnapshotManager>();
        auto files = snapshotStore_->List();
        for (const auto& name : files) {
            auto opt = snapshotStore_->Load(name);
            if (opt) {
                std::lock_guard<std::mutex> lk(systemSnapshotsMutex_);
                systemSnapshotsJson_[name] = *opt;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "LoadSnapshotsFromDisk failed: " << e.what() << std::endl;
    }
}

void HttpServer::HandleGetSystemSnapshot(const httplib::Request& req, httplib::Response& res) {
    try {
        std::string name;
        if (req.has_param("name")) name = req.get_param_value("name");
        if (name.empty() && req.matches.size() >= 2) name = req.matches[1];
        if (name.empty()) {
            res.status = 400;
            res.set_content(R"({"success":false,"error":"Missing snapshot name"})", "application/json");
            return;
        }

        LoadSnapshotsFromDisk();

        std::string payload;
        {
            std::lock_guard<std::mutex> lk(systemSnapshotsMutex_);
            auto it = systemSnapshotsJson_.find(name);
            if (it != systemSnapshotsJson_.end()) payload = it->second;
        }

        if (payload.empty() && snapshotStore_) {
            auto opt = snapshotStore_->Load(name);
            if (opt) payload = *opt;
        }

        if (payload.empty()) {
            res.status = 404;
            res.set_content(R"({"success":false,"error":"Snapshot not found"})", "application/json");
            return;
        }

        res.set_content(payload, "application/json");
    } catch (const std::exception& e) {
        res.status = 500;
        res.set_content(R"({"success":false,"error":"Internal error"})", "application/json");
    }
}

void HttpServer::HandleListSystemSnapshots(const httplib::Request& req, httplib::Response& res) {
    try {
        LoadSnapshotsFromDisk();

        // 收集快照名称和JSON数据
        std::vector<std::pair<std::string, std::string>> snapshots;
        {
            std::lock_guard<std::mutex> lk(systemSnapshotsMutex_);
            snapshots.reserve(systemSnapshotsJson_.size());
            for (const auto &kv : systemSnapshotsJson_) {
                snapshots.push_back({kv.first, kv.second});
            }
        }

        // 按名称降序排序
        std::sort(snapshots.begin(), snapshots.end(), 
                  [](const auto& a, const auto& b) { return a.first > b.first; });

        // 构建返回的JSON数组，包含名称和时间戳
        json arr = json::array();
        for (const auto &[name, jsonStr] : snapshots) {
            json item;
            item["name"] = name;
            
            // 尝试解析JSON获取时间戳
            try {
                json snapshotData = json::parse(jsonStr);
                if (snapshotData.contains("snapshotTimestamp")) {
                    item["timestamp"] = snapshotData["snapshotTimestamp"];
                } else {
                    item["timestamp"] = 0;
                }
            } catch (const json::exception& e) {
                // 解析失败时设置默认值
                std::cerr << "Failed to parse snapshot JSON for " << name << ": " << e.what() << std::endl;
                item["timestamp"] = 0;
            }
            
            arr.push_back(item);
        }

        res.set_content(arr.dump(), "application/json");
    } catch (const std::exception& e) {
        json error;
        error["error"] = e.what();
        res.status = 500;
        res.set_content(error.dump(), "application/json");
    }
}

void HttpServer::HandleSaveSystemSnapshot(const httplib::Request& req, httplib::Response& res) {
    try {
        std::string name;

        if (req.has_param("name")) {
            name = req.get_param_value("name");
        }

        if (name.empty() && !req.body.empty()) {
            try {
                auto body = json::parse(req.body);
                if (body.is_object() && body.contains("name") && body["name"].is_string()) {
                    name = body["name"].get<std::string>();
                }
            } catch (const json::exception& e) {
                snapshot::Logger::getInstance().log(snapshot::LogLevel::WARNING, __FILE__, __LINE__, "HandleSaveSystemSnapshot: json parse failed: ", e.what());
            }
        }

        if (name.empty()) {
            auto now = std::chrono::system_clock::now();
            auto tt = std::chrono::system_clock::to_time_t(now);
            std::tm tm;
            localtime_s(&tm, &tt);
            std::ostringstream oss;
            oss << "snapshot_" << std::put_time(&tm, "%Y%m%d_%H%M%S");
            name = oss.str();
        }

        std::string payload;
        {
            std::lock_guard<std::mutex> lk(systemSnapshotsMutex_);
            auto it = systemSnapshotsJson_.find(name);
            if (it != systemSnapshotsJson_.end()) payload = it->second;
        }

        if (payload.empty()) {
            auto snapshot = SystemSnapshotCollector::Collect(cpuMonitor_, memoryMonitor_, diskMonitor_, driverMonitor_, registryMonitor_, processMonitor_);
            payload = snapshot.ToJson().dump();
        }

        bool ok = false;
        try {
            if (!snapshotStore_) snapshotStore_ = std::make_unique<snapshot::SnapshotManager>();
            ok = snapshotStore_->Save(name, payload);
            if (ok) {
                std::lock_guard<std::mutex> lk(systemSnapshotsMutex_);
                systemSnapshotsJson_[name] = payload;
            }
        } catch (const std::exception& e) {
            snapshot::Logger::getInstance().log(snapshot::LogLevel::E_ERROR, __FILE__, __LINE__, "HandleSaveSystemSnapshot: exception saving snapshot: ", e.what());
            ok = false;
        }

        json resp;
        resp["success"] = ok;
        resp["name"] = name;
        res.set_content(resp.dump(), "application/json");
    } catch (const std::exception& e) {
        json error; error["error"] = e.what(); res.status = 500; res.set_content(error.dump(), "application/json");
    }
}

void HttpServer::HandleDeleteSystemSnapshot(const httplib::Request& req, httplib::Response& res) {
    try {
        if (req.matches.size() < 2) {
            res.status = 400;
            res.set_content(R"({"success":false,"error":"Missing snapshot name parameter"})", "application/json");
            return;
        }

        std::string snapshotName = req.matches[1];
        if (snapshotName.empty()) {
            res.status = 400;
            res.set_content(R"({"success":false,"error":"Snapshot name cannot be empty"})", "application/json");
            return;
        }

        std::string sanitized = SanitizeUTF8(snapshotName);

        bool removedInMemory = false;
        {
            std::lock_guard<std::mutex> lk(systemSnapshotsMutex_);
            auto it = systemSnapshotsJson_.find(snapshotName);
            if (it != systemSnapshotsJson_.end()) {
                systemSnapshotsJson_.erase(it);
                removedInMemory = true;
            } else {
                for (auto iter = systemSnapshotsJson_.begin(); iter != systemSnapshotsJson_.end(); ++iter) {
                    if (SanitizeUTF8(iter->first) == sanitized) {
                        systemSnapshotsJson_.erase(iter);
                        removedInMemory = true;
                        break;
                    }
                }
            }
        }

        bool removedOnDisk = false;
        if (snapshotStore_) {
            try {
                removedOnDisk = snapshotStore_->Delete(snapshotName) || snapshotStore_->Delete(sanitized);
            } catch (const std::exception& e) {
                std::cerr << "SnapshotManager::Delete threw: " << e.what() << std::endl;
            }
        }

        if (!removedInMemory && !removedOnDisk) {
            res.status = 404;
            res.set_content(R"({"success":false,"error":"Snapshot not found"})", "application/json");
            return;
        }

        json resp;
        resp["success"] = true;
        resp["deletedInMemory"] = removedInMemory;
        resp["deletedOnDisk"] = removedOnDisk;
        std::string out;
        try { out = resp.dump(); } catch (...) { out = "{\"success\":true}"; }
        res.set_content(out, "application/json");

    } catch (const std::exception& e) {
        std::cerr << "HandleDeleteSystemSnapshot exception: " << e.what() << std::endl;
        res.status = 500;
        res.set_content(R"({"success":false,"error":"Internal server error"})", "application/json");
    }
}

// 详细比较两个系统快照JSON
json HttpServer::CompareSystemSnapshotsJson(const json& snap1, const json& snap2, const std::string& name1, const std::string& name2) {
    json result;
    result["snapshot1"] = name1;
    result["snapshot2"] = name2;
    result["timestamp1"] = snap1.value("timestamp", 0);
    result["timestamp2"] = snap2.value("timestamp", 0);

    // ============ CPU 比较 ============
    result["cpu"] = json::object();
    if (snap1.contains("cpu") && snap2.contains("cpu")) {
        auto cpu1 = snap1["cpu"];
        auto cpu2 = snap2["cpu"];
        
        result["cpu"]["totalUsageDiff"] = cpu2.value("totalUsage", 0.0) - cpu1.value("totalUsage", 0.0);
        result["cpu"]["totalUsage1"] = cpu1.value("totalUsage", 0.0);
        result["cpu"]["totalUsage2"] = cpu2.value("totalUsage", 0.0);
        
        if (cpu1.contains("coreUsages") && cpu2.contains("coreUsages") &&
            cpu1["coreUsages"].is_array() && cpu2["coreUsages"].is_array()) {
            auto cores1 = cpu1["coreUsages"];
            auto cores2 = cpu2["coreUsages"];
            size_t minSize = (cores1.size() < cores2.size()) ? cores1.size() : cores2.size();
            
            json coreDiffs = json::array();
            for (size_t i = 0; i < minSize; ++i) {
                json coreDiff;
                coreDiff["coreIndex"] = i;
                coreDiff["usage1"] = cores1[i].get<double>();
                coreDiff["usage2"] = cores2[i].get<double>();
                coreDiff["diff"] = cores2[i].get<double>() - cores1[i].get<double>();
                coreDiffs.push_back(coreDiff);
            }
            result["cpu"]["coreUsageDiffs"] = coreDiffs;
        }
    }

    // ============ 内存比较 ============
    result["memory"] = json::object();
    if (snap1.contains("memory") && snap2.contains("memory")) {
        auto mem1 = snap1["memory"];
        auto mem2 = snap2["memory"];
        
        uint64_t totalPhysical1 = mem1.value("totalPhysical", static_cast<uint64_t>(0));
        uint64_t totalPhysical2 = mem2.value("totalPhysical", static_cast<uint64_t>(0));
        result["memory"]["totalPhysical1"] = totalPhysical1;
        result["memory"]["totalPhysical2"] = totalPhysical2;
        result["memory"]["totalPhysicalDiff"] = static_cast<int64_t>(totalPhysical2) - static_cast<int64_t>(totalPhysical1);
        
        uint64_t availablePhysical1 = mem1.value("availablePhysical", static_cast<uint64_t>(0));
        uint64_t availablePhysical2 = mem2.value("availablePhysical", static_cast<uint64_t>(0));
        result["memory"]["availablePhysical1"] = availablePhysical1;
        result["memory"]["availablePhysical2"] = availablePhysical2;
        result["memory"]["availablePhysicalDiff"] = static_cast<int64_t>(availablePhysical2) - static_cast<int64_t>(availablePhysical1);
        
        uint64_t usedPhysical1 = mem1.value("usedPhysical", static_cast<uint64_t>(0));
        uint64_t usedPhysical2 = mem2.value("usedPhysical", static_cast<uint64_t>(0));
        result["memory"]["usedPhysical1"] = usedPhysical1;
        result["memory"]["usedPhysical2"] = usedPhysical2;
        result["memory"]["usedPhysicalDiff"] = static_cast<int64_t>(usedPhysical2) - static_cast<int64_t>(usedPhysical1);
        
        result["memory"]["usedPercent1"] = mem1.value("usedPercent", 0.0);
        result["memory"]["usedPercent2"] = mem2.value("usedPercent", 0.0);
        result["memory"]["usedPercentDiff"] = mem2.value("usedPercent", 0.0) - mem1.value("usedPercent", 0.0);
    }

    // ============ 磁盘比较 ============
    result["disk"] = json::object();
    if (snap1.contains("disk") && snap2.contains("disk")) {
        auto disk1 = snap1["disk"];
        auto disk2 = snap2["disk"];
        
        // 比较驱动器 (drives)
        result["disk"]["drives"] = json::object();
        if (disk1.contains("drives") && disk2.contains("drives")) {
            std::map<std::string, json> drives1Map, drives2Map;
            for (const auto& d : disk1["drives"]) {
                if (d.contains("deviceId")) {
                    drives1Map[d["deviceId"]] = d;
                }
            }
            for (const auto& d : disk2["drives"]) {
                if (d.contains("deviceId")) {
                    drives2Map[d["deviceId"]] = d;
                }
            }
            
            json driveChanges = json::array();
            json added = json::array();
            json removed = json::array();
            json modified = json::array();
            
            // 检查添加和修改的驱动器
            for (const auto& [deviceId, d2] : drives2Map) {
                if (drives1Map.find(deviceId) == drives1Map.end()) {
                    // 新增驱动器
                    json add;
                    add["deviceId"] = deviceId;
                    add["model"] = d2.value("model", "");
                    add["serialNumber"] = d2.value("serialNumber", "");
                    add["interfaceType"] = d2.value("interfaceType", "");
                    add["mediaType"] = d2.value("mediaType", "");
                    add["totalSize"] = d2.value("totalSize", static_cast<uint64_t>(0));  // 使用 uint64_t
                    add["status"] = d2.value("status", "");
                    added.push_back(add);
                } else {
                    // 比较变化
                    const auto& d1 = drives1Map[deviceId];
                    json change;
                    change["deviceId"] = deviceId;
                    change["model"] = d2.value("model", "");
                    
                    bool hasChange = false;
                    if (d1.value("status", "") != d2.value("status", "")) {
                        change["statusChanged"] = true;
                        change["status1"] = d1.value("status", "");
                        change["status2"] = d2.value("status", "");
                        hasChange = true;
                    }
                    
                    uint64_t size1 = d1.value("totalSize", static_cast<uint64_t>(0));
                    uint64_t size2 = d2.value("totalSize", static_cast<uint64_t>(0));
                    if (size1 != size2) {
                        change["totalSizeChanged"] = true;
                        change["totalSize1"] = size1;
                        change["totalSize2"] = size2;
                        hasChange = true;
                    }
                    
                    if (hasChange) {
                        modified.push_back(change);
                    }
                }
            }
            
            // 检查删除的驱动器
            for (const auto& [deviceId, d1] : drives1Map) {
                if (drives2Map.find(deviceId) == drives2Map.end()) {
                    json rem;
                    rem["deviceId"] = deviceId;
                    rem["model"] = d1.value("model", "");
                    removed.push_back(rem);
                }
            }
            
            result["disk"]["drives"]["added"] = added;
            result["disk"]["drives"]["removed"] = removed;
            result["disk"]["drives"]["modified"] = modified;
            result["disk"]["drives"]["addedCount"] = added.size();
            result["disk"]["drives"]["removedCount"] = removed.size();
            result["disk"]["drives"]["modifiedCount"] = modified.size();
        }
        
        // 比较分区 (partitions)
        result["disk"]["partitions"] = json::object();
        if (disk1.contains("partitions") && disk2.contains("partitions")) {
            std::map<std::string, json> parts1Map, parts2Map;
            for (const auto& p : disk1["partitions"]) {
                if (p.contains("driveLetter")) {
                    parts1Map[p["driveLetter"]] = p;
                }
            }
            for (const auto& p : disk2["partitions"]) {
                if (p.contains("driveLetter")) {
                    parts2Map[p["driveLetter"]] = p;
                }
            }
            
            json partitionChanges = json::array();
            for (const auto& [letter, p1] : parts1Map) {
                if (parts2Map.find(letter) != parts2Map.end()) {
                    const auto& p2 = parts2Map[letter];
                    json change;
                    change["driveLetter"] = letter;
                    change["label"] = p2.value("label", "");
                    change["fileSystem"] = p2.value("fileSystem", "");
                    
                    uint64_t totalSize1 = p1.value("totalSize", static_cast<uint64_t>(0));
                    uint64_t totalSize2 = p2.value("totalSize", static_cast<uint64_t>(0));
                    change["totalSize1"] = totalSize1;
                    change["totalSize2"] = totalSize2;
                    change["totalSizeDiff"] = static_cast<int64_t>(totalSize2) - static_cast<int64_t>(totalSize1);
                    
                    uint64_t freeSpace1 = p1.value("freeSpace", static_cast<uint64_t>(0));
                    uint64_t freeSpace2 = p2.value("freeSpace", static_cast<uint64_t>(0));
                    change["freeSpace1"] = freeSpace1;
                    change["freeSpace2"] = freeSpace2;
                    change["freeSpaceDiff"] = static_cast<int64_t>(freeSpace2) - static_cast<int64_t>(freeSpace1);
                    
                    uint64_t usedSpace1 = p1.value("usedSpace", static_cast<uint64_t>(0));
                    uint64_t usedSpace2 = p2.value("usedSpace", static_cast<uint64_t>(0));
                    change["usedSpace1"] = usedSpace1;
                    change["usedSpace2"] = usedSpace2;
                    change["usedSpaceDiff"] = static_cast<int64_t>(usedSpace2) - static_cast<int64_t>(usedSpace1);
                    
                    change["usagePercentage1"] = p1.value("usagePercentage", 0.0);
                    change["usagePercentage2"] = p2.value("usagePercentage", 0.0);
                    change["usagePercentageDiff"] = p2.value("usagePercentage", 0.0) - p1.value("usagePercentage", 0.0);
                    
                    partitionChanges.push_back(change);
                }
            }
            
            result["disk"]["partitions"]["changes"] = partitionChanges;
        }
        
        // 比较性能数据 (performance)
        result["disk"]["performance"] = json::object();
        if (disk1.contains("performance") && disk2.contains("performance")) {
            std::map<std::string, json> perf1Map, perf2Map;
            for (const auto& p : disk1["performance"]) {
                if (p.contains("driveLetter")) {
                    perf1Map[p["driveLetter"]] = p;
                }
            }
            for (const auto& p : disk2["performance"]) {
                if (p.contains("driveLetter")) {
                    perf2Map[p["driveLetter"]] = p;
                }
            }
            
            json perfChanges = json::array();
            for (const auto& [letter, p1] : perf1Map) {
                if (perf2Map.find(letter) != perf2Map.end()) {
                    const auto& p2 = perf2Map[letter];
                    json change;
                    change["driveLetter"] = letter;
                    
                    change["readSpeed1"] = p1.value("readSpeed", 0.0);
                    change["readSpeed2"] = p2.value("readSpeed", 0.0);
                    change["readSpeedDiff"] = p2.value("readSpeed", 0.0) - p1.value("readSpeed", 0.0);
                    
                    change["writeSpeed1"] = p1.value("writeSpeed", 0.0);
                    change["writeSpeed2"] = p2.value("writeSpeed", 0.0);
                    change["writeSpeedDiff"] = p2.value("writeSpeed", 0.0) - p1.value("writeSpeed", 0.0);
                    
                    uint64_t readBytes1 = p1.value("readBytesPerSec", static_cast<uint64_t>(0));
                    uint64_t readBytes2 = p2.value("readBytesPerSec", static_cast<uint64_t>(0));
                    change["readBytesPerSec1"] = readBytes1;
                    change["readBytesPerSec2"] = readBytes2;
                    change["readBytesPerSecDiff"] = static_cast<int64_t>(readBytes2) - static_cast<int64_t>(readBytes1);
                    
                    uint64_t writeBytes1 = p1.value("writeBytesPerSec", static_cast<uint64_t>(0));
                    uint64_t writeBytes2 = p2.value("writeBytesPerSec", static_cast<uint64_t>(0));
                    change["writeBytesPerSec1"] = writeBytes1;
                    change["writeBytesPerSec2"] = writeBytes2;
                    change["writeBytesPerSecDiff"] = static_cast<int64_t>(writeBytes2) - static_cast<int64_t>(writeBytes1);
                    
                    change["queueLength1"] = p1.value("queueLength", 0.0);
                    change["queueLength2"] = p2.value("queueLength", 0.0);
                    change["queueLengthDiff"] = p2.value("queueLength", 0.0) - p1.value("queueLength", 0.0);
                    
                    change["usagePercentage1"] = p1.value("usagePercentage", 0.0);
                    change["usagePercentage2"] = p2.value("usagePercentage", 0.0);
                    change["usagePercentageDiff"] = p2.value("usagePercentage", 0.0) - p1.value("usagePercentage", 0.0);
                    
                    change["responseTime1"] = p1.value("responseTime", 0.0);
                    change["responseTime2"] = p2.value("responseTime", 0.0);
                    change["responseTimeDiff"] = p2.value("responseTime", 0.0) - p1.value("responseTime", 0.0);
                    
                    perfChanges.push_back(change);
                }
            }
            
            result["disk"]["performance"]["changes"] = perfChanges;
        }
        
        // 比较SMART数据 (smart)
        result["disk"]["smart"] = json::object();
        if (disk1.contains("smart") && disk2.contains("smart")) {
            std::map<std::string, json> smart1Map, smart2Map;
            for (const auto& s : disk1["smart"]) {
                if (s.contains("deviceId")) {
                    smart1Map[s["deviceId"]] = s;
                }
            }
            for (const auto& s : disk2["smart"]) {
                if (s.contains("deviceId")) {
                    smart2Map[s["deviceId"]] = s;
                }
            }
            
            json smartChanges = json::array();
            for (const auto& [deviceId, s1] : smart1Map) {
                if (smart2Map.find(deviceId) != smart2Map.end()) {
                    const auto& s2 = smart2Map[deviceId];
                    json change;
                    change["deviceId"] = deviceId;
                    
                    change["temperature1"] = s1.value("temperature", 0);
                    change["temperature2"] = s2.value("temperature", 0);
                    change["temperatureDiff"] = s2.value("temperature", 0) - s1.value("temperature", 0);
                    
                    change["healthStatus1"] = s1.value("healthStatus", 0);
                    change["healthStatus2"] = s2.value("healthStatus", 0);
                    change["healthStatusDiff"] = s2.value("healthStatus", 0) - s1.value("healthStatus", 0);
                    
                    change["powerOnHours1"] = s1.value("powerOnHours", 0);
                    change["powerOnHours2"] = s2.value("powerOnHours", 0);
                    change["powerOnHoursDiff"] = s2.value("powerOnHours", 0) - s1.value("powerOnHours", 0);
                    
                    change["badSectors1"] = s1.value("badSectors", 0);
                    change["badSectors2"] = s2.value("badSectors", 0);
                    change["badSectorsDiff"] = s2.value("badSectors", 0) - s1.value("badSectors", 0);
                    
                    change["readErrorCount1"] = s1.value("readErrorCount", 0);
                    change["readErrorCount2"] = s2.value("readErrorCount", 0);
                    change["readErrorCountDiff"] = s2.value("readErrorCount", 0) - s1.value("readErrorCount", 0);
                    
                    change["writeErrorCount1"] = s1.value("writeErrorCount", 0);
                    change["writeErrorCount2"] = s2.value("writeErrorCount", 0);
                    change["writeErrorCountDiff"] = s2.value("writeErrorCount", 0) - s1.value("writeErrorCount", 0);
                    
                    change["overallHealth1"] = s1.value("overallHealth", "");
                    change["overallHealth2"] = s2.value("overallHealth", "");
                    
                    smartChanges.push_back(change);
                }
            }
            
            result["disk"]["smart"]["changes"] = smartChanges;
        }
    }

    // ============ 驱动程序比较 ============
    result["drivers"] = json::object();
    if (snap1.contains("driver") && snap2.contains("driver")) {
        auto drv1 = snap1["driver"];
        auto drv2 = snap2["driver"];
        
        // 比较统计数据
        if (drv1.contains("stats") && drv2.contains("stats")) {
            auto stats1 = drv1["stats"];
            auto stats2 = drv2["stats"];
            
            result["drivers"]["totalDrivers1"] = stats1.value("totalDrivers", 0);
            result["drivers"]["totalDrivers2"] = stats2.value("totalDrivers", 0);
            result["drivers"]["totalDriversDiff"] = stats2.value("totalDrivers", 0) - stats1.value("totalDrivers", 0);
            
            result["drivers"]["runningCount1"] = stats1.value("runningCount", 0);
            result["drivers"]["runningCount2"] = stats2.value("runningCount", 0);
            result["drivers"]["runningCountDiff"] = stats2.value("runningCount", 0) - stats1.value("runningCount", 0);
            
            result["drivers"]["stoppedCount1"] = stats1.value("stoppedCount", 0);
            result["drivers"]["stoppedCount2"] = stats2.value("stoppedCount", 0);
            result["drivers"]["stoppedCountDiff"] = stats2.value("stoppedCount", 0) - stats1.value("stoppedCount", 0);
        }
        
        // 比较运行中的驱动程序详情
        if (drv1.contains("runningDrivers") && drv2.contains("runningDrivers")) {
            std::map<std::string, json> running1Map, running2Map;
            for (const auto& d : drv1["runningDrivers"]) {
                if (d.contains("name")) {
                    running1Map[d["name"]] = d;
                }
            }
            for (const auto& d : drv2["runningDrivers"]) {
                if (d.contains("name")) {
                    running2Map[d["name"]] = d;
                }
            }
            
            json added = json::array();
            json removed = json::array();
            
            // 新启动的驱动
            for (const auto& [name, d2] : running2Map) {
                if (running1Map.find(name) == running1Map.end()) {
                    json add;
                    add["name"] = name;
                    add["displayName"] = d2.value("displayName", "");
                    add["description"] = d2.value("description", "");
                    add["state"] = d2.value("state", "");
                    add["startType"] = d2.value("startType", "");
                    add["binaryPath"] = d2.value("binaryPath", "");
                    added.push_back(add);
                }
            }
            
            // 停止的驱动
            for (const auto& [name, d1] : running1Map) {
                if (running2Map.find(name) == running2Map.end()) {
                    json rem;
                    rem["name"] = name;
                    rem["displayName"] = d1.value("displayName", "");
                    removed.push_back(rem);
                }
            }
            
            result["drivers"]["runningDrivers"] = json::object();
            result["drivers"]["runningDrivers"]["added"] = added;
            result["drivers"]["runningDrivers"]["removed"] = removed;
            result["drivers"]["runningDrivers"]["addedCount"] = added.size();
            result["drivers"]["runningDrivers"]["removedCount"] = removed.size();
        }
    }

    // ============ 注册表比较 ============
    result["registry"] = json::object();
    if (snap1.contains("registry") && snap2.contains("registry")) {
        auto reg1 = snap1["registry"];
        auto reg2 = snap2["registry"];
        
        std::string folder1, folder2;
        if (reg1.contains("backupInfo") && reg1["backupInfo"].contains("folderName")) {
            folder1 = reg1["backupInfo"]["folderName"].get<std::string>();
        }
        if (reg2.contains("backupInfo") && reg2["backupInfo"].contains("folderName")) {
            folder2 = reg2["backupInfo"]["folderName"].get<std::string>();
        }
        
        result["registry"]["folderName1"] = folder1;
        result["registry"]["folderName2"] = folder2;
        
        if (!folder1.empty() || !folder2.empty())
        {
            auto diff = registryMonitor_.compareFolders(folder1, folder2);
            result["registry"]["totalComparedFiles"] = diff.totalComparedFiles;
            result["registry"]["totalAddedKeys"] = diff.totalAddedKeys;
            result["registry"]["totalRemovedKeys"] = diff.totalRemovedKeys;
            result["registry"]["totalModifiedKeys"] = diff.totalModifiedKeys;

            json filesOnlyInFolder1Json = json::array();
            for (const auto& file : diff.filesOnlyInFolder1) {
                filesOnlyInFolder1Json.push_back(file);
            }
            result["registry"]["filesOnlyInFolder1"] = filesOnlyInFolder1Json;

            json filesOnlyInFolder2Json = json::array();
            for (const auto& file : diff.filesOnlyInFolder2) {
                filesOnlyInFolder2Json.push_back(file);
            }
            result["registry"]["filesOnlyInFolder2"] = filesOnlyInFolder2Json;

            json comparedFilesJson = json::array();
            for (const auto& [prefix, fileResult] : diff.comparedFiles) {
                json fileComparisonJson;
                fileComparisonJson["prefix"] = prefix;
                fileComparisonJson["file1"] = fileResult.file1;
                fileComparisonJson["file2"] = fileResult.file2;
                fileComparisonJson["addedKeys"] = fileResult.addedKeys.size();
                fileComparisonJson["removedKeys"] = fileResult.removedKeys.size();
                fileComparisonJson["modifiedKeys"] = fileResult.modifiedKeys.size();
                
                json addedKeysJson = json::array();
                for (const auto& key : fileResult.addedKeys) {
                    addedKeysJson.push_back(key);
                }
                fileComparisonJson["addedKeysDetails"] = addedKeysJson;
                
                json removedKeysJson = json::array();
                for (const auto& key : fileResult.removedKeys) {
                    removedKeysJson.push_back(key);
                }
                fileComparisonJson["removedKeysDetails"] = removedKeysJson;
                
                json modifiedKeysJson = json::array();
                for (const auto& key : fileResult.modifiedKeys) {
                    modifiedKeysJson.push_back(key);
                }
                fileComparisonJson["modifiedKeysDetails"] = modifiedKeysJson;
                
                comparedFilesJson.push_back(fileComparisonJson);
            }
            result["registry"]["comparedFiles"] = comparedFilesJson;
        }
    }

    // ============ 进程比较 ============
    result["processes"] = json::object();
    if (snap1.contains("processes") && snap2.contains("processes")) {
        auto proc1 = snap1["processes"];
        auto proc2 = snap2["processes"];
        
        result["processes"]["totalProcesses1"] = proc1.value("totalProcesses", 0);
        result["processes"]["totalProcesses2"] = proc2.value("totalProcesses", 0);
        result["processes"]["totalProcessesDiff"] = proc2.value("totalProcesses", 0) - proc1.value("totalProcesses", 0);
        
        result["processes"]["totalThreads1"] = proc1.value("totalThreads", 0);
        result["processes"]["totalThreads2"] = proc2.value("totalThreads", 0);
        result["processes"]["totalThreadsDiff"] = proc2.value("totalThreads", 0) - proc1.value("totalThreads", 0);
        
        result["processes"]["totalHandles1"] = proc1.value("totalHandles", 0);
        result["processes"]["totalHandles2"] = proc2.value("totalHandles", 0);
        result["processes"]["totalHandlesDiff"] = proc2.value("totalHandles", 0) - proc1.value("totalHandles", 0);
        
        if (proc1.contains("processes") && proc2.contains("processes") &&
            proc1["processes"].is_array() && proc2["processes"].is_array()) {
            
            std::map<int, json> procs1Map, procs2Map;
            for (const auto& p : proc1["processes"]) {
                if (p.contains("pid")) {
                    procs1Map[p["pid"]] = p;
                }
            }
            for (const auto& p : proc2["processes"]) {
                if (p.contains("pid")) {
                    procs2Map[p["pid"]] = p;
                }
            }
            
            json added = json::array();
            json removed = json::array();
            json changed = json::array();
            
            // 新增的进程
            for (const auto& [pid, proc] : procs2Map) {
                if (procs1Map.find(pid) == procs1Map.end()) {
                    json p;
                    p["pid"] = pid;
                    p["name"] = proc.value("name", "");
                    p["fullPath"] = proc.value("fullPath", "");
                    p["cpuUsage"] = proc.value("cpuUsage", 0.0);
                    p["memoryUsage"] = proc.value("memoryUsage", 0);
                    p["threadCount"] = proc.value("threadCount", 0);
                    added.push_back(p);
                }
            }
            
            // 删除和变化的进程
            for (const auto& [pid, proc1] : procs1Map) {
                if (procs2Map.find(pid) == procs2Map.end()) {
                    // 已删除
                    json p;
                    p["pid"] = pid;
                    p["name"] = proc1.value("name", "");
                    removed.push_back(p);
                } else {
                    // 比较变化
                    const auto& proc2 = procs2Map[pid];
                    json change;
                    change["pid"] = pid;
                    change["name"] = proc2.value("name", "");
                    
                    bool hasChange = false;
                    
                    double cpu1 = proc1.value("cpuUsage", 0.0);
                    double cpu2 = proc2.value("cpuUsage", 0.0);
                    if (std::abs(cpu2 - cpu1) > 0.01) { // 只记录有意义的变化
                        change["cpuUsage1"] = cpu1;
                        change["cpuUsage2"] = cpu2;
                        change["cpuUsageDiff"] = cpu2 - cpu1;
                        hasChange = true;
                    }
                    
                    uint64_t mem1 = proc1.value("memoryUsage", static_cast<uint64_t>(0));
                    uint64_t mem2 = proc2.value("memoryUsage", static_cast<uint64_t>(0));
                    if (mem1 != mem2) {
                        change["memoryUsage1"] = mem1;
                        change["memoryUsage2"] = mem2;
                        change["memoryUsageDiff"] = static_cast<int64_t>(mem2) - static_cast<int64_t>(mem1);
                        hasChange = true;
                    }
                    
                    int threads1 = proc1.value("threadCount", 0);
                    int threads2 = proc2.value("threadCount", 0);
                    if (threads1 != threads2) {
                        change["threadCount1"] = threads1;
                        change["threadCount2"] = threads2;
                        change["threadCountDiff"] = threads2 - threads1;
                        hasChange = true;
                    }
                    
                    uint64_t workingSet1 = proc1.value("workingSetSize", static_cast<uint64_t>(0));
                    uint64_t workingSet2 = proc2.value("workingSetSize", static_cast<uint64_t>(0));
                    if (workingSet1 != workingSet2) {
                        change["workingSetSize1"] = workingSet1;
                        change["workingSetSize2"] = workingSet2;
                        change["workingSetSizeDiff"] = static_cast<int64_t>(workingSet2) - static_cast<int64_t>(workingSet1);
                        hasChange = true;
                    }
                    
                    int handles1 = proc1.value("handleCount", 0);
                    int handles2 = proc2.value("handleCount", 0);
                    if (handles1 != handles2) {
                        change["handleCount1"] = handles1;
                        change["handleCount2"] = handles2;
                        change["handleCountDiff"] = handles2 - handles1;
                        hasChange = true;
                    }
                    
                    if (hasChange) {
                        changed.push_back(change);
                    }
                }
            }
            
            result["processes"]["added"] = added;
            result["processes"]["removed"] = removed;
            result["processes"]["changed"] = changed;
            result["processes"]["addedCount"] = added.size();
            result["processes"]["removedCount"] = removed.size();
            result["processes"]["changedCount"] = changed.size();
        }
    }

    return result;
}

void HttpServer::HandleCompareSystemSnapshots(const httplib::Request& req, httplib::Response& res) {
    try {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        
        json request_json;
        try {
            request_json = json::parse(req.body);
        } catch (const json::exception& e) {
            json error;
            error["success"] = false;
            error["error"] = "Invalid JSON format: " + std::string(e.what());
            res.status = 400;
            res.set_content(error.dump(), "application/json");
            return;
        }
        
        std::string snapshot1, snapshot2;
        if (request_json.contains("snapshot1") && request_json["snapshot1"].is_string()) {
            snapshot1 = request_json["snapshot1"];
        }
        if (request_json.contains("snapshot2") && request_json["snapshot2"].is_string()) {
            snapshot2 = request_json["snapshot2"];
        }
        
        if (snapshot1.empty() || snapshot2.empty()) {
            json error;
            error["success"] = false;
            error["error"] = "Both snapshot1 and snapshot2 are required";
            res.status = 400;
            res.set_content(error.dump(), "application/json");
            return;
        }
        
        // Lambda 函数：从内存或磁盘获取快照JSON
        auto getSnapshotJson = [this](const std::string& name) -> std::optional<std::string> {
            {
                std::lock_guard<std::mutex> lk(systemSnapshotsMutex_);
                auto it = systemSnapshotsJson_.find(name);
                if (it != systemSnapshotsJson_.end()) {
                    return it->second;
                }
            }
            
            if (!snapshotStore_) {
                const_cast<HttpServer*>(this)->snapshotStore_ = std::make_unique<snapshot::SnapshotManager>();
            }
            return snapshotStore_->Load(name);
        };
        
        auto json1Opt = getSnapshotJson(snapshot1);
        auto json2Opt = getSnapshotJson(snapshot2);
        
        if (!json1Opt) {
            json error;
            error["success"] = false;
            error["error"] = "Snapshot not found: " + snapshot1;
            res.status = 404;
            res.set_content(error.dump(), "application/json");
            return;
        }
        
        if (!json2Opt) {
            json error;
            error["success"] = false;
            error["error"] = "Snapshot not found: " + snapshot2;
            res.status = 404;
            res.set_content(error.dump(), "application/json");
            return;
        }
        
        json snap1, snap2;
        try {
            snap1 = json::parse(*json1Opt);
            snap2 = json::parse(*json2Opt);
        } catch (const json::exception& e) {
            json error;
            error["success"] = false;
            error["error"] = "Failed to parse snapshot JSON: " + std::string(e.what());
            res.status = 500;
            res.set_content(error.dump(), "application/json");
            return;
        }
        
        // 使用提取的比较函数
        json result = CompareSystemSnapshotsJson(snap1, snap2, snapshot1, snapshot2);
        
        res.set_content(result.dump(), "application/json");
        
    } catch (const std::exception& e) {
        std::cerr << "HandleCompareSystemSnapshots exception: " << e.what() << std::endl;
        json error;
        error["success"] = false;
        error["error"] = "Internal server error";
        res.status = 500;
        res.set_content(error.dump(), "application/json");
    }
}

} // namespace snapshot