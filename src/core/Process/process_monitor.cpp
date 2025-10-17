#include "process_monitor.h"
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <wtsapi32.h>
#include <sddl.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <comdef.h>
#include <wbemidl.h>

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "wtsapi32.lib")
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "advapi32.lib")

namespace sysmonitor {

ProcessMonitor::ProcessMonitor() : lastSystemTime_(0) {
    Initialize();
}

ProcessMonitor::~ProcessMonitor() {
    Cleanup();
}

bool ProcessMonitor::Initialize() {
    return true;
}

void ProcessMonitor::Cleanup() {
    processCpuData_.clear();
}

ProcessSnapshot ProcessMonitor::GetProcessSnapshot() {
    ProcessSnapshot snapshot;
    snapshot.timestamp = GetTickCount64();
    
    // Use ToolHelp API to get process list (most reliable method)
    auto processes = GetProcessesViaToolHelp();
    snapshot.processes = std::move(processes);
    snapshot.totalProcesses = static_cast<uint32_t>(snapshot.processes.size());
    
    // Calculate total thread count
    snapshot.totalThreads = 0;
    for (const auto& process : snapshot.processes) {
        snapshot.totalThreads += process.threadCount;
    }
    
    return snapshot;
}

std::vector<ProcessInfo> ProcessMonitor::GetProcessesViaToolHelp() {
    std::vector<ProcessInfo> processes;
    
    // Create process snapshot
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS | TH32CS_SNAPMODULE, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to create process snapshot: " << GetLastError() << std::endl;
        return processes;
    }
    
    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);
    
    // Iterate through all processes
    if (Process32First(snapshot, &processEntry)) {
        do {
            ProcessInfo info;
            info.pid = processEntry.th32ProcessID;
            info.parentPid = processEntry.th32ParentProcessID;
            info.name = processEntry.szExeFile;
            info.threadCount = processEntry.cntThreads;
            info.priority = processEntry.pcPriClassBase;
            
            // Get more detailed information
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 
                                         FALSE, processEntry.th32ProcessID);
            
            if (hProcess) {
                // Get process path
                char path[MAX_PATH];
                if (GetModuleFileNameExA(hProcess, NULL, path, MAX_PATH)) {
                    info.fullPath = path;
                }
                
                // Get memory information
                PROCESS_MEMORY_COUNTERS pmc;
                if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
                    info.memoryUsage = pmc.WorkingSetSize;
                    info.workingSetSize = pmc.WorkingSetSize;
                    info.pagefileUsage = pmc.PagefileUsage;
                }
                
                // Get username
                info.username = GetProcessUsername(processEntry.th32ProcessID);
                
                // Get command line
                info.commandLine = GetProcessCommandLine(processEntry.th32ProcessID);
                
                // Get creation time
                FILETIME createTime, exitTime, kernelTime, userTime;
                if (GetProcessTimes(hProcess, &createTime, &exitTime, &kernelTime, &userTime)) {
                    ULARGE_INTEGER createTimeValue;
                    createTimeValue.LowPart = createTime.dwLowDateTime;
                    createTimeValue.HighPart = createTime.dwHighDateTime;
                    info.createTime = createTimeValue.QuadPart;
                }
                
                // Calculate CPU usage
                info.cpuUsage = GetProcessCpuUsage(processEntry.th32ProcessID);
                
                CloseHandle(hProcess);
            }
            
            // Set process state
            info.state = "Running";
            
            processes.push_back(info);
            
        } while (Process32Next(snapshot, &processEntry));
    }
    
    CloseHandle(snapshot);
    return processes;
}

std::string ProcessMonitor::GetProcessUsername(uint32_t pid) {
    std::string username;
    
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!hProcess) {
        return "Unknown";
    }
    
    HANDLE hToken = NULL;
    if (OpenProcessToken(hProcess, TOKEN_QUERY, &hToken)) {
        DWORD tokenInfoSize = 0;
        GetTokenInformation(hToken, TokenUser, NULL, 0, &tokenInfoSize);
        
        if (tokenInfoSize > 0) {
            auto tokenInfo = std::make_unique<BYTE[]>(tokenInfoSize);
            if (GetTokenInformation(hToken, TokenUser, tokenInfo.get(), tokenInfoSize, &tokenInfoSize)) {
                PTOKEN_USER pTokenUser = (PTOKEN_USER)tokenInfo.get();
                
                // Convert SID to username
                char name[256];
                char domain[256];
                DWORD nameSize = sizeof(name);
                DWORD domainSize = sizeof(domain);
                SID_NAME_USE sidType;
                
                if (LookupAccountSidA(NULL, pTokenUser->User.Sid, name, &nameSize, 
                                    domain, &domainSize, &sidType)) {
                    username = std::string(domain) + "\\" + name;
                }
            }
        }
        
        CloseHandle(hToken);
    }
    
    CloseHandle(hProcess);
    
    if (username.empty()) {
        username = "SYSTEM";
    }
    
    return username;
}

