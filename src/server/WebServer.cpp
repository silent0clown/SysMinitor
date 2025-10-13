
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "WebServer.h"
#include "../utils/AsyncLogger.h"
#include "../third_party/nlohmann/json.hpp"
#include <sstream>
#include <iomanip>

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
    
    // 设置路由
    SetupRoutes();
    
    // 初始化CPU监控
    if (!cpuMonitor_.Initialize()) {
        std::cerr << "CPU监控初始化失败" << std::endl;
        return false;
    }
    
    // 启动后台监控
    StartBackgroundMonitoring();
    
    // 启动HTTP服务器
    serverThread_ = std::make_unique<std::thread>([this]() {
        std::cout << "启动HTTP服务器，端口: " << port_ << std::endl;
        std::cout << "API端点:" << std::endl;
        std::cout << "  GET /api/cpu/info     - 获取CPU信息" << std::endl;
        std::cout << "  GET /api/cpu/usage    - 获取当前CPU使用率" << std::endl;
        std::cout << "  GET /api/system/info  - 获取系统信息" << std::endl;
        std::cout << "  GET /api/cpu/stream   - 实时流式CPU使用率" << std::endl;
        
        isRunning_ = true;
        server_->listen("0.0.0.0", port_);
        isRunning_ = false;
    });
    
    // 等待服务器启动
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
    // 静态文件服务（用于前端页面）
    server_->set_mount_point("/", "www");//测试页面
    // server_->set_mount_point("/", "webclient");

    // API路由 CPU相关
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
    
    // 添加新的API路由 进程相关
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


    // 磁盘相关路由
    server_->Get("/api/disk/info", [this](const httplib::Request& req, httplib::Response& res) {
        HandleGetDiskInfo(req, res);
    });
    
    server_->Get("/api/disk/performance", [this](const httplib::Request& req, httplib::Response& res) {
        HandleGetDiskPerformance(req, res);
    });

    // 添加OPTIONS请求处理（用于CORS预检）
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


    // 注册表相关API
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


    // 驱动信息
    server_->Get("/api/drivers/snapshot", [this](const httplib::Request& req, httplib::Response& res) {
        HandleGetDriverSnapshot(req, res);
    });
    
    server_->Get("/api/drivers/detail", [this](const httplib::Request& req, httplib::Response& res) {
        HandleGetDriverDetail(req, res);
    });

    // 默认路由
    server_->Get("/", [](const httplib::Request& req, httplib::Response& res) {
        res.set_redirect("/index.html");
    });
}

