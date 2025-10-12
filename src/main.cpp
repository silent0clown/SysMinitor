#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <csignal>
#include "core/stdafx.h"
#include "core/CPUInfo/cpu_monitor.h"
#include "core/CPUInfo/wmi_helper.h"
#include "server/WebServer.h"

using namespace sysmonitor;

// 全局变量用于信号处理
std::atomic<bool> g_running{true};

void SignalHandler(int signal) {
    std::cout << "\n接收到中断信号，正在关闭服务器..." << std::endl;
    g_running = false;
}

void PrintCPUInfo(const CPUInfo& info) {
    std::cout << "=== CPU详细信息 ===" << std::endl;
    std::cout << "名称: " << info.name << std::endl;
    std::cout << "架构: " << info.architecture << std::endl;
    std::cout << "物理核心: " << info.physicalCores << std::endl;
    std::cout << "逻辑核心: " << info.logicalCores << std::endl;
    std::cout << "处理器封装: " << info.packages << std::endl;
    std::cout << "基础频率: " << info.baseFrequency << " MHz" << std::endl;
    if (info.maxFrequency > 0) {
        std::cout << "最大频率: " << info.maxFrequency << " MHz" << std::endl;
    }
    std::cout << std::endl;
}

int main() {
    // 设置控制台输出编码
    system("chcp 936 > nul");
    
    // 注册信号处理
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    std::cout << "系统监控工具 - HTTP服务器版本" << std::endl;
    
    // 显示CPU信息
    CPUInfo info = SystemInfo::GetCPUInfo();
    PrintCPUInfo(info);
    
    // 使用WMI获取更详细信息（可选）
    CPUInfo detailedInfo = WMIHelper::GetDetailedCPUInfo();
    if (!detailedInfo.name.empty()) {
        std::cout << "=== WMI详细信息 ===" << std::endl;
        std::cout << "WMI处理器名称: " << detailedInfo.name << std::endl;
        std::cout << "WMI物理核心: " << detailedInfo.physicalCores << std::endl;
        std::cout << "WMI逻辑核心: " << detailedInfo.logicalCores << std::endl;
        std::cout << "WMI最大频率: " << detailedInfo.maxFrequency << " MHz" << std::endl;
        std::cout << std::endl;
    }
    
    // 启动HTTP服务器
    HttpServer server;
    if (server.Start(8080)) {
        std::cout << "服务器启动成功！" << std::endl;
        std::cout << "打开浏览器访问: http://localhost:8080" << std::endl;
        std::cout << "按 Ctrl+C 停止服务器" << std::endl;
        
        // 等待退出信号
        while (g_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        server.Stop();
        std::cout << "服务器已停止" << std::endl;
    } else {
        std::cerr << "服务器启动失败" << std::endl;
        return -1;
    }
    
    WMIHelper::Uninitialize();
    return 0;
}