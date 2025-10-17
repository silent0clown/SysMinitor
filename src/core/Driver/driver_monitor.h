#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <string>
#include <vector>
#include <chrono>
#include <windows.h>
#include "../../utils/encode.h"

struct DriverVersion {
    std::string fileVersion;        // File version
    std::string productVersion;     // Product version
    std::string companyName;        // Company name
    std::string fileDescription;    // File description
    std::string legalCopyright;     // Copyright information
    std::string originalFilename;   // Original filename
};

struct DriverDetail {
    std::string name;                   // Driver name
    std::string displayName;            // Display name
    std::string description;            // Description
    std::string state;                  // State
    std::string startType;              // Start type
    std::string binaryPath;             // Driver file path
    std::string serviceType;            // Service type
    std::string errorControl;           // Error control
    std::string account;                // Run account
    std::string group;                  // Load group
    std::string tagId;                  // Tag ID
    std::string driverType;             // Driver type
    std::string hardwareClass;          // Hardware class
    DWORD pid;                          // Process ID
    std::string exitCode;               // Exit code
    std::string win32ExitCode;          // Win32 exit code
    std::string serviceSpecificExitCode;// Service specific exit code
    DriverVersion version;              // Version information
    std::chrono::system_clock::time_point installTime;
    
    DriverDetail() : pid(0) {}
    
    DriverDetail& convertToUTF8() {
        name = util::EncodingUtil::SafeString(name);
        displayName = util::EncodingUtil::SafeString(displayName);
        description = util::EncodingUtil::SafeString(description);
        state = util::EncodingUtil::SafeString(state);
        startType = util::EncodingUtil::SafeString(startType);
        binaryPath = util::EncodingUtil::SafeString(binaryPath);
        serviceType = util::EncodingUtil::SafeString(serviceType);
        errorControl = util::EncodingUtil::SafeString(errorControl);
        account = util::EncodingUtil::SafeString(account);
        group = util::EncodingUtil::SafeString(group);
        tagId = util::EncodingUtil::SafeString(tagId);
        driverType = util::EncodingUtil::SafeString(driverType);
        hardwareClass = util::EncodingUtil::SafeString(hardwareClass);
        exitCode = util::EncodingUtil::SafeString(exitCode);
        win32ExitCode = util::EncodingUtil::SafeString(win32ExitCode);
        serviceSpecificExitCode = util::EncodingUtil::SafeString(serviceSpecificExitCode);
        
        // Clean version information
        version.fileVersion = util::EncodingUtil::SafeString(version.fileVersion);
        version.productVersion = util::EncodingUtil::SafeString(version.productVersion);
        version.companyName = util::EncodingUtil::SafeString(version.companyName);
        version.fileDescription = util::EncodingUtil::SafeString(version.fileDescription);
        version.legalCopyright = util::EncodingUtil::SafeString(version.legalCopyright);
        version.originalFilename = util::EncodingUtil::SafeString(version.originalFilename);
        
        return *this;
    }
    
    bool operator==(const DriverDetail& other) const {
        return name == other.name;
    }
};

struct DriverSnapshot {
    uint64_t timestamp;
    std::vector<DriverDetail> kernelDrivers;        // Kernel drivers
    std::vector<DriverDetail> fileSystemDrivers;    // File system drivers
    std::vector<DriverDetail> hardwareDrivers;      // Hardware drivers
    std::vector<DriverDetail> runningDrivers;       // Running drivers
    std::vector<DriverDetail> stoppedDrivers;       // Stopped drivers
    std::vector<DriverDetail> autoStartDrivers;     // Auto-start drivers
    std::vector<DriverDetail> thirdPartyDrivers;    // Third-party drivers
    
    // Classified by hardware category
    std::vector<DriverDetail> displayDrivers;       // Display adapter drivers
    std::vector<DriverDetail> audioDrivers;         // Audio drivers
    std::vector<DriverDetail> networkDrivers;       // Network adapter drivers
    std::vector<DriverDetail> inputDrivers;         // Input device drivers
    std::vector<DriverDetail> storageDrivers;       // Storage controller drivers
    std::vector<DriverDetail> printerDrivers;       // Printer drivers
    std::vector<DriverDetail> usbDrivers;           // USB drivers
    std::vector<DriverDetail> bluetoothDrivers;     // Bluetooth drivers
    
    struct {
        size_t totalDrivers;
        size_t runningCount;
        size_t stoppedCount;
        size_t kernelCount;
        size_t fileSystemCount;
        size_t hardwareCount;
        size_t autoStartCount;
        size_t thirdPartyCount;
        // Hardware driver statistics
        size_t displayCount;
        size_t audioCount;
        size_t networkCount;
        size_t inputCount;
        size_t storageCount;
        size_t printerCount;
        size_t usbCount;
        size_t bluetoothCount;
    } stats;
    
    DriverSnapshot() : timestamp(0) {
        stats.totalDrivers = 0;
        stats.runningCount = 0;
        stats.stoppedCount = 0;
        stats.kernelCount = 0;
        stats.fileSystemCount = 0;
        stats.hardwareCount = 0;
        stats.autoStartCount = 0;
        stats.thirdPartyCount = 0;
        stats.displayCount = 0;
        stats.audioCount = 0;
        stats.networkCount = 0;
        stats.inputCount = 0;
        stats.storageCount = 0;
        stats.printerCount = 0;
        stats.usbCount = 0;
        stats.bluetoothCount = 0;
    }
};

class DriverMonitor {
public:
    DriverMonitor();
    ~DriverMonitor();
    
    DriverSnapshot GetDriverSnapshot();
    DriverDetail GetDriverDetail(const std::string& driverName);
    bool IsDriverRunning(const std::string& driverName);

private:
    std::vector<DriverDetail> GetAllDriversViaSCM();
    std::vector<DriverDetail> GetHardwareDriversViaSetupAPI();
    std::vector<DriverDetail> GetDriversViaWMI();
    
    // Version information retrieval
    DriverVersion GetDriverVersionInfo(const std::string& filePath);
    std::string GetFileVersion(const std::string& filePath);
    
    // Hardware driver classification
    void CategorizeHardwareDrivers(DriverSnapshot& snapshot, const std::vector<DriverDetail>& hardwareDrivers);
    void CategorizeDrivers(DriverSnapshot& snapshot, const std::vector<DriverDetail>& allDrivers);
    void CalculateStatistics(DriverSnapshot& snapshot);
    void SanitizeAllDrivers(DriverSnapshot& snapshot);
    
    // Utility functions
    bool IsHardwareDriver(const DriverDetail& driver);
    std::string DetectHardwareClass(const DriverDetail& driver);
};