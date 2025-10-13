
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
    
    // ����·��
    SetupRoutes();
    
    // ��ʼ��CPU���
    if (!cpuMonitor_.Initialize()) {
        std::cerr << "CPU��س�ʼ��ʧ��" << std::endl;
        return false;
    }
    
    // ������̨���
    StartBackgroundMonitoring();
    
    // ����HTTP������
    serverThread_ = std::make_unique<std::thread>([this]() {
        std::cout << "����HTTP���������˿�: " << port_ << std::endl;
        std::cout << "API�˵�:" << std::endl;
        std::cout << "  GET /api/cpu/info     - ��ȡCPU��Ϣ" << std::endl;
        std::cout << "  GET /api/cpu/usage    - ��ȡ��ǰCPUʹ����" << std::endl;
        std::cout << "  GET /api/system/info  - ��ȡϵͳ��Ϣ" << std::endl;
        std::cout << "  GET /api/cpu/stream   - ʵʱ��ʽCPUʹ����" << std::endl;
        
        isRunning_ = true;
        server_->listen("0.0.0.0", port_);
        isRunning_ = false;
    });
    
    // �ȴ�����������
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
    // ��̬�ļ���������ǰ��ҳ�棩
    server_->set_mount_point("/", "www");//����ҳ��
    // server_->set_mount_point("/", "webclient");

    // API·�� CPU���
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
    
    // ����µ�API·�� �������
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


    // �������·��
    server_->Get("/api/disk/info", [this](const httplib::Request& req, httplib::Response& res) {
        HandleGetDiskInfo(req, res);
    });
    
    server_->Get("/api/disk/performance", [this](const httplib::Request& req, httplib::Response& res) {
        HandleGetDiskPerformance(req, res);
    });

    // ���OPTIONS����������CORSԤ�죩
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


    // ע������API
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


    // ������Ϣ
    server_->Get("/api/drivers/snapshot", [this](const httplib::Request& req, httplib::Response& res) {
        HandleGetDriverSnapshot(req, res);
    });
    
    server_->Get("/api/drivers/detail", [this](const httplib::Request& req, httplib::Response& res) {
        HandleGetDriverDetail(req, res);
    });

    // Ĭ��·��
    server_->Get("/", [](const httplib::Request& req, httplib::Response& res) {
        res.set_redirect("/index.html");
    });
}

void HttpServer::StartBackgroundMonitoring() {
    // ����CPUʹ���ʻص�
    cpuMonitor_.SetUsageCallback([this](const CPUUsage& usage) {
        currentUsage_.store(usage.totalUsage);
    });
    
    // �������
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
    
    // ����ϵͳ��Ϣ
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
    // ����Server-Sent Events (SSE) ͷ
    res.set_header("Content-Type", "text/event-stream");
    res.set_header("Cache-Control", "no-cache");
    res.set_header("Connection", "keep-alive");
    res.set_header("Access-Control-Allow-Origin", "*");
    
    // ���ͳ�ʼ����
    std::stringstream initialData;
    initialData << "data: " << json{{"usage", currentUsage_.load()}, {"timestamp", GetTickCount64()}}.dump() << "\n\n";
    res.set_content(initialData.str(), "text/event-stream");
    
    // ע�⣺����һ���򻯵���ʵ�֣�ʵ������������Ҫ�����ӵ����ӹ���
}



// ����µĴ�����ʵ��
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
        
        // ������ѡ���˳�����
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