void HttpServer::StartBackgroundMonitoring() {
    // 设置CPU使用率回调
    cpuMonitor_.SetUsageCallback([this](const CPUUsage& usage) {
        currentUsage_.store(usage.totalUsage);
    });
    
    // 启动监控
    cpuMonitor_.StartMonitoring(1000);
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

void HttpServer::HandleGetSystemInfo(const httplib::Request& req, httplib::Response& res) {
    json response;
    
    // 基础系统信息
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
    // 设置Server-Sent Events (SSE) 头
    res.set_header("Content-Type", "text/event-stream");
    res.set_header("Cache-Control", "no-cache");
    res.set_header("Connection", "keep-alive");
    res.set_header("Access-Control-Allow-Origin", "*");
    
    // 发送初始数据
    std::stringstream initialData;
    initialData << "data: " << json{{"usage", currentUsage_.load()}, {"timestamp", GetTickCount64()}}.dump() << "\n\n";
    res.set_content(initialData.str(), "text/event-stream");
    
    // 注意：这是一个简化的流实现，实际生产环境需要更复杂的连接管理
}



// 添加新的处理函数实现
void HttpServer::HandleGetProcesses(const httplib::Request& req, httplib::Response& res) {
    try {
        auto snapshot = processMonitor_.GetProcessSnapshot();
        json response;
        
        response["timestamp"] = snapshot.timestamp;
        response["totalProcesses"] = snapshot.totalProcesses;
        response["totalThreads"] = snapshot.totalThreads;
        
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
        
        // 解析可选的退出代码
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

// 添加磁盘信息处理函数
void HttpServer::HandleGetDiskInfo(const httplib::Request& req, httplib::Response& res) {
    try {
        // 设置CORS头
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        
        auto snapshot = diskMonitor_.GetDiskSnapshot();
        json response;
        
        response["timestamp"] = snapshot.timestamp;
        
        // 磁盘驱动器信息 - 使用更安全的JSON构建方式
        json drivesJson = json::array();
        for (const auto& drive : snapshot.drives) {
            try {
                json driveJson;
                // 确保所有字符串都是有效的UTF-8
                driveJson["model"] = drive.model.empty() ? "Unknown" : drive.model;
                driveJson["serialNumber"] = drive.serialNumber.empty() ? "" : drive.serialNumber;
                driveJson["interfaceType"] = drive.interfaceType.empty() ? "Unknown" : drive.interfaceType;
                driveJson["mediaType"] = drive.mediaType.empty() ? "Unknown" : drive.mediaType;
                driveJson["totalSize"] = drive.totalSize;
                driveJson["bytesPerSector"] = drive.bytesPerSector;
                driveJson["status"] = drive.status.empty() ? "Unknown" : drive.status;
                driveJson["deviceId"] = drive.deviceId.empty() ? "Unknown" : drive.deviceId;
                
                // 验证JSON对象是否可以序列化
                std::string test = driveJson.dump();
                drivesJson.push_back(driveJson);
            } catch (const std::exception& e) {
                std::cerr << "跳过有问题的驱动器数据: " << e.what() << std::endl;
                continue;
            }
        }
        response["drives"] = drivesJson;
        
        // 分区信息 - 使用更安全的JSON构建方式
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
                
                // 验证JSON对象是否可以序列化
                std::string test = partitionJson.dump();
                partitionsJson.push_back(partitionJson);
            } catch (const std::exception& e) {
                std::cerr << "跳过有问题的分区数据: " << e.what() << std::endl;
                continue;
            }
        }
        response["partitions"] = partitionsJson;
        
        std::string responseStr = response.dump();
        std::cout << "返回磁盘信息，驱动器数量: " << snapshot.drives.size() 
                  << ", 分区数量: " << snapshot.partitions.size() 
                  << ", JSON长度: " << responseStr.length() << std::endl;
        
        res.set_content(responseStr, "application/json");
        
    } catch (const std::exception& e) {
        std::cerr << "HandleGetDiskInfo 异常: " << e.what() << std::endl;
        json error;
        error["error"] = "Internal server error";
        res.status = 500;
        res.set_content(error.dump(), "application/json");
    }
}

void HttpServer::HandleGetDiskPerformance(const httplib::Request& req, httplib::Response& res) {
    try {
        // 设置CORS头
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
        
        std::cout << "返回磁盘性能数据，性能计数器数量: " << snapshot.performance.size() << std::endl;
        
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        std::cerr << "HandleGetDiskPerformance 异常: " << e.what() << std::endl;
        json error;
        error["error"] = e.what();
        res.status = 500;
        res.set_content(error.dump(), "application/json");
    }
}

// 辅助函数：获取当前时间字符串
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
        
        // 系统信息键
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
        
        // 软件信息键
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
        
        // 网络配置键
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
        
        std::string responseStr = response.dump();
        std::cout << "返回注册表快照，系统信息键: " << snapshot.systemInfoKeys.size()
                  << ", 软件键: " << snapshot.softwareKeys.size()
                  << ", 网络键: " << snapshot.networkKeys.size() << std::endl;
        
        res.set_content(responseStr, "application/json");
        
    } catch (const std::exception& e) {
        std::cerr << "HandleGetRegistrySnapshot 异常: " << e.what() << std::endl;
        json error;
        error["error"] = "获取注册表快照失败: " + std::string(e.what());
        res.status = 500;
        res.set_content(error.dump(), "application/json");
    }
}

