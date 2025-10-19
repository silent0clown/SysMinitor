#include "driver_monitor.h"
#include <windows.h>
#include <winsvc.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <cctype>  // Add cctype header
#include "../../utils/util_time.h"
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "cfgmgr32.lib")
#pragma comment(lib, "version.lib")  // Add version information library

// Fix conversion function
namespace {
    char toLowerChar(char c) {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
}

DriverMonitor::DriverMonitor() {}

DriverMonitor::~DriverMonitor() {}

DriverSnapshot DriverMonitor::GetDriverSnapshot() {
    DriverSnapshot snapshot;
    snapshot.timestamp = GET_LOCAL_TIME_MS();
    
    try {
        // Get service drivers
        auto scmDrivers = GetAllDriversViaSCM();
        
        // Get hardware drivers
        auto hardwareDrivers = GetHardwareDriversViaSetupAPI();
        
        // Merge all drivers
        std::vector<DriverDetail> allDrivers = scmDrivers;
        allDrivers.insert(allDrivers.end(), hardwareDrivers.begin(), hardwareDrivers.end());
        
        // Categorize drivers
        CategorizeDrivers(snapshot, allDrivers);
        CategorizeHardwareDrivers(snapshot, hardwareDrivers);
        
        // Calculate statistics
        CalculateStatistics(snapshot);
        
        // Encoding cleanup
        SanitizeAllDrivers(snapshot);
        
        std::cout << "Driver snapshot retrieved successfully - "
                  << "Kernel drivers: " << snapshot.kernelDrivers.size() << ", "
                  << "File system drivers: " << snapshot.fileSystemDrivers.size() << ", "
                  << "Hardware drivers: " << snapshot.hardwareDrivers.size() << std::endl;
                  
    } catch (const std::exception& e) {
        std::cerr << "Exception occurred while getting driver snapshot: " << e.what() << std::endl;
    }
    
    return snapshot;
}

std::vector<DriverDetail> DriverMonitor::GetAllDriversViaSCM() {
    std::vector<DriverDetail> drivers;
    
    SC_HANDLE scm = OpenSCManager(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE);
    if (!scm) {
        throw std::runtime_error("Unable to open Service Control Manager");
    }
    
    DWORD bufferSize = 0;
    DWORD serviceCount = 0;
    DWORD resumeHandle = 0;
    
    // Get required buffer size
    EnumServicesStatusEx(
        scm, SC_ENUM_PROCESS_INFO, SERVICE_DRIVER, SERVICE_STATE_ALL,
        nullptr, 0, &bufferSize, &serviceCount, &resumeHandle, nullptr);
    
    if (GetLastError() != ERROR_MORE_DATA) {
        CloseServiceHandle(scm);
        return drivers;
    }
    
    // Allocate buffer and get service information
    std::vector<BYTE> buffer(bufferSize);
    ENUM_SERVICE_STATUS_PROCESS* services = 
        reinterpret_cast<ENUM_SERVICE_STATUS_PROCESS*>(buffer.data());
    
    if (EnumServicesStatusEx(
        scm, SC_ENUM_PROCESS_INFO, SERVICE_DRIVER, SERVICE_STATE_ALL,
        buffer.data(), bufferSize, &bufferSize, &serviceCount, &resumeHandle, nullptr)) {
        
        for (DWORD i = 0; i < serviceCount; ++i) {
            DriverDetail driver;
            driver.name = services[i].lpServiceName ? services[i].lpServiceName : "";
            driver.displayName = services[i].lpDisplayName ? services[i].lpDisplayName : "";
            driver.driverType = "Service"; // Mark as service driver
            
            // Get detailed service information
            SC_HANDLE service = OpenService(scm, services[i].lpServiceName, 
                                          SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS);
            if (service) {
                // Get service configuration
                DWORD configSize = 0;
                QueryServiceConfig(service, nullptr, 0, &configSize);
                if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                    std::vector<BYTE> configBuffer(configSize);
                    QUERY_SERVICE_CONFIG* config = 
                        reinterpret_cast<QUERY_SERVICE_CONFIG*>(configBuffer.data());
                    
                    if (QueryServiceConfig(service, config, configSize, &configSize)) {
                        driver.binaryPath = config->lpBinaryPathName ? config->lpBinaryPathName : "";
                        driver.account = config->lpServiceStartName ? config->lpServiceStartName : "";
                        
                        // Start type
                        switch (config->dwStartType) {
                            case SERVICE_BOOT_START: driver.startType = "Boot"; break;
                            case SERVICE_SYSTEM_START: driver.startType = "System"; break;
                            case SERVICE_AUTO_START: driver.startType = "Auto"; break;
                            case SERVICE_DEMAND_START: driver.startType = "Manual"; break;
                            case SERVICE_DISABLED: driver.startType = "Disabled"; break;
                            default: driver.startType = "Unknown"; break;
                        }
                        
                        // Service type
                        switch (config->dwServiceType) {
                            case SERVICE_KERNEL_DRIVER:
                                driver.serviceType = "Kernel Driver";
                                driver.driverType = "Kernel";
                                break;
                            case SERVICE_FILE_SYSTEM_DRIVER:
                                driver.serviceType = "File System Driver"; 
                                driver.driverType = "FileSystem";
                                break;
                            default:
                                driver.serviceType = "Unknown";
                                break;
                        }
                        
                        // Error control
                        switch (config->dwErrorControl) {
                            case SERVICE_ERROR_IGNORE: driver.errorControl = "Ignore"; break;
                            case SERVICE_ERROR_NORMAL: driver.errorControl = "Normal"; break;
                            case SERVICE_ERROR_SEVERE: driver.errorControl = "Severe"; break;
                            case SERVICE_ERROR_CRITICAL: driver.errorControl = "Critical"; break;
                            default: driver.errorControl = "Unknown"; break;
                        }
                    }
                }
                
                // Get service status
                SERVICE_STATUS_PROCESS status;
                DWORD bytesNeeded;
                if (QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, 
                                       reinterpret_cast<LPBYTE>(&status), 
                                       sizeof(status), &bytesNeeded)) {
                    driver.pid = status.dwProcessId;
                    
                    switch (status.dwCurrentState) {
                        case SERVICE_RUNNING: driver.state = "Running"; break;
                        case SERVICE_STOPPED: driver.state = "Stopped"; break;
                        case SERVICE_START_PENDING: driver.state = "Start Pending"; break;
                        case SERVICE_STOP_PENDING: driver.state = "Stop Pending"; break;
                        case SERVICE_PAUSE_PENDING: driver.state = "Pause Pending"; break;
                        case SERVICE_PAUSED: driver.state = "Paused"; break;
                        case SERVICE_CONTINUE_PENDING: driver.state = "Continue Pending"; break;
                        default: driver.state = "Unknown"; break;
                    }
                    
                    driver.win32ExitCode = std::to_string(status.dwWin32ExitCode);
                    driver.serviceSpecificExitCode = std::to_string(status.dwServiceSpecificExitCode);
                }
                
                CloseServiceHandle(service);
            }
            
            // Get version information
            if (!driver.binaryPath.empty()) {
                // Clean path of system directory references
                std::string cleanPath = driver.binaryPath;
                if (cleanPath.find("System32\\") != std::string::npos) {
                    char systemDir[MAX_PATH];
                    GetSystemDirectoryA(systemDir, MAX_PATH);
                    size_t pos = cleanPath.find("System32\\");
                    if (pos != std::string::npos) {
                        cleanPath = std::string(systemDir) + "\\" + cleanPath.substr(pos + 10);
                    }
                }
                driver.version = GetDriverVersionInfo(cleanPath);
            }
            
            drivers.push_back(driver);
        }
    }
    
    CloseServiceHandle(scm);
    return drivers;
}