// ��Ӵ�����Ϣ������
void HttpServer::HandleGetDiskInfo(const httplib::Request& req, httplib::Response& res) {
    try {
        // ����CORSͷ
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        
        auto snapshot = diskMonitor_.GetDiskSnapshot();
        json response;
        
        response["timestamp"] = snapshot.timestamp;
        
        // ������������Ϣ - ʹ�ø���ȫ��JSON������ʽ
        json drivesJson = json::array();
        for (const auto& drive : snapshot.drives) {
            try {
                json driveJson;
                // ȷ�������ַ���������Ч��UTF-8
                driveJson["model"] = drive.model.empty() ? "Unknown" : drive.model;
                driveJson["serialNumber"] = drive.serialNumber.empty() ? "" : drive.serialNumber;
                driveJson["interfaceType"] = drive.interfaceType.empty() ? "Unknown" : drive.interfaceType;
                driveJson["mediaType"] = drive.mediaType.empty() ? "Unknown" : drive.mediaType;
                driveJson["totalSize"] = drive.totalSize;
                driveJson["bytesPerSector"] = drive.bytesPerSector;
                driveJson["status"] = drive.status.empty() ? "Unknown" : drive.status;
                driveJson["deviceId"] = drive.deviceId.empty() ? "Unknown" : drive.deviceId;
                
                // ��֤JSON�����Ƿ�������л�
                std::string test = driveJson.dump();
                drivesJson.push_back(driveJson);
            } catch (const std::exception& e) {
                std::cerr << "���������������������: " << e.what() << std::endl;
                continue;
            }
        }
        response["drives"] = drivesJson;
        
        // ������Ϣ - ʹ�ø���ȫ��JSON������ʽ
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
                
                // ��֤JSON�����Ƿ�������л�
                std::string test = partitionJson.dump();
                partitionsJson.push_back(partitionJson);
            } catch (const std::exception& e) {
                std::cerr << "����������ķ�������: " << e.what() << std::endl;
                continue;
            }
        }
        response["partitions"] = partitionsJson;
        
        std::string responseStr = response.dump();
        std::cout << "���ش�����Ϣ������������: " << snapshot.drives.size() 
                  << ", ��������: " << snapshot.partitions.size() 
                  << ", JSON����: " << responseStr.length() << std::endl;
        
        res.set_content(responseStr, "application/json");
        
    } catch (const std::exception& e) {
        std::cerr << "HandleGetDiskInfo �쳣: " << e.what() << std::endl;
        json error;
        error["error"] = "Internal server error";
        res.status = 500;
        res.set_content(error.dump(), "application/json");
    }
}

void HttpServer::HandleGetDiskPerformance(const httplib::Request& req, httplib::Response& res) {
    try {
        // ����CORSͷ
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
        
        std::cout << "���ش����������ݣ����ܼ���������: " << snapshot.performance.size() << std::endl;
        
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        std::cerr << "HandleGetDiskPerformance �쳣: " << e.what() << std::endl;
        json error;
        error["error"] = e.what();
        res.status = 500;
        res.set_content(error.dump(), "application/json");
    }
}

// ������������ȡ��ǰʱ���ַ���
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
        
        // ϵͳ��Ϣ��
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
        
        // �����Ϣ��
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
        
        // �������ü�
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
        std::cout << "����ע�����գ�ϵͳ��Ϣ��: " << snapshot.systemInfoKeys.size()
                  << ", �����: " << snapshot.softwareKeys.size()
                  << ", �����: " << snapshot.networkKeys.size() << std::endl;
        
        res.set_content(responseStr, "application/json");
        
    } catch (const std::exception& e) {
        std::cerr << "HandleGetRegistrySnapshot �쳣: " << e.what() << std::endl;
        json error;
        error["error"] = "��ȡע������ʧ��: " + std::string(e.what());
        res.status = 500;
        res.set_content(error.dump(), "application/json");
    }
}