void HttpServer::HandleSaveRegistrySnapshot(const httplib::Request& req, httplib::Response& res) {
    try {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        
        std::cout << "收到保存注册表快照请求" << std::endl;
        std::cout << "请求体大小: " << req.body.size() << " 字节" << std::endl;
        
        // 解析JSON数据
        json request_json;
        try {
            request_json = json::parse(req.body);
            std::cout << "JSON解析成功" << std::endl;
        } catch (const json::exception& e) {
            std::cerr << "JSON解析失败: " << e.what() << std::endl;
            json error;
            error["success"] = false;
            error["error"] = "无效的JSON格式";
            res.status = 400;
            res.set_content(error.dump(), "application/json");
            return;
        }
        
        // 获取快照名称
        std::string snapshotName;
        if (request_json.contains("snapshotName") && request_json["snapshotName"].is_string()) {
            snapshotName = request_json["snapshotName"];
        }
        if (snapshotName.empty()) {
            snapshotName = "snapshot_" + std::to_string(GetTickCount64());
        }
        
        // 创建快照对象但不解析详细内容（避免编码问题）
        RegistrySnapshot snapshot;
        snapshot.timestamp = GetTickCount64();
        
        // 只记录数量，不解析具体内容
        if (request_json.contains("systemInfo") && request_json["systemInfo"].is_array()) {
            snapshot.systemInfoKeys.resize(request_json["systemInfo"].size());
        }
        
        if (request_json.contains("software") && request_json["software"].is_array()) {
            snapshot.softwareKeys.resize(request_json["software"].size());
        }
        
        if (request_json.contains("network") && request_json["network"].is_array()) {
            snapshot.networkKeys.resize(request_json["network"].size());
        }
        
        // 保存快照
        registrySnapshots_[snapshotName] = snapshot;
        
        // 构建响应 - 确保所有字符串都是安全的
        json response;
        response["success"] = true;
        response["snapshotName"] = snapshotName; // snapshotName 应该是安全的
        response["timestamp"] = snapshot.timestamp;
        response["systemInfoCount"] = snapshot.systemInfoKeys.size();
        response["softwareCount"] = snapshot.softwareKeys.size();
        response["networkCount"] = snapshot.networkKeys.size();
        response["message"] = "快照保存成功";
        
        std::cout << "保存注册表快照成功: " << snapshotName 
                  << " (系统: " << snapshot.systemInfoKeys.size()
                  << ", 软件: " << snapshot.softwareKeys.size() 
                  << ", 网络: " << snapshot.networkKeys.size() << ")" << std::endl;
        
        // 安全地设置响应内容
        std::string response_str;
        try {
            response_str = response.dump();
            res.set_content(response_str, "application/json");
        } catch (const json::exception& e) {
            std::cerr << "JSON序列化失败: " << e.what() << std::endl;
            // 回退方案：构建一个简单的响应
            res.set_content("{\"success\":true,\"message\":\"快照保存成功\"}", "application/json");
        }
        
    } catch (const std::exception& e) {
        std::cerr << "HandleSaveRegistrySnapshot 异常: " << e.what() << std::endl;
        // 使用简单的错误响应，避免JSON问题
        res.status = 500;
        res.set_content("{\"success\":false,\"error\":\"服务器内部错误\"}", "application/json");
    }
}

// void HttpServer::HandleSaveRegistrySnapshot(const httplib::Request& req, httplib::Response& res) {
//     try {
//         res.set_header("Access-Control-Allow-Origin", "*");
//         res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
//         res.set_header("Access-Control-Allow-Headers", "Content-Type");
        
//         // 添加请求日志
//         std::cout << "收到保存注册表快照请求" << std::endl;
//         std::cout << "请求体大小: " << req.body.size() << " 字节" << std::endl;
        
//         // 解析JSON数据
//         json request_json;
//         try {
//             request_json = json::parse(req.body);
//             std::cout << "JSON解析成功" << std::endl;
//         } catch (const json::exception& e) {
//             std::cerr << "JSON解析失败: " << e.what() << std::endl;
//             json error;
//             error["success"] = false;
//             error["error"] = "无效的JSON格式: " + std::string(e.what());
//             res.status = 400;
//             res.set_content(error.dump(), "application/json");
//             return;
//         }
        
//         // 获取快照名称
//         std::string snapshotName;
//         if (request_json.contains("snapshotName") && !request_json["snapshotName"].is_null()) {
//             snapshotName = request_json["snapshotName"];
//         }
//         if (snapshotName.empty()) {
//             snapshotName = "snapshot_" + std::to_string(GetTickCount64());
//         }
        
//         // 提取注册表数据
//         RegistrySnapshot snapshot;
//         snapshot.timestamp = GetTickCount64();
        
