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

// ȫ�ֱ��������źŴ���
std::atomic<bool> g_running{true};

void SignalHandler(int signal) {
    std::cout << "\n���յ��ж��źţ����ڹرշ�����..." << std::endl;
    g_running = false;
}

void PrintCPUInfo(const CPUInfo& info) {
    std::cout << "=== CPU��ϸ��Ϣ ===" << std::endl;
    std::cout << "����: " << info.name << std::endl;
    std::cout << "�ܹ�: " << info.architecture << std::endl;
    std::cout << "�������: " << info.physicalCores << std::endl;
    std::cout << "�߼�����: " << info.logicalCores << std::endl;
    std::cout << "��������װ: " << info.packages << std::endl;
    std::cout << "����Ƶ��: " << info.baseFrequency << " MHz" << std::endl;
    if (info.maxFrequency > 0) {
        std::cout << "���Ƶ��: " << info.maxFrequency << " MHz" << std::endl;
    }
    std::cout << std::endl;
}

int main() {
    // ���ÿ���̨�������
    system("chcp 936 > nul");
    
    // ע���źŴ���
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    std::cout << "ϵͳ��ع��� - HTTP�������汾" << std::endl;
    
    // ��ʾCPU��Ϣ
    CPUInfo info = SystemInfo::GetCPUInfo();
    PrintCPUInfo(info);
    
    // ʹ��WMI��ȡ����ϸ��Ϣ����ѡ��
    CPUInfo detailedInfo = WMIHelper::GetDetailedCPUInfo();
    if (!detailedInfo.name.empty()) {
        std::cout << "=== WMI��ϸ��Ϣ ===" << std::endl;
        std::cout << "WMI����������: " << detailedInfo.name << std::endl;
        std::cout << "WMI�������: " << detailedInfo.physicalCores << std::endl;
        std::cout << "WMI�߼�����: " << detailedInfo.logicalCores << std::endl;
        std::cout << "WMI���Ƶ��: " << detailedInfo.maxFrequency << " MHz" << std::endl;
        std::cout << std::endl;
    }
    
    // ����HTTP������
    HttpServer server;
    if (server.Start(8080)) {
        std::cout << "�����������ɹ���" << std::endl;
        std::cout << "�����������: http://localhost:8080" << std::endl;
        std::cout << "�� Ctrl+C ֹͣ������" << std::endl;
        
        // �ȴ��˳��ź�
        while (g_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        server.Stop();
        std::cout << "��������ֹͣ" << std::endl;
    } else {
        std::cerr << "����������ʧ��" << std::endl;
        return -1;
    }
    
    WMIHelper::Uninitialize();
    return 0;
}