void HttpServer::HandleSaveRegistrySnapshot(const httplib::Request& req, httplib::Response& res) {
    try {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        
        std::cout << "�յ�����ע����������" << std::endl;
        std::cout << "�������С: " << req.body.size() << " �ֽ�" << std::endl;
        
        // ����JSON����
        json request_json;
        try {
            request_json = json::parse(req.body);
            std::cout << "JSON�����ɹ�" << std::endl;
        } catch (const json::exception& e) {
            std::cerr << "JSON����ʧ��: " << e.what() << std::endl;
            json error;
            error["success"] = false;
            error["error"] = "��Ч��JSON��ʽ";
            res.status = 400;
            res.set_content(error.dump(), "application/json");
            return;
        }
        
        // ��ȡ��������
        std::string snapshotName;
        if (request_json.contains("snapshotName") && request_json["snapshotName"].is_string()) {
            snapshotName = request_json["snapshotName"];
        }
        if (snapshotName.empty()) {
            snapshotName = "snapshot_" + std::to_string(GetTickCount64());
        }
        
        // �������ն��󵫲�������ϸ���ݣ�����������⣩
        RegistrySnapshot snapshot;
        snapshot.timestamp = GetTickCount64();
        
        // ֻ��¼��������������������
        if (request_json.contains("systemInfo") && request_json["systemInfo"].is_array()) {
            snapshot.systemInfoKeys.resize(request_json["systemInfo"].size());
        }
        
        if (request_json.contains("software") && request_json["software"].is_array()) {
            snapshot.softwareKeys.resize(request_json["software"].size());
        }
        
        if (request_json.contains("network") && request_json["network"].is_array()) {
            snapshot.networkKeys.resize(request_json["network"].size());
        }
        
        // �������
        registrySnapshots_[snapshotName] = snapshot;
        
        // ������Ӧ - ȷ�������ַ������ǰ�ȫ��
        json response;
        response["success"] = true;
        response["snapshotName"] = snapshotName; // snapshotName Ӧ���ǰ�ȫ��
        response["timestamp"] = snapshot.timestamp;
        response["systemInfoCount"] = snapshot.systemInfoKeys.size();
        response["softwareCount"] = snapshot.softwareKeys.size();
        response["networkCount"] = snapshot.networkKeys.size();
        response["message"] = "���ձ���ɹ�";
        
        std::cout << "����ע�����ճɹ�: " << snapshotName 
                  << " (ϵͳ: " << snapshot.systemInfoKeys.size()
                  << ", ���: " << snapshot.softwareKeys.size() 
                  << ", ����: " << snapshot.networkKeys.size() << ")" << std::endl;
        
        // ��ȫ��������Ӧ����
        std::string response_str;
        try {
            response_str = response.dump();
            res.set_content(response_str, "application/json");
        } catch (const json::exception& e) {
            std::cerr << "JSON���л�ʧ��: " << e.what() << std::endl;
            // ���˷���������һ���򵥵���Ӧ
            res.set_content("{\"success\":true,\"message\":\"���ձ���ɹ�\"}", "application/json");
        }
        
    } catch (const std::exception& e) {
        std::cerr << "HandleSaveRegistrySnapshot �쳣: " << e.what() << std::endl;
        // ʹ�ü򵥵Ĵ�����Ӧ������JSON����
        res.status = 500;
        res.set_content("{\"success\":false,\"error\":\"�������ڲ�����\"}", "application/json");
    }
}

// void HttpServer::HandleSaveRegistrySnapshot(const httplib::Request& req, httplib::Response& res) {
//     try {
//         res.set_header("Access-Control-Allow-Origin", "*");
//         res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
//         res.set_header("Access-Control-Allow-Headers", "Content-Type");
        
//         // ���������־
//         std::cout << "�յ�����ע����������" << std::endl;
//         std::cout << "�������С: " << req.body.size() << " �ֽ�" << std::endl;
        
//         // ����JSON����
//         json request_json;
//         try {
//             request_json = json::parse(req.body);
//             std::cout << "JSON�����ɹ�" << std::endl;
//         } catch (const json::exception& e) {
//             std::cerr << "JSON����ʧ��: " << e.what() << std::endl;
//             json error;
//             error["success"] = false;
//             error["error"] = "��Ч��JSON��ʽ: " + std::string(e.what());
//             res.status = 400;
//             res.set_content(error.dump(), "application/json");
//             return;
//         }
        
//         // ��ȡ��������
//         std::string snapshotName;
//         if (request_json.contains("snapshotName") && !request_json["snapshotName"].is_null()) {
//             snapshotName = request_json["snapshotName"];
//         }
//         if (snapshotName.empty()) {
//             snapshotName = "snapshot_" + std::to_string(GetTickCount64());
//         }
        
//         // ��ȡע�������
//         RegistrySnapshot snapshot;
//         snapshot.timestamp = GetTickCount64();
        
//         // ��ȫ����ȡϵͳ��Ϣ
//         if (request_json.contains("systemInfo") && request_json["systemInfo"].is_array()) {
//             try {
//                 snapshot.systemInfoKeys = ParseRegistryKeysFromJson(request_json["systemInfo"]);
//             } catch (const std::exception& e) {
//                 std::cerr << "����ϵͳ��Ϣʧ��: " << e.what() << std::endl;
//                 // ����������������
//             }
//         }
        