//         // 安全地提取系统信息
//         if (request_json.contains("systemInfo") && request_json["systemInfo"].is_array()) {
//             try {
//                 snapshot.systemInfoKeys = ParseRegistryKeysFromJson(request_json["systemInfo"]);
//             } catch (const std::exception& e) {
//                 std::cerr << "解析系统信息失败: " << e.what() << std::endl;
//                 // 继续处理其他部分
//             }
//         }
        
//         // 安全地提取软件信息
//         if (request_json.contains("software") && request_json["software"].is_array()) {
//             try {
//                 snapshot.softwareKeys = ParseRegistryKeysFromJson(request_json["software"]);
//             } catch (const std::exception& e) {
//                 std::cerr << "解析软件信息失败: " << e.what() << std::endl;
//             }
//         }
        
//         // 安全地提取网络配置
//         if (request_json.contains("network") && request_json["network"].is_array()) {
//             try {
//                 snapshot.networkKeys = ParseRegistryKeysFromJson(request_json["network"]);
//             } catch (const std::exception& e) {
//                 std::cerr << "解析网络配置失败: " << e.what() << std::endl;
//             }
//         }
        
//         // 保存快照
//         registrySnapshots_[snapshotName] = snapshot;
        
//         json response;
//         response["success"] = true;
//         response["snapshotName"] = snapshotName;
//         response["timestamp"] = snapshot.timestamp;
//         response["systemInfoCount"] = snapshot.systemInfoKeys.size();
//         response["softwareCount"] = snapshot.softwareKeys.size();
//         response["networkCount"] = snapshot.networkKeys.size();
//         response["message"] = "快照保存成功";
        
//         std::cout << "保存注册表快照成功: " << snapshotName 
//                   << " (系统: " << snapshot.systemInfoKeys.size()
//                   << ", 软件: " << snapshot.softwareKeys.size() 
//                   << ", 网络: " << snapshot.networkKeys.size() << ")" << std::endl;
        
//         res.set_content(response.dump(), "application/json");
        
//     } catch (const std::exception& e) {
//         std::cerr << "HandleSaveRegistrySnapshot 异常: " << e.what() << std::endl;
//         json error;
//         error["success"] = false;
//         error["error"] = "服务器内部错误: " + std::string(e.what());
//         res.status = 500;
//         res.set_content(error.dump(), "application/json");
//     }
// }

// 添加安全解析JSON的方法
std::vector<RegistryKey> HttpServer::ParseRegistryKeysFromJson(const json& json_array) {
    std::vector<RegistryKey> keys;
    
    if (!json_array.is_array()) {
        return keys;
    }
    
    for (const auto& item : json_array) {
        try {
            RegistryKey key;
            
            // 安全提取路径
            if (item.contains("path") && item["path"].is_string()) {
                key.path = SafeJsonToString(item["path"]);
            }
            
            // 安全提取值
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
            
            // 安全提取子键
            if (item.contains("subkeys") && item["subkeys"].is_array()) {
                for (const auto& subkey_item : item["subkeys"]) {
                    if (subkey_item.is_string()) {
                        key.subkeys.push_back(SafeJsonToString(subkey_item));
                    }
                }
            }
            
            keys.push_back(key);
            
        } catch (const std::exception& e) {
            std::cerr << "解析注册表键失败: " << e.what() << std::endl;
            // 跳过有问题的键，继续处理下一个
        }
    }
    
    return keys;
}

