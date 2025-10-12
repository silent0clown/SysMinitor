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
    // server_->set_mount_point("/", "../www");//测试页面
    server_->set_mount_point("/", "webclient");

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








} // namespace snapshot