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

// Global variable for signal handling
std::atomic<bool> g_running{true};

void SignalHandler(int signal) {
    std::cout << "\nReceived interrupt signal, shutting down server..." << std::endl;
    g_running = false;
}

void PrintCPUInfo(const CPUInfo& info) {
    std::cout << "=== CPU Detailed Information ===" << std::endl;
    std::cout << "Name: " << info.name << std::endl;
    std::cout << "Architecture: " << info.architecture << std::endl;
    std::cout << "Physical Cores: " << info.physicalCores << std::endl;
    std::cout << "Logical Cores: " << info.logicalCores << std::endl;
    std::cout << "Processor Packages: " << info.packages << std::endl;
    std::cout << "Base Frequency: " << info.baseFrequency << " MHz" << std::endl;
    if (info.maxFrequency > 0) {
        std::cout << "Max Frequency: " << info.maxFrequency << " MHz" << std::endl;
    }
    std::cout << std::endl;
}

int main() {
    // Set console output encoding
    system("chcp 936 > nul");
    
    // Register signal handlers
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    std::cout << "System Monitoring Tool - HTTP Server Version" << std::endl;
    
    // Display CPU information
    CPUInfo info = SystemInfo::GetCPUInfo();
    PrintCPUInfo(info);
    
    // Use WMI to get more detailed information (optional)
    CPUInfo detailedInfo = WMIHelper::GetDetailedCPUInfo();
    if (!detailedInfo.name.empty()) {
        std::cout << "=== WMI Detailed Information ===" << std::endl;
        std::cout << "WMI Processor Name: " << detailedInfo.name << std::endl;
        std::cout << "WMI Physical Cores: " << detailedInfo.physicalCores << std::endl;
        std::cout << "WMI Logical Cores: " << detailedInfo.logicalCores << std::endl;
        std::cout << "WMI Max Frequency: " << detailedInfo.maxFrequency << " MHz" << std::endl;
        std::cout << std::endl;
    }
    
    // Start HTTP server
    HttpServer server;
    if (server.Start(8080)) {
        std::cout << "Server started successfully!" << std::endl;
        std::cout << "Open browser and visit: http://localhost:8080" << std::endl;
        std::cout << "Press Ctrl+C to stop the server" << std::endl;
        
        // Wait for exit signal
        while (g_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        server.Stop();
        std::cout << "Server stopped" << std::endl;
    } else {
        std::cerr << "Failed to start server" << std::endl;
        return -1;
    }
    
    WMIHelper::Uninitialize();
    return 0;
}