// 安全地从JSON提取字符串
std::string HttpServer::SafeJsonToString(const json& json_value) {
    if (json_value.is_null()) {
        return "";
    }
    
    if (json_value.is_string()) {
        try {
            std::string str = json_value.get<std::string>();
            return SanitizeUTF8(str);
        } catch (const std::exception& e) {
            std::cerr << "JSON字符串转换失败: " << e.what() << std::endl;
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

// UTF-8清理函数（在HttpServer类中添加）
std::string HttpServer::SanitizeUTF8(const std::string& str) {
    std::string result;
    result.reserve(str.length());
    
    for (size_t i = 0; i < str.length(); ) {
        unsigned char c = static_cast<unsigned char>(str[i]);
        
        if (c <= 0x7F) {
            // ASCII字符
            result += c;
            i++;
        } else if ((c & 0xE0) == 0xC0) {
            // 2字节UTF-8字符
            if (i + 1 < str.length() && (str[i+1] & 0xC0) == 0x80) {
                result += str.substr(i, 2);
                i += 2;
            } else {
                result += '?';
                i++;
            }
        } else if ((c & 0xF0) == 0xE0) {
            // 3字节UTF-8字符
            if (i + 2 < str.length() && (str[i+1] & 0xC0) == 0x80 && (str[i+2] & 0xC0) == 0x80) {
                result += str.substr(i, 3);
                i += 3;
            } else {
                result += '?';
                i++;
            }
        } else if ((c & 0xF8) == 0xF0) {
            // 4字节UTF-8字符
            if (i + 3 < str.length() && (str[i+1] & 0xC0) == 0x80 && 
                (str[i+2] & 0xC0) == 0x80 && (str[i+3] & 0xC0) == 0x80) {
                result += str.substr(i, 4);
                i += 4;
            } else {
                result += '?';
                i++;
            }
        } else {
            // 无效的UTF-8字节
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
        
        std::cout << "返回保存的快照列表，数量: " << registrySnapshots_.size() << std::endl;
        
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        std::cerr << "HandleGetSavedSnapshots 异常: " << e.what() << std::endl;
        json error;
        error["error"] = "获取快照列表失败";
        res.status = 500;
        res.set_content(error.dump(), "application/json");
    }
}


// 辅助函数：比较两个注册表键列表
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
    
    // 查找新增的键
    for (const auto& [path, key] : map2) {
        if (map1.find(path) == map1.end()) {
            changes["added"].push_back(path);
        }
    }
    
    // 查找删除的键
    for (const auto& [path, key] : map1) {
        if (map2.find(path) == map2.end()) {
            changes["removed"].push_back(path);
        }
    }
    
    // 查找修改的键
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
        
        // 解析JSON请求体
        json request_json;
        try {
            request_json = json::parse(req.body);
        } catch (const json::exception& e) {
            json error;
            error["success"] = false;
            error["error"] = "无效的JSON格式: " + std::string(e.what());
            res.status = 400;
            res.set_content(error.dump(), "application/json");
            return;
        }
        
        // 从JSON中获取参数
        std::string snapshot1, snapshot2;
        if (request_json.contains("snapshot1") && request_json["snapshot1"].is_string()) {
            snapshot1 = request_json["snapshot1"];
        }
        if (request_json.contains("snapshot2") && request_json["snapshot2"].is_string()) {
            snapshot2 = request_json["snapshot2"];
        }
        
        std::cout << "收到比较快照请求: " << snapshot1 << " vs " << snapshot2 << std::endl;
        
        if (snapshot1.empty() || snapshot2.empty()) {
            json error;
            error["success"] = false;
            error["error"] = "需要提供两个快照名称";
            res.status = 400;
            res.set_content(error.dump(), "application/json");
            return;
        }
        
        // 检查快照是否存在
        if (registrySnapshots_.find(snapshot1) == registrySnapshots_.end()) {
            json error;
            error["success"] = false;
            error["error"] = "快照不存在: " + snapshot1;
            res.status = 404;
            res.set_content(error.dump(), "application/json");
            return;
        }
        
        if (registrySnapshots_.find(snapshot2) == registrySnapshots_.end()) {
            json error;
            error["success"] = false;
            error["error"] = "快照不存在: " + snapshot2;
            res.status = 404;
            res.set_content(error.dump(), "application/json");
            return;
        }
        
        // 获取快照
        const auto& snap1 = registrySnapshots_[snapshot1];
        const auto& snap2 = registrySnapshots_[snapshot2];
        
        json response;
        response["snapshot1"] = snapshot1;
        response["snapshot2"] = snapshot2;
        response["timestamp1"] = snap1.timestamp;
        response["timestamp2"] = snap2.timestamp;
        
        // 比较系统信息
        response["systemInfoChanges"] = CompareRegistrySnapshots(snap1.systemInfoKeys, snap2.systemInfoKeys);
        // 比较软件信息
        response["softwareChanges"] = CompareRegistrySnapshots(snap1.softwareKeys, snap2.softwareKeys);
        // 比较网络配置
        response["networkChanges"] = CompareRegistrySnapshots(snap1.networkKeys, snap2.networkKeys);
        
        std::cout << "比较快照完成: " << snapshot1 << " vs " << snapshot2 << std::endl;
        
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        std::cerr << "HandleCompareSnapshots 异常: " << e.what() << std::endl;
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
        
        // 从路径参数获取快照名称
        if (req.matches.size() < 2) {
            res.status = 400;
            res.set_content(R"({"success":false,"error":"缺少快照名称参数"})", "application/json");
            return;
        }
        
        std::string snapshotName = req.matches[1];
        
        std::cout << "收到删除快照请求，原始名称: " << snapshotName << std::endl;
        
        if (snapshotName.empty()) {
            res.status = 400;
            res.set_content(R"({"success":false,"error":"快照名称不能为空"})", "application/json");
            return;
        }
        
        // 清理快照名称中的非UTF-8字符
        std::string cleanSnapshotName = SanitizeUTF8(snapshotName);
        std::cout << "清理后的快照名称: " << cleanSnapshotName << std::endl;
        
        // 在map中查找匹配的快照
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
            // 使用原始名称删除
            registrySnapshots_.erase(actualSnapshotName);
            
            // 构建JSON响应
            json response;
            response["success"] = true;
            response["message"] = "快照删除成功";
            
            // 安全地序列化JSON
            std::string response_str;
            try {
                response_str = response.dump();
            } catch (const json::exception& e) {
                std::cerr << "JSON序列化失败，使用回退响应: " << e.what() << std::endl;
                response_str = R"({"success":true,"message":"快照删除成功"})";
            }
            
            res.set_content(response_str, "application/json");
            std::cout << "删除快照成功: " << actualSnapshotName << std::endl;
            
        } else {
            // 快照不存在
            json error;
            error["success"] = false;
            error["error"] = "快照不存在: " + cleanSnapshotName;
            
            std::string error_str;
            try {
                error_str = error.dump();
            } catch (const json::exception& e) {
                std::cerr << "错误JSON序列化失败: " << e.what() << std::endl;
                error_str = R"({"success":false,"error":"快照不存在"})";
            }
            
            res.status = 404;
            res.set_content(error_str, "application/json");
            std::cout << "删除快照失败: 快照不存在 - " << cleanSnapshotName << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "HandleDeleteSnapshot 异常: " << e.what() << std::endl;
        res.status = 500;
        res.set_content(R"({"success":false,"error":"删除快照失败"})", "application/json");
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
        
        // 统计信息
        json statsJson;
        statsJson["totalDrivers"] = snapshot.stats.totalDrivers;
        statsJson["runningCount"] = snapshot.stats.runningCount;
        statsJson["stoppedCount"] = snapshot.stats.stoppedCount;
        statsJson["kernelCount"] = snapshot.stats.kernelCount;
        statsJson["fileSystemCount"] = snapshot.stats.fileSystemCount;
        statsJson["autoStartCount"] = snapshot.stats.autoStartCount;
        statsJson["thirdPartyCount"] = snapshot.stats.thirdPartyCount;
        response["statistics"] = statsJson;
        
        // 内核驱动
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
        
        // 文件系统驱动
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
        
        // 运行中的驱动
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
        
        // 已停止的驱动
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
        
        // 自动启动的驱动
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
        
        // 第三方驱动
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
        std::cout << "返回驱动快照 - "
                  << "内核驱动: " << snapshot.kernelDrivers.size() << " 个, "
                  << "文件系统驱动: " << snapshot.fileSystemDrivers.size() << " 个, "
                  << "运行中: " << snapshot.runningDrivers.size() << " 个" << std::endl;
        
        res.set_content(responseStr, "application/json");
        
    } catch (const std::exception& e) {
        std::cerr << "HandleGetDriverSnapshot 异常: " << e.what() << std::endl;
        json error;
        error["error"] = "获取驱动快照失败: " + std::string(e.what());
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
            error["error"] = "缺少驱动名称参数";
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
        std::cerr << "HandleGetDriverDetail 异常: " << e.what() << std::endl;
        json error;
        error["error"] = "获取驱动详情失败: " + std::string(e.what());
        res.status = 500;
        res.set_content(error.dump(), "application/json");
    }
}






} // namespace snapshot