std::vector<DriverDetail> DriverMonitor::GetHardwareDriversViaSetupAPI() {
    std::vector<DriverDetail> drivers;
    
    // Get device information set
    HDEVINFO deviceInfoSet = SetupDiGetClassDevs(nullptr, nullptr, nullptr, 
                                                DIGCF_ALLCLASSES | DIGCF_PRESENT);
    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
        return drivers;
    }
    
    SP_DEVINFO_DATA deviceInfoData;
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    
    for (DWORD i = 0; SetupDiEnumDeviceInfo(deviceInfoSet, i, &deviceInfoData); i++) {
        DriverDetail driver;
        driver.driverType = "Hardware";
        driver.state = "Running"; // Hardware drivers are usually running
        
        // Get device description
        char deviceDesc[256] = {0};
        if (SetupDiGetDeviceRegistryProperty(deviceInfoSet, &deviceInfoData, 
                                           SPDRP_DEVICEDESC, nullptr, 
                                           (PBYTE)deviceDesc, sizeof(deviceDesc), nullptr)) {
            driver.displayName = deviceDesc;
        }
        
        // Get device name
        char deviceName[256] = {0};
        if (SetupDiGetDeviceRegistryProperty(deviceInfoSet, &deviceInfoData,
                                           SPDRP_FRIENDLYNAME, nullptr,
                                           (PBYTE)deviceName, sizeof(deviceName), nullptr)) {
            driver.name = deviceName;
        } else {
            driver.name = driver.displayName;
        }
        
        // Get hardware class
        char hardwareClass[256] = {0};
        if (SetupDiGetDeviceRegistryProperty(deviceInfoSet, &deviceInfoData,
                                           SPDRP_CLASS, nullptr,
                                           (PBYTE)hardwareClass, sizeof(hardwareClass), nullptr)) {
            driver.hardwareClass = hardwareClass;
        }
        
        // Get driver information
        SP_DRVINFO_DATA driverInfoData;
        driverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
        
        if (SetupDiEnumDriverInfo(deviceInfoSet, &deviceInfoData, 
                                SPDIT_COMPATDRIVER, 0, &driverInfoData)) {
            driver.description = driverInfoData.Description;
            
            // Get driver file path - use hardware ID
            char hardwareID[1024] = {0};
            if (SetupDiGetDeviceRegistryProperty(deviceInfoSet, &deviceInfoData,
                                               SPDRP_HARDWAREID, nullptr,
                                               (PBYTE)hardwareID, sizeof(hardwareID), nullptr)) {
                driver.binaryPath = hardwareID;
            }
            
            // Get INF file path
            char infPath[MAX_PATH] = {0};
            if (SetupDiGetDeviceRegistryProperty(deviceInfoSet, &deviceInfoData,
                                               SPDRP_DRIVER, nullptr,
                                               (PBYTE)infPath, sizeof(infPath), nullptr)) {
                if (strlen(infPath) > 0) {
                    // Try to get version information from INF file
                    std::string infFilePath = infPath;
                    driver.version = GetDriverVersionInfo(infFilePath);
                }
            }
        }
        
        // Detect hardware class
        driver.hardwareClass = DetectHardwareClass(driver);
        
        drivers.push_back(driver);
    }
    
    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    return drivers;
}