//         // ��ȫ����ȡ�����Ϣ
//         if (request_json.contains("software") && request_json["software"].is_array()) {
//             try {
//                 snapshot.softwareKeys = ParseRegistryKeysFromJson(request_json["software"]);
//             } catch (const std::exception& e) {
//                 std::cerr << "���������Ϣʧ��: " << e.what() << std::endl;
//             }
//         }
        
//         // ��ȫ����ȡ��������
//         if (request_json.contains("network") && request_json["network"].is_array()) {
//             try {
//                 snapshot.networkKeys = ParseRegistryKeysFromJson(request_json["network"]);
//             } catch (const std::exception& e) {
//                 std::cerr << "������������ʧ��: " << e.what() << std::endl;
//             }
//         }
        
//         // �������
//         registrySnapshots_[snapshotName] = snapshot;
        
//         json response;
//         response["success"] = true;
//         response["snapshotName"] = snapshotName;
//         response["timestamp"] = snapshot.timestamp;
//         response["systemInfoCount"] = snapshot.systemInfoKeys.size();
//         response["softwareCount"] = snapshot.softwareKeys.size();
//         response["networkCount"] = snapshot.networkKeys.size();
//         response["message"] = "���ձ���ɹ�";
        
//         std::cout << "����ע�����ճɹ�: " << snapshotName 
//                   << " (ϵͳ: " << snapshot.systemInfoKeys.size()
//                   << ", ���: " << snapshot.softwareKeys.size() 
//                   << ", ����: " << snapshot.networkKeys.size() << ")" << std::endl;
        
//         res.set_content(response.dump(), "application/json");
        
//     } catch (const std::exception& e) {
//         std::cerr << "HandleSaveRegistrySnapshot �쳣: " << e.what() << std::endl;
//         json error;
//         error["success"] = false;
//         error["error"] = "�������ڲ�����: " + std::string(e.what());
//         res.status = 500;
//         res.set_content(error.dump(), "application/json");
//     }
// }

// ��Ӱ�ȫ����JSON�ķ���
std::vector<RegistryKey> HttpServer::ParseRegistryKeysFromJson(const json& json_array) {
    std::vector<RegistryKey> keys;
    
    if (!json_array.is_array()) {
        return keys;
    }
    
    for (const auto& item : json_array) {
        try {
            RegistryKey key;
            
            // ��ȫ��ȡ·��
            if (item.contains("path") && item["path"].is_string()) {
                key.path = SafeJsonToString(item["path"]);
            }
            
            // ��ȫ��ȡֵ
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
            
            // ��ȫ��ȡ�Ӽ�
            if (item.contains("subkeys") && item["subkeys"].is_array()) {
                for (const auto& subkey_item : item["subkeys"]) {
                    if (subkey_item.is_string()) {
                        key.subkeys.push_back(SafeJsonToString(subkey_item));
                    }
                }
            }
            
            keys.push_back(key);
            
        } catch (const std::exception& e) {
            std::cerr << "����ע����ʧ��: " << e.what() << std::endl;
            // ����������ļ�������������һ��
        }
    }
    
    return keys;
}