std::string ProcessMonitor::GetProcessCommandLine(uint32_t pid) {
    std::string commandLine;
    
    // Higher privileges required to get command line
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!hProcess) {
        return "";
    }
    
    // Use WMI to get command line (more reliable method)
    // Simplified implementation here, actual implementation can use WMI queries
    HMODULE hNtDll = GetModuleHandleA("ntdll.dll");
    if (hNtDll) {
        typedef NTSTATUS(NTAPI* _NtQueryInformationProcess)(
            HANDLE, int, PVOID, ULONG, PULONG);
        
        auto NtQueryInformationProcess = (_NtQueryInformationProcess)
            GetProcAddress(hNtDll, "NtQueryInformationProcess");
        
        if (NtQueryInformationProcess) {
            // Simplified implementation, return process path as command line
            char path[MAX_PATH];
            if (GetModuleFileNameExA(hProcess, NULL, path, MAX_PATH)) {
                commandLine = path;
            }
        }
    }
    
    CloseHandle(hProcess);
    return commandLine;
}

double ProcessMonitor::GetProcessCpuUsage(uint32_t pid) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!hProcess) {
        return 0.0;
    }
    
    FILETIME createTime, exitTime, kernelTime, userTime;
    FILETIME sysIdleTime, sysKernelTime, sysUserTime;
    
    if (!GetProcessTimes(hProcess, &createTime, &exitTime, &kernelTime, &userTime) ||
        !GetSystemTimes(&sysIdleTime, &sysKernelTime, &sysUserTime)) {
        CloseHandle(hProcess);
        return 0.0;
    }
    
    ULARGE_INTEGER currentKernelTime, currentUserTime;
    currentKernelTime.LowPart = kernelTime.dwLowDateTime;
    currentKernelTime.HighPart = kernelTime.dwHighDateTime;
    currentUserTime.LowPart = userTime.dwLowDateTime;
    currentUserTime.HighPart = userTime.dwHighDateTime;
    
    uint64_t currentProcessTime = currentKernelTime.QuadPart + currentUserTime.QuadPart;
    uint64_t currentSystemTime = 0;
    
    ULARGE_INTEGER sysKernel, sysUser;
    sysKernel.LowPart = sysKernelTime.dwLowDateTime;
    sysKernel.HighPart = sysKernelTime.dwHighDateTime;
    sysUser.LowPart = sysUserTime.dwLowDateTime;
    sysUser.HighPart = sysUserTime.dwHighDateTime;
    currentSystemTime = sysKernel.QuadPart + sysUser.QuadPart;
    
    double cpuUsage = 0.0;
    
    // Check if historical data exists
    auto it = processCpuData_.find(pid);
    if (it != processCpuData_.end() && lastSystemTime_ > 0) {
        const auto& prevData = it->second;
        
        uint64_t processTimeDiff = currentProcessTime - prevData.kernelTime - prevData.userTime;
        uint64_t systemTimeDiff = currentSystemTime - lastSystemTime_;
        
        if (systemTimeDiff > 0) {
            cpuUsage = (100.0 * processTimeDiff) / systemTimeDiff;
            // cpuUsage = std::max(0.0, std::min(100.0, cpuUsage));
            cpuUsage = cpuUsage < 0.0 ? 0.0 : (cpuUsage > 100.0 ? 100.0 : cpuUsage);
        }
    }
    
    // Update data
    processCpuData_[pid] = {
        currentKernelTime.QuadPart,
        currentUserTime.QuadPart,
        GetTickCount64()
    };
    
    lastSystemTime_ = currentSystemTime;
    
    CloseHandle(hProcess);
    return cpuUsage;
}

ProcessInfo ProcessMonitor::GetProcessInfo(uint32_t pid) {
    auto snapshot = GetProcessSnapshot();
    for (const auto& process : snapshot.processes) {
        if (process.pid == pid) {
            return process;
        }
    }
    
    // Return empty ProcessInfo indicating not found
    return ProcessInfo();
}

std::vector<ProcessInfo> ProcessMonitor::FindProcessesByName(const std::string& name) {
    auto snapshot = GetProcessSnapshot();
    std::vector<ProcessInfo> result;
    
    for (const auto& process : snapshot.processes) {
        if (_stricmp(process.name.c_str(), name.c_str()) == 0) {
            result.push_back(process);
        }
    }
    
    return result;
}

bool ProcessMonitor::TerminateProcess(uint32_t pid, uint32_t exitCode) {
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!hProcess) {
        return false;
    }
    
    BOOL result = ::TerminateProcess(hProcess, exitCode);
    CloseHandle(hProcess);
    
    return result != FALSE;
}

} // namespace sysmonitor