DriverVersion DriverMonitor::GetDriverVersionInfo(const std::string& filePath) {
    DriverVersion version;
    
    // Return directly if file path is empty
    if (filePath.empty()) {
        return version;
    }
    
    DWORD versionSize = GetFileVersionInfoSizeA(filePath.c_str(), nullptr);
    if (versionSize == 0) {
        return version;
    }
    
    std::vector<BYTE> versionData(versionSize);
    if (!GetFileVersionInfoA(filePath.c_str(), 0, versionSize, versionData.data())) {
        return version;
    }
    
    // Get version information
    VS_FIXEDFILEINFO* fileInfo;
    UINT fileInfoLen;
    if (VerQueryValueA(versionData.data(), "\\", (LPVOID*)&fileInfo, &fileInfoLen)) {
        version.fileVersion = std::to_string(HIWORD(fileInfo->dwFileVersionMS)) + "." +
                            std::to_string(LOWORD(fileInfo->dwFileVersionMS)) + "." +
                            std::to_string(HIWORD(fileInfo->dwFileVersionLS)) + "." +
                            std::to_string(LOWORD(fileInfo->dwFileVersionLS));
        
        version.productVersion = std::to_string(HIWORD(fileInfo->dwProductVersionMS)) + "." +
                               std::to_string(LOWORD(fileInfo->dwProductVersionMS)) + "." +
                               std::to_string(HIWORD(fileInfo->dwProductVersionLS)) + "." +
                               std::to_string(LOWORD(fileInfo->dwProductVersionLS));
    }
    
    // Get other version information
    struct LANGANDCODEPAGE {
        WORD language;
        WORD codePage;
    } *langCodePage;
    
    UINT langCodePageLen;
    if (VerQueryValueA(versionData.data(), "\\VarFileInfo\\Translation", 
                     (LPVOID*)&langCodePage, &langCodePageLen)) {
        for (UINT i = 0; i < (langCodePageLen / sizeof(LANGANDCODEPAGE)); i++) {
            char subBlock[256];
            sprintf_s(subBlock, sizeof(subBlock), "\\StringFileInfo\\%04x%04x\\", 
                     langCodePage[i].language, langCodePage[i].codePage);
            
            char* value;
            UINT valueLen;
            
            // Company name
            if (VerQueryValueA(versionData.data(), (std::string(subBlock) + "CompanyName").c_str(), 
                             (LPVOID*)&value, &valueLen)) {
                version.companyName = value;
            }
            
            // File description
            if (VerQueryValueA(versionData.data(), (std::string(subBlock) + "FileDescription").c_str(), 
                             (LPVOID*)&value, &valueLen)) {
                version.fileDescription = value;
            }
            
            // Copyright information
            if (VerQueryValueA(versionData.data(), (std::string(subBlock) + "LegalCopyright").c_str(), 
                             (LPVOID*)&value, &valueLen)) {
                version.legalCopyright = value;
            }
            
            // Original filename
            if (VerQueryValueA(versionData.data(), (std::string(subBlock) + "OriginalFilename").c_str(), 
                             (LPVOID*)&value, &valueLen)) {
                version.originalFilename = value;
            }
        }
    }
    
    return version;
}