// ��ȫ�ش�JSON��ȡ�ַ���
std::string HttpServer::SafeJsonToString(const json& json_value) {
    if (json_value.is_null()) {
        return "";
    }
    
    if (json_value.is_string()) {
        try {
            std::string str = json_value.get<std::string>();
            return SanitizeUTF8(str);
        } catch (const std::exception& e) {
            std::cerr << "JSON�ַ���ת��ʧ��: " << e.what() << std::endl;
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

// UTF-8����������HttpServer������ӣ�
std::string HttpServer::SanitizeUTF8(const std::string& str) {
    std::string result;
    result.reserve(str.length());
    
    for (size_t i = 0; i < str.length(); ) {
        unsigned char c = static_cast<unsigned char>(str[i]);
        
        if (c <= 0x7F) {
            // ASCII�ַ�
            result += c;
            i++;
        } else if ((c & 0xE0) == 0xC0) {
            // 2�ֽ�UTF-8�ַ�
            if (i + 1 < str.length() && (str[i+1] & 0xC0) == 0x80) {
                result += str.substr(i, 2);
                i += 2;
            } else {
                result += '?';
                i++;
            }
        } else if ((c & 0xF0) == 0xE0) {
            // 3�ֽ�UTF-8�ַ�
            if (i + 2 < str.length() && (str[i+1] & 0xC0) == 0x80 && (str[i+2] & 0xC0) == 0x80) {
                result += str.substr(i, 3);
                i += 3;
            } else {
                result += '?';
                i++;
            }
        } else if ((c & 0xF8) == 0xF0) {
            // 4�ֽ�UTF-8�ַ�
            if (i + 3 < str.length() && (str[i+1] & 0xC0) == 0x80 && 
                (str[i+2] & 0xC0) == 0x80 && (str[i+3] & 0xC0) == 0x80) {
                result += str.substr(i, 4);
                i += 4;
            } else {
                result += '?';
                i++;
            }
        } else {
            // ��Ч��UTF-8�ֽ�
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
        
        std::cout << "���ر���Ŀ����б�����: " << registrySnapshots_.size() << std::endl;
        
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        std::cerr << "HandleGetSavedSnapshots �쳣: " << e.what() << std::endl;
        json error;
        error["error"] = "��ȡ�����б�ʧ��";
        res.status = 500;
        res.set_content(error.dump(), "application/json");
    }
}


// �����������Ƚ�����ע�����б�
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
    
    // ���������ļ�
    for (const auto& [path, key] : map2) {
        if (map1.find(path) == map1.end()) {
            changes["added"].push_back(path);
        }
    }
    
    // ����ɾ���ļ�
    for (const auto& [path, key] : map1) {
        if (map2.find(path) == map2.end()) {
            changes["removed"].push_back(path);
        }
    }
    
    // �����޸ĵļ�
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
        
        // ����JSON������
        json request_json;
        try {
            request_json = json::parse(req.body);
        } catch (const json::exception& e) {
            json error;
            error["success"] = false;
            error["error"] = "��Ч��JSON��ʽ: " + std::string(e.what());
            res.status = 400;
            res.set_content(error.dump(), "application/json");
            return;
        }
        
        // ��JSON�л�ȡ����
        std::string snapshot1, snapshot2;
        if (request_json.contains("snapshot1") && request_json["snapshot1"].is_string()) {
            snapshot1 = request_json["snapshot1"];
        }
        if (request_json.contains("snapshot2") && request_json["snapshot2"].is_string()) {
            snapshot2 = request_json["snapshot2"];
        }
        
        std::cout << "�յ��ȽϿ�������: " << snapshot1 << " vs " << snapshot2 << std::endl;
        
        if (snapshot1.empty() || snapshot2.empty()) {
            json error;
            error["success"] = false;
            error["error"] = "��Ҫ�ṩ������������";
            res.status = 400;
            res.set_content(error.dump(), "application/json");
            return;
        }
        
        // �������Ƿ����
        if (registrySnapshots_.find(snapshot1) == registrySnapshots_.end()) {
            json error;
            error["success"] = false;
            error["error"] = "���ղ�����: " + snapshot1;
            res.status = 404;
            res.set_content(error.dump(), "application/json");
            return;
        }
        
        if (registrySnapshots_.find(snapshot2) == registrySnapshots_.end()) {
            json error;
            error["success"] = false;
            error["error"] = "���ղ�����: " + snapshot2;
            res.status = 404;
            res.set_content(error.dump(), "application/json");
            return;
        }
        
        // ��ȡ����
        const auto& snap1 = registrySnapshots_[snapshot1];
        const auto& snap2 = registrySnapshots_[snapshot2];
        
        json response;
        response["snapshot1"] = snapshot1;
        response["snapshot2"] = snapshot2;
        response["timestamp1"] = snap1.timestamp;
        response["timestamp2"] = snap2.timestamp;
        
        // �Ƚ�ϵͳ��Ϣ
        response["systemInfoChanges"] = CompareRegistrySnapshots(snap1.systemInfoKeys, snap2.systemInfoKeys);
        // �Ƚ������Ϣ
        response["softwareChanges"] = CompareRegistrySnapshots(snap1.softwareKeys, snap2.softwareKeys);
        // �Ƚ���������
        response["networkChanges"] = CompareRegistrySnapshots(snap1.networkKeys, snap2.networkKeys);
        
        std::cout << "�ȽϿ������: " << snapshot1 << " vs " << snapshot2 << std::endl;
        
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        std::cerr << "HandleCompareSnapshots �쳣: " << e.what() << std::endl;
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
        
        // ��·��������ȡ��������
        if (req.matches.size() < 2) {
            res.status = 400;
            res.set_content(R"({"success":false,"error":"ȱ�ٿ������Ʋ���"})", "application/json");
            return;
        }
        
        std::string snapshotName = req.matches[1];
        
        std::cout << "�յ�ɾ����������ԭʼ����: " << snapshotName << std::endl;
        
        if (snapshotName.empty()) {
            res.status = 400;
            res.set_content(R"({"success":false,"error":"�������Ʋ���Ϊ��"})", "application/json");
            return;
        }
        
        // ������������еķ�UTF-8�ַ�
        std::string cleanSnapshotName = SanitizeUTF8(snapshotName);
        std::cout << "�����Ŀ�������: " << cleanSnapshotName << std::endl;
        
        // ��map�в���ƥ��Ŀ���
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
            // ʹ��ԭʼ����ɾ��
            registrySnapshots_.erase(actualSnapshotName);
            
            // ����JSON��Ӧ
            json response;
            response["success"] = true;
            response["message"] = "����ɾ���ɹ�";
            
            // ��ȫ�����л�JSON
            std::string response_str;
            try {
                response_str = response.dump();
            } catch (const json::exception& e) {
                std::cerr << "JSON���л�ʧ�ܣ�ʹ�û�����Ӧ: " << e.what() << std::endl;
                response_str = R"({"success":true,"message":"����ɾ���ɹ�"})";
            }
            
            res.set_content(response_str, "application/json");
            std::cout << "ɾ�����ճɹ�: " << actualSnapshotName << std::endl;
            
        } else {
            // ���ղ�����
            json error;
            error["success"] = false;
            error["error"] = "���ղ�����: " + cleanSnapshotName;
            
            std::string error_str;
            try {
                error_str = error.dump();
            } catch (const json::exception& e) {
                std::cerr << "����JSON���л�ʧ��: " << e.what() << std::endl;
                error_str = R"({"success":false,"error":"���ղ�����"})";
            }
            
            res.status = 404;
            res.set_content(error_str, "application/json");
            std::cout << "ɾ������ʧ��: ���ղ����� - " << cleanSnapshotName << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "HandleDeleteSnapshot �쳣: " << e.what() << std::endl;
        res.status = 500;
        res.set_content(R"({"success":false,"error":"ɾ������ʧ��"})", "application/json");
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
        
        // ͳ����Ϣ
        json statsJson;
        statsJson["totalDrivers"] = snapshot.stats.totalDrivers;
        statsJson["runningCount"] = snapshot.stats.runningCount;
        statsJson["stoppedCount"] = snapshot.stats.stoppedCount;
        statsJson["kernelCount"] = snapshot.stats.kernelCount;
        statsJson["fileSystemCount"] = snapshot.stats.fileSystemCount;
        statsJson["autoStartCount"] = snapshot.stats.autoStartCount;
        statsJson["thirdPartyCount"] = snapshot.stats.thirdPartyCount;
        response["statistics"] = statsJson;
        
        // �ں�����
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
        
        // �ļ�ϵͳ����
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
        
        // �����е�����
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
        
        // ��ֹͣ������
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
        
        // �Զ�����������
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
        
        // ����������
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
        std::cout << "������������ - "
                  << "�ں�����: " << snapshot.kernelDrivers.size() << " ��, "
                  << "�ļ�ϵͳ����: " << snapshot.fileSystemDrivers.size() << " ��, "
                  << "������: " << snapshot.runningDrivers.size() << " ��" << std::endl;
        
        res.set_content(responseStr, "application/json");
        
    } catch (const std::exception& e) {
        std::cerr << "HandleGetDriverSnapshot �쳣: " << e.what() << std::endl;
        json error;
        error["error"] = "��ȡ��������ʧ��: " + std::string(e.what());
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
            error["error"] = "ȱ���������Ʋ���";
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
        std::cerr << "HandleGetDriverDetail �쳣: " << e.what() << std::endl;
        json error;
        error["error"] = "��ȡ��������ʧ��: " + std::string(e.what());
        res.status = 500;
        res.set_content(error.dump(), "application/json");
    }
}






} // namespace snapshot