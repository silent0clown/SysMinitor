#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "WebServer.h"
#include "../utils/AsyncLogger.h"
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
    server_->set_mount_point("/", "www");//Test page
    // server_->set_mount_point("/", "webclient");

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
    
    server_->Post("/api/process/(\\d+)/terminate", [this](const httplib::Request& req, httplib::Response& res) {
        HandleTerminateProcess(req, res);
    });


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
        HandleGetRegistrySnapshot(req, res);
    });
    
    server_->Post("/api/registry/snapshot/save", [this](const httplib::Request& req, httplib::Response& res) {
        HandleSaveRegistrySnapshot(req, res);
    });
    
    server_->Get("/api/registry/snapshots", [this](const httplib::Request& req, httplib::Response& res) {
        HandleGetSavedSnapshots(req, res);
    });
    
    server_->Post("/api/registry/snapshots/compare", [this](const httplib::Request& req, httplib::Response& res) {
        HandleCompareSnapshots(req, res);
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

        Sample s{GetTickCount64(), usage.totalUsage};
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

        Sample s{usage.timestamp ? usage.timestamp : GetTickCount64(), usage.usedPercent};
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
    response["usage"] = usage;
    response["timestamp"] = GetTickCount64();
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
    response["usedPercent"] = snapshot.usedPercent;
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
    response["timestamp"] = GetTickCount64();
    
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
    initialData << "data: " << json{{"usage", currentUsage_.load()}, {"timestamp", GetTickCount64()}}.dump() << "\n\n";
    res.set_content(initialData.str(), "text/event-stream");
    
    // Note: This is a simplified streaming implementation, production environment requires more complex connection management
}



// Add new handler function implementations
void HttpServer::HandleGetProcesses(const httplib::Request& req, httplib::Response& res) {
    try {
        auto snapshot = processMonitor_.GetProcessSnapshot();
        json response;
        
        response["timestamp"] = snapshot.timestamp;
        response["totalProcesses"] = snapshot.totalProcesses;
        response["totalThreads"] = snapshot.totalThreads;
        response["totalHandles"] = snapshot.totalHandles;        // 新�??
        response["totalGdiObjects"] = snapshot.totalGdiObjects;  // 新�??
        response["totalUserObjects"] = snapshot.totalUserObjects; // 新�??
        
        json processesJson = json::array();
        for (const auto& process : snapshot.processes) {
            json processJson;
            processJson["pid"] = process.pid;
            processJson["parentPid"] = process.parentPid;
            processJson["name"] = process.name;
            processJson["fullPath"] = process.fullPath;
            processJson["state"] = process.state;
            processJson["username"] = process.username;
            processJson["cpuUsage"] = process.cpuUsage;
            processJson["memoryUsage"] = process.memoryUsage;
            processJson["workingSetSize"] = process.workingSetSize;
            processJson["pagefileUsage"] = process.pagefileUsage;
            processJson["createTime"] = process.createTime;
            processJson["priority"] = process.priority;
            processJson["threadCount"] = process.threadCount;
            processJson["commandLine"] = process.commandLine;
            processJson["handleCount"] = process.handleCount;    // 新�??
            processJson["gdiCount"] = process.gdiCount;          // 新�??
            processJson["userCount"] = process.userCount;        // 新�??
            
            processesJson.push_back(processJson);
        }
        
        response["processes"] = processesJson;
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        json error;
        error["error"] = e.what();
        res.status = 500;
        res.set_content(error.dump(), "application/json");
    }
}

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

void HttpServer::HandleGetRegistrySnapshot(const httplib::Request& req, httplib::Response& res) {
    try {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        
        auto snapshot = registryMonitor_.GetRegistrySnapshot();
        json response;
        
        response["timestamp"] = snapshot.timestamp;
        
        // System information keys
        json systemInfoJson = json::array();
        for (const auto& key : snapshot.systemInfoKeys) {
            json keyJson;
            keyJson["path"] = key.path;
            
            json valuesJson = json::array();
            for (const auto& value : key.values) {
                json valueJson;
                valueJson["name"] = value.name;
                valueJson["type"] = value.type;
                valueJson["data"] = value.data;
                valueJson["size"] = value.size;
                valuesJson.push_back(valueJson);
            }
            keyJson["values"] = valuesJson;
            keyJson["subkeys"] = key.subkeys;
            
            systemInfoJson.push_back(keyJson);
        }
        response["systemInfo"] = systemInfoJson;
        
        // Software information keys
        json softwareJson = json::array();
        for (const auto& key : snapshot.softwareKeys) {
            json keyJson;
            keyJson["path"] = key.path;
            
            json valuesJson = json::array();
            for (const auto& value : key.values) {
                json valueJson;
                valueJson["name"] = value.name;
                valueJson["type"] = value.type;
                valueJson["data"] = value.data;
                valueJson["size"] = value.size;
                valuesJson.push_back(valueJson);
            }
            keyJson["values"] = valuesJson;
            keyJson["subkeys"] = key.subkeys;
            
            softwareJson.push_back(keyJson);
        }
        response["software"] = softwareJson;
        
        // Network configuration keys
        json networkJson = json::array();
        for (const auto& key : snapshot.networkKeys) {
            json keyJson;
            keyJson["path"] = key.path;
            
            json valuesJson = json::array();
            for (const auto& value : key.values) {
                json valueJson;
                valueJson["name"] = value.name;
                valueJson["type"] = value.type;
                valueJson["data"] = value.data;
                valueJson["size"] = value.size;
                valuesJson.push_back(valueJson);
            }
            keyJson["values"] = valuesJson;
            keyJson["subkeys"] = key.subkeys;
            
            networkJson.push_back(keyJson);
        }
        response["network"] = networkJson;
        
        // New: Auto-start items
        json autoStartJson = json::array();
        for (const auto& key : snapshot.autoStartKeys) {
            json keyJson;
            keyJson["path"] = key.path;
            
            json valuesJson = json::array();
            for (const auto& value : key.values) {
                json valueJson;
                valueJson["name"] = value.name;
                valueJson["type"] = value.type;
                valueJson["data"] = value.data;
                valueJson["size"] = value.size;
                valuesJson.push_back(valueJson);
            }
            keyJson["values"] = valuesJson;
            keyJson["subkeys"] = key.subkeys;
            
            // Add auto-start specific fields
            if (!key.autoStartType.empty()) {
                keyJson["autoStartType"] = key.autoStartType;
            }
            if (!key.category.empty()) {
                keyJson["category"] = key.category;
            }
            
            autoStartJson.push_back(keyJson);
        }
        response["autoStart"] = autoStartJson;
        
        std::string responseStr = response.dump();
        std::cout << "Returning registry snapshot, system info keys: " << snapshot.systemInfoKeys.size()
                  << ", software keys: " << snapshot.softwareKeys.size()
                  << ", network keys: " << snapshot.networkKeys.size()
                  << ", auto-start items: " << snapshot.autoStartKeys.size() << std::endl;
        
        res.set_content(responseStr, "application/json");
        
    } catch (const std::exception& e) {
        std::cerr << "HandleGetRegistrySnapshot exception: " << e.what() << std::endl;
        json error;
        error["error"] = "Failed to get registry snapshot: " + std::string(e.what());
        res.status = 500;
        res.set_content(error.dump(), "application/json");
    }
}

void HttpServer::HandleSaveRegistrySnapshot(const httplib::Request& req, httplib::Response& res) {
    try {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        
        std::cout << "Received save registry snapshot request" << std::endl;
        std::cout << "Request body size: " << req.body.size() << " bytes" << std::endl;
        
        // Parse JSON data
        json request_json;
        try {
            request_json = json::parse(req.body);
            std::cout << "JSON parsing successful" << std::endl;
        } catch (const json::exception& e) {
            std::cerr << "JSON parsing failed: " << e.what() << std::endl;
            json error;
            error["success"] = false;
            error["error"] = "Invalid JSON format";
            res.status = 400;
            res.set_content(error.dump(), "application/json");
            return;
        }
        
        // Get snapshot name
        std::string snapshotName;
        if (request_json.contains("snapshotName") && request_json["snapshotName"].is_string()) {
            snapshotName = request_json["snapshotName"];
        }
        if (snapshotName.empty()) {
            snapshotName = "snapshot_" + std::to_string(GetTickCount64());
        }
        
        // Create snapshot object but don't parse detailed content (avoid encoding issues)
        RegistrySnapshot snapshot;
        snapshot.timestamp = GetTickCount64();
        
        // Only record count, don't parse specific content
        if (request_json.contains("systemInfo") && request_json["systemInfo"].is_array()) {
            snapshot.systemInfoKeys.resize(request_json["systemInfo"].size());
        }
        
        if (request_json.contains("software") && request_json["software"].is_array()) {
            snapshot.softwareKeys.resize(request_json["software"].size());
        }
        
        if (request_json.contains("network") && request_json["network"].is_array()) {
            snapshot.networkKeys.resize(request_json["network"].size());
        }
        
        // Save snapshot
        registrySnapshots_[snapshotName] = snapshot;
        
        // Build response - ensure all strings are safe
        json response;
        response["success"] = true;
        response["snapshotName"] = snapshotName; // snapshotName should be safe
        response["timestamp"] = snapshot.timestamp;
        response["systemInfoCount"] = snapshot.systemInfoKeys.size();
        response["softwareCount"] = snapshot.softwareKeys.size();
        response["networkCount"] = snapshot.networkKeys.size();
        response["message"] = "Snapshot saved successfully";
        
        std::cout << "Registry snapshot saved successfully: " << snapshotName 
                  << " (System: " << snapshot.systemInfoKeys.size()
                  << ", Software: " << snapshot.softwareKeys.size() 
                  << ", Network: " << snapshot.networkKeys.size() << ")" << std::endl;
        
        // Safely set response content
        std::string response_str;
        try {
            response_str = response.dump();
            res.set_content(response_str, "application/json");
        } catch (const json::exception& e) {
            std::cerr << "JSON serialization failed: " << e.what() << std::endl;
            // Fallback: build a simple response
            res.set_content("{\"success\":true,\"message\":\"Snapshot saved successfully\"}", "application/json");
        }
        
    } catch (const std::exception& e) {
        std::cerr << "HandleSaveRegistrySnapshot exception: " << e.what() << std::endl;
        // Use simple error response to avoid JSON issues
        res.status = 500;
        res.set_content("{\"success\":false,\"error\":\"Internal server error\"}", "application/json");
    }
}

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

void HttpServer::HandleGetSavedSnapshots(const httplib::Request& req, httplib::Response& res) {
    try {
        res.set_header("Access-Control-Allow-Origin", "*");
        
        json response = json::array();
        
        for (const auto& [name, snapshot] : registrySnapshots_) {
            json snapshotJson;
            snapshotJson["name"] = name;
            snapshotJson["timestamp"] = snapshot.timestamp;
            snapshotJson["systemInfoCount"] = snapshot.systemInfoKeys.size();
            snapshotJson["softwareCount"] = snapshot.softwareKeys.size();
            snapshotJson["networkCount"] = snapshot.networkKeys.size();
            
            response.push_back(snapshotJson);
        }
        
        std::cout << "Returning saved snapshot list, count: " << registrySnapshots_.size() << std::endl;
        
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        std::cerr << "HandleGetSavedSnapshots exception: " << e.what() << std::endl;
        json error;
        error["error"] = "Failed to get snapshot list";
        res.status = 500;
        res.set_content(error.dump(), "application/json");
    }
}


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

void HttpServer::HandleCompareSnapshots(const httplib::Request& req, httplib::Response& res) {
    try {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.set_header("Content-Type", "application/json; charset=utf-8");
        
        // Parse JSON request body
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
        
        // Get parameters from JSON
        std::string snapshot1, snapshot2;
        if (request_json.contains("snapshot1") && request_json["snapshot1"].is_string()) {
            snapshot1 = request_json["snapshot1"];
        }
        if (request_json.contains("snapshot2") && request_json["snapshot2"].is_string()) {
            snapshot2 = request_json["snapshot2"];
        }
        
        std::cout << "Received compare snapshots request: " << snapshot1 << " vs " << snapshot2 << std::endl;
        
        if (snapshot1.empty() || snapshot2.empty()) {
            json error;
            error["success"] = false;
            error["error"] = "Two snapshot names are required";
            res.status = 400;
            res.set_content(error.dump(), "application/json");
            return;
        }
        
        // Check if snapshots exist
        if (registrySnapshots_.find(snapshot1) == registrySnapshots_.end()) {
            json error;
            error["success"] = false;
            error["error"] = "Snapshot does not exist: " + snapshot1;
            res.status = 404;
            res.set_content(error.dump(), "application/json");
            return;
        }
        
        if (registrySnapshots_.find(snapshot2) == registrySnapshots_.end()) {
            json error;
            error["success"] = false;
            error["error"] = "Snapshot does not exist: " + snapshot2;
            res.status = 404;
            res.set_content(error.dump(), "application/json");
            return;
        }
        
        // Get snapshots
        const auto& snap1 = registrySnapshots_[snapshot1];
        const auto& snap2 = registrySnapshots_[snapshot2];
        
        json response;
        response["snapshot1"] = snapshot1;
        response["snapshot2"] = snapshot2;
        response["timestamp1"] = snap1.timestamp;
        response["timestamp2"] = snap2.timestamp;
        
        // Compare system information
        response["systemInfoChanges"] = CompareRegistrySnapshots(snap1.systemInfoKeys, snap2.systemInfoKeys);
        // Compare software information
        response["softwareChanges"] = CompareRegistrySnapshots(snap1.softwareKeys, snap2.softwareKeys);
        // Compare network configuration
        response["networkChanges"] = CompareRegistrySnapshots(snap1.networkKeys, snap2.networkKeys);
        
        std::cout << "Snapshot comparison completed: " << snapshot1 << " vs " << snapshot2 << std::endl;
        
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        std::cerr << "HandleCompareSnapshots exception: " << e.what() << std::endl;
        json error;
        error["error"] = e.what();
        res.status = 500;
        res.set_content(error.dump(), "application/json");
    }
}

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
        res.status = 500;
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

        std::vector<std::string> names;
        {
            std::lock_guard<std::mutex> lk(systemSnapshotsMutex_);
            names.reserve(systemSnapshotsJson_.size());
            for (const auto &kv : systemSnapshotsJson_) {
                names.push_back(kv.first);
            }
        }

        std::sort(names.begin(), names.end(), std::greater<std::string>());

        json arr = json::array();
        for (const auto &n : names) arr.push_back(n);

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
} // namespace snapshot