void DriverMonitor::CategorizeHardwareDrivers(DriverSnapshot& snapshot, const std::vector<DriverDetail>& hardwareDrivers) {
    for (const auto& driver : hardwareDrivers) {
        snapshot.hardwareDrivers.push_back(driver);
        
        // Classify by hardware category
        std::string hardwareClass = driver.hardwareClass;
        std::string name = driver.name;
        std::string displayName = driver.displayName;
        
        // Use safe conversion function
        std::transform(hardwareClass.begin(), hardwareClass.end(), hardwareClass.begin(), toLowerChar);
        std::transform(name.begin(), name.end(), name.begin(), toLowerChar);
        std::transform(displayName.begin(), displayName.end(), displayName.begin(), toLowerChar);
        
        if (hardwareClass.find("display") != std::string::npos ||
            hardwareClass.find("graphics") != std::string::npos ||
            name.find("nvidia") != std::string::npos ||
            name.find("amd") != std::string::npos ||
            name.find("intel") != std::string::npos ||
            name.find("radeon") != std::string::npos ||
            name.find("geforce") != std::string::npos ||
            displayName.find("nvidia") != std::string::npos ||
            displayName.find("amd") != std::string::npos ||
            displayName.find("intel") != std::string::npos) {
            snapshot.displayDrivers.push_back(driver);
        } else if (hardwareClass.find("audio") != std::string::npos ||
                   hardwareClass.find("sound") != std::string::npos ||
                   name.find("realtek") != std::string::npos ||
                   name.find("audio") != std::string::npos ||
                   name.find("sound") != std::string::npos ||
                   displayName.find("realtek") != std::string::npos ||
                   displayName.find("audio") != std::string::npos) {
            snapshot.audioDrivers.push_back(driver);
        } else if (hardwareClass.find("network") != std::string::npos ||
                   hardwareClass.find("net") != std::string::npos ||
                   name.find("ethernet") != std::string::npos ||
                   name.find("wifi") != std::string::npos ||
                   name.find("wireless") != std::string::npos ||
                   name.find("network") != std::string::npos ||
                   displayName.find("ethernet") != std::string::npos ||
                   displayName.find("wifi") != std::string::npos) {
            snapshot.networkDrivers.push_back(driver);
        } else if (hardwareClass.find("keyboard") != std::string::npos ||
                   hardwareClass.find("mouse") != std::string::npos ||
                   hardwareClass.find("input") != std::string::npos ||
                   name.find("keyboard") != std::string::npos ||
                   name.find("mouse") != std::string::npos ||
                   name.find("hid") != std::string::npos ||
                   displayName.find("keyboard") != std::string::npos ||
                   displayName.find("mouse") != std::string::npos) {
            snapshot.inputDrivers.push_back(driver);
        } else if (hardwareClass.find("storage") != std::string::npos ||
                   hardwareClass.find("disk") != std::string::npos ||
                   name.find("storage") != std::string::npos ||
                   name.find("disk") != std::string::npos ||
                   name.find("raid") != std::string::npos ||
                   name.find("sata") != std::string::npos ||
                   displayName.find("storage") != std::string::npos ||
                   displayName.find("disk") != std::string::npos) {
            snapshot.storageDrivers.push_back(driver);
        } else if (hardwareClass.find("print") != std::string::npos ||
                   name.find("print") != std::string::npos ||
                   displayName.find("print") != std::string::npos) {
            snapshot.printerDrivers.push_back(driver);
        } else if (hardwareClass.find("usb") != std::string::npos ||
                   name.find("usb") != std::string::npos ||
                   displayName.find("usb") != std::string::npos) {
            snapshot.usbDrivers.push_back(driver);
        } else if (hardwareClass.find("bluetooth") != std::string::npos ||
                   name.find("bluetooth") != std::string::npos ||
                   displayName.find("bluetooth") != std::string::npos) {
            snapshot.bluetoothDrivers.push_back(driver);
        }
    }
}

void DriverMonitor::CategorizeDrivers(DriverSnapshot& snapshot, const std::vector<DriverDetail>& allDrivers) {
    for (const auto& driver : allDrivers) {
        // Classify by type
        if (driver.driverType == "Kernel") {
            snapshot.kernelDrivers.push_back(driver);
        } else if (driver.driverType == "FileSystem") {
            snapshot.fileSystemDrivers.push_back(driver);
        } else if (driver.driverType == "Hardware") {
            snapshot.hardwareDrivers.push_back(driver);
        }
        
        // Classify by state
        if (driver.state == "Running") {
            snapshot.runningDrivers.push_back(driver);
        } else if (driver.state == "Stopped") {
            snapshot.stoppedDrivers.push_back(driver);
        }
        
        // Classify by start type
        if (driver.startType == "Auto" || driver.startType == "Boot" || driver.startType == "System") {
            snapshot.autoStartDrivers.push_back(driver);
        }
        
        // Determine third-party drivers
        if (driver.binaryPath.find("Microsoft") == std::string::npos && 
            driver.binaryPath.find("Windows") == std::string::npos &&
            !driver.binaryPath.empty()) {
            snapshot.thirdPartyDrivers.push_back(driver);
        }
    }
}

void DriverMonitor::CalculateStatistics(DriverSnapshot& snapshot) {
    size_t total = snapshot.kernelDrivers.size() + snapshot.fileSystemDrivers.size() + snapshot.hardwareDrivers.size();
    
    snapshot.stats.totalDrivers = total;
    snapshot.stats.runningCount = snapshot.runningDrivers.size();
    snapshot.stats.stoppedCount = snapshot.stoppedDrivers.size();
    snapshot.stats.kernelCount = snapshot.kernelDrivers.size();
    snapshot.stats.fileSystemCount = snapshot.fileSystemDrivers.size();
    snapshot.stats.hardwareCount = snapshot.hardwareDrivers.size();
    snapshot.stats.autoStartCount = snapshot.autoStartDrivers.size();
    snapshot.stats.thirdPartyCount = snapshot.thirdPartyDrivers.size();
    
    snapshot.stats.displayCount = snapshot.displayDrivers.size();
    snapshot.stats.audioCount = snapshot.audioDrivers.size();
    snapshot.stats.networkCount = snapshot.networkDrivers.size();
    snapshot.stats.inputCount = snapshot.inputDrivers.size();
    snapshot.stats.storageCount = snapshot.storageDrivers.size();
    snapshot.stats.printerCount = snapshot.printerDrivers.size();
    snapshot.stats.usbCount = snapshot.usbDrivers.size();
    snapshot.stats.bluetoothCount = snapshot.bluetoothDrivers.size();
}

void DriverMonitor::SanitizeAllDrivers(DriverSnapshot& snapshot) {
    auto sanitizeVector = [](std::vector<DriverDetail>& drivers) {
        for (auto& driver : drivers) {
            driver.convertToUTF8();
        }
    };
    
    sanitizeVector(snapshot.kernelDrivers);
    sanitizeVector(snapshot.fileSystemDrivers);
    sanitizeVector(snapshot.hardwareDrivers);
    sanitizeVector(snapshot.runningDrivers);
    sanitizeVector(snapshot.stoppedDrivers);
    sanitizeVector(snapshot.autoStartDrivers);
    sanitizeVector(snapshot.thirdPartyDrivers);
    
    sanitizeVector(snapshot.displayDrivers);
    sanitizeVector(snapshot.audioDrivers);
    sanitizeVector(snapshot.networkDrivers);
    sanitizeVector(snapshot.inputDrivers);
    sanitizeVector(snapshot.storageDrivers);
    sanitizeVector(snapshot.printerDrivers);
    sanitizeVector(snapshot.usbDrivers);
    sanitizeVector(snapshot.bluetoothDrivers);
}

std::string DriverMonitor::DetectHardwareClass(const DriverDetail& driver) {
    std::string name = driver.name;
    std::string displayName = driver.displayName;
    std::string hardwareClass = driver.hardwareClass;
    
    // Use safe conversion function
    std::transform(name.begin(), name.end(), name.begin(), toLowerChar);
    std::transform(displayName.begin(), displayName.end(), displayName.begin(), toLowerChar);
    std::transform(hardwareClass.begin(), hardwareClass.end(), hardwareClass.begin(), toLowerChar);
    
    if (hardwareClass.find("display") != std::string::npos || 
        hardwareClass.find("graphics") != std::string::npos ||
        name.find("nvidia") != std::string::npos ||
        name.find("amd") != std::string::npos ||
        name.find("intel") != std::string::npos ||
        name.find("radeon") != std::string::npos ||
        name.find("geforce") != std::string::npos ||
        displayName.find("nvidia") != std::string::npos ||
        displayName.find("amd") != std::string::npos ||
        displayName.find("intel") != std::string::npos) {
        return "Display";
    } else if (hardwareClass.find("audio") != std::string::npos ||
               hardwareClass.find("sound") != std::string::npos ||
               name.find("realtek") != std::string::npos ||
               name.find("audio") != std::string::npos ||
               name.find("sound") != std::string::npos ||
               displayName.find("realtek") != std::string::npos ||
               displayName.find("audio") != std::string::npos) {
        return "Audio";
    } else if (hardwareClass.find("network") != std::string::npos ||
               hardwareClass.find("net") != std::string::npos ||
               name.find("ethernet") != std::string::npos ||
               name.find("wifi") != std::string::npos ||
               name.find("wireless") != std::string::npos ||
               name.find("network") != std::string::npos ||
               displayName.find("ethernet") != std::string::npos ||
               displayName.find("wifi") != std::string::npos) {
        return "Network";
    } else if (hardwareClass.find("keyboard") != std::string::npos ||
               hardwareClass.find("mouse") != std::string::npos ||
               hardwareClass.find("input") != std::string::npos ||
               name.find("keyboard") != std::string::npos ||
               name.find("mouse") != std::string::npos ||
               name.find("hid") != std::string::npos ||
               displayName.find("keyboard") != std::string::npos ||
               displayName.find("mouse") != std::string::npos) {
        return "Input";
    } else if (hardwareClass.find("storage") != std::string::npos ||
               hardwareClass.find("disk") != std::string::npos ||
               name.find("storage") != std::string::npos ||
               name.find("disk") != std::string::npos ||
               name.find("raid") != std::string::npos ||
               name.find("sata") != std::string::npos ||
               displayName.find("storage") != std::string::npos ||
               displayName.find("disk") != std::string::npos) {
        return "Storage";
    } else if (hardwareClass.find("print") != std::string::npos ||
               name.find("print") != std::string::npos ||
               displayName.find("print") != std::string::npos) {
        return "Printer";
    } else if (hardwareClass.find("usb") != std::string::npos ||
               name.find("usb") != std::string::npos ||
               displayName.find("usb") != std::string::npos) {
        return "USB";
    } else if (hardwareClass.find("bluetooth") != std::string::npos ||
               name.find("bluetooth") != std::string::npos ||
               displayName.find("bluetooth") != std::string::npos) {
        return "Bluetooth";
    }
    
    return hardwareClass.empty() ? "Other" : hardwareClass;
}

DriverDetail DriverMonitor::GetDriverDetail(const std::string& driverName) {
    auto snapshot = GetDriverSnapshot();
    std::string safeName = util::EncodingUtil::SafeString(driverName);
    
    // Search for driver in all categories
    std::vector<std::vector<DriverDetail>> allCategories = {
        snapshot.kernelDrivers,
        snapshot.fileSystemDrivers,
        snapshot.hardwareDrivers,
        snapshot.runningDrivers,
        snapshot.stoppedDrivers,
        snapshot.autoStartDrivers,
        snapshot.thirdPartyDrivers
    };
    
    for (const auto& category : allCategories) {
        auto it = std::find_if(category.begin(), category.end(),
            [&safeName](const DriverDetail& driver) {
                return util::EncodingUtil::SafeString(driver.name) == safeName;
            });
        
        if (it != category.end()) {
            return *it;
        }
    }
    
    throw std::runtime_error("Driver not found: " + driverName);
}

bool DriverMonitor::IsDriverRunning(const std::string& driverName) {
    try {
        auto driver = GetDriverDetail(driverName);
        return driver.state == "Running";
    } catch (const std::exception&) {
        return false;
    }
}