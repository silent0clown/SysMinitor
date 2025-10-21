#pragma once
#include "CPUInfo/system_info.h"
#include "Memory/memory_monitor.h"
#include "Disk/disk_monitor.h"
#include "Driver/driver_monitor.h"
#include "Register/registry_monitor.h"
#include "Process/process_monitor.h"
#include "../third_party/nlohmann/json.hpp"
#include <string>
#include <chrono>
#include <ctime>

namespace sysmonitor {

using json = nlohmann::json;

struct SystemSnapshot {
    CPUUsage cpu{};
    MemoryUsage memory{};
    DiskSnapshot disk{};
    DriverSnapshot driver{};
    RegistrySnapshot registry{};
    ProcessSnapshot processes{};
    uint64_t timestamp = 0;
    std::string id; // optional name/id

    json ToJson() const {
        json out;

        // cpu
        out["cpu"]["totalUsage"] = cpu.totalUsage;
        out["cpu"]["coreUsages"] = cpu.coreUsages;
        out["cpu"]["timestamp"] = cpu.timestamp;

        // memory
        out["memory"]["totalPhysical"] = memory.totalPhysical;
        out["memory"]["availablePhysical"] = memory.availablePhysical;
        out["memory"]["usedPhysical"] = memory.usedPhysical;
        out["memory"]["usedPercent"] = memory.usedPercent;
        out["memory"]["timestamp"] = memory.timestamp;

        // disk
        out["disk"]["timestamp"] = disk.timestamp;
        out["disk"]["drives"] = json::array();
        for (const auto& d : disk.drives) {
            json jd;
            jd["model"] = util::EncodingUtil::ToUTF8(d.model);
            jd["serialNumber"] = util::EncodingUtil::ToUTF8(d.serialNumber);
            jd["interfaceType"] = util::EncodingUtil::ToUTF8(d.interfaceType);
            jd["mediaType"] = util::EncodingUtil::ToUTF8(d.mediaType);
            jd["totalSize"] = d.totalSize;
            jd["bytesPerSector"] = d.bytesPerSector;
            jd["status"] = util::EncodingUtil::ToUTF8(d.status);
            jd["deviceId"] = util::EncodingUtil::ToUTF8(d.deviceId);
            out["disk"]["drives"].push_back(jd);
        }

        out["disk"]["partitions"] = json::array();
        for (const auto& p : disk.partitions) {
            json jp;
            jp["driveLetter"] = util::EncodingUtil::ToUTF8(p.driveLetter);
            jp["label"] = util::EncodingUtil::ToUTF8(p.label);
            jp["fileSystem"] = util::EncodingUtil::ToUTF8(p.fileSystem);
            jp["totalSize"] = p.totalSize;
            jp["freeSpace"] = p.freeSpace;
            jp["usedSpace"] = p.usedSpace;
            jp["usagePercentage"] = p.usagePercentage;
            jp["serialNumber"] = p.serialNumber;
            out["disk"]["partitions"].push_back(jp);
        }

        out["disk"]["performance"] = json::array();
        for (const auto& perf : disk.performance) {
            json jperf;
            jperf["driveLetter"] = util::EncodingUtil::ToUTF8(perf.driveLetter);
            jperf["readSpeed"] = perf.readSpeed;
            jperf["writeSpeed"] = perf.writeSpeed;
            jperf["readBytesPerSec"] = perf.readBytesPerSec;
            jperf["writeBytesPerSec"] = perf.writeBytesPerSec;
            jperf["readCountPerSec"] = perf.readCountPerSec;
            jperf["writeCountPerSec"] = perf.writeCountPerSec;
            jperf["queueLength"] = perf.queueLength;
            jperf["usagePercentage"] = perf.usagePercentage;
            jperf["responseTime"] = perf.responseTime;
            out["disk"]["performance"].push_back(jperf);
        }

        out["disk"]["smart"] = json::array();
        for (const auto& s : disk.smartData) {
            json js;
            js["deviceId"] = util::EncodingUtil::ToUTF8(s.deviceId);
            js["temperature"] = s.temperature;
            js["healthStatus"] = s.healthStatus;
            js["powerOnHours"] = s.powerOnHours;
            js["powerOnCount"] = s.powerOnCount;
            js["badSectors"] = s.badSectors;
            js["readErrorCount"] = s.readErrorCount;
            js["writeErrorCount"] = s.writeErrorCount;
            js["overallHealth"] = util::EncodingUtil::ToUTF8(s.overallHealth);
            out["disk"]["smart"].push_back(js);
        }

        // driver
        out["driver"]["timestamp"] = driver.timestamp;
        out["driver"]["stats"] = json::object();
        out["driver"]["stats"]["totalDrivers"] = driver.stats.totalDrivers;
        out["driver"]["stats"]["runningCount"] = driver.stats.runningCount;
        out["driver"]["stats"]["stoppedCount"] = driver.stats.stoppedCount;
        out["driver"]["runningDrivers"] = json::array();
        for (const auto& det : driver.runningDrivers) {
            json jd;
            jd["name"] = util::EncodingUtil::ToUTF8(det.name);
            jd["displayName"] = util::EncodingUtil::ToUTF8(det.displayName);
            jd["description"] = util::EncodingUtil::ToUTF8(det.description);
            jd["state"] = util::EncodingUtil::ToUTF8(det.state);
            jd["startType"] = util::EncodingUtil::ToUTF8(det.startType);
            jd["binaryPath"] = util::EncodingUtil::ToUTF8(det.binaryPath);
            jd["serviceType"] = util::EncodingUtil::ToUTF8(det.serviceType);
            jd["errorControl"] = util::EncodingUtil::ToUTF8(det.errorControl);
            jd["account"] = util::EncodingUtil::ToUTF8(det.account);
            jd["group"] = util::EncodingUtil::ToUTF8(det.group);
            jd["tagId"] = det.tagId;
            jd["driverType"] = util::EncodingUtil::ToUTF8(det.driverType);
            jd["hardwareClass"] = util::EncodingUtil::ToUTF8(det.hardwareClass);
            jd["pid"] = det.pid;
            jd["exitCode"] = det.exitCode;
            jd["win32ExitCode"] = det.win32ExitCode;
            jd["serviceSpecificExitCode"] = det.serviceSpecificExitCode;
            jd["version"]["fileVersion"] = util::EncodingUtil::ToUTF8(det.version.fileVersion);
            jd["version"]["productVersion"] = util::EncodingUtil::ToUTF8(det.version.productVersion);
            jd["version"]["companyName"] = util::EncodingUtil::ToUTF8(det.version.companyName);
            jd["version"]["fileDescription"] = util::EncodingUtil::ToUTF8(det.version.fileDescription);
            jd["version"]["legalCopyright"] = util::EncodingUtil::ToUTF8(det.version.legalCopyright);
            jd["version"]["originalFilename"] = util::EncodingUtil::ToUTF8(det.version.originalFilename);

            try {
                auto t = std::chrono::system_clock::to_time_t(det.installTime);
                jd["installTime"] = static_cast<uint64_t>(t);
            } catch (...) {
                jd["installTime"] = 0;
            }
            out["driver"]["runningDrivers"].push_back(jd);
        }

        auto pushDriverList = [&](const std::vector<DriverDetail>& list, const std::string& key) {
            out["driver"][key] = json::array();
            for (const auto& det : list) {
                json jd;
                jd["name"] = util::EncodingUtil::ToUTF8(det.name);
                jd["displayName"] = util::EncodingUtil::ToUTF8(det.displayName);
                jd["state"] = util::EncodingUtil::ToUTF8(det.state);
                jd["binaryPath"] = util::EncodingUtil::ToUTF8(det.binaryPath);
                out["driver"][key].push_back(jd);
            }
        };
        pushDriverList(driver.kernelDrivers, "kernelDrivers");
        pushDriverList(driver.fileSystemDrivers, "fileSystemDrivers");
        pushDriverList(driver.hardwareDrivers, "hardwareDrivers");
        pushDriverList(driver.stoppedDrivers, "stoppedDrivers");
        pushDriverList(driver.autoStartDrivers, "autoStartDrivers");
        pushDriverList(driver.thirdPartyDrivers, "thirdPartyDrivers");
        pushDriverList(driver.displayDrivers, "displayDrivers");
        pushDriverList(driver.audioDrivers, "audioDrivers");
        pushDriverList(driver.networkDrivers, "networkDrivers");
        pushDriverList(driver.inputDrivers, "inputDrivers");
        pushDriverList(driver.storageDrivers, "storageDrivers");
        pushDriverList(driver.printerDrivers, "printerDrivers");
        pushDriverList(driver.usbDrivers, "usbDrivers");
        pushDriverList(driver.bluetoothDrivers, "bluetoothDrivers");

        // registry
        out["registry"]["timestamp"] = registry.timestamp;
        out["registry"]["backupInfo"]["folderName"] = util::EncodingUtil::ToUTF8(registry.backupInfo.folderName);
        out["registry"]["backupInfo"]["folderPath"] = util::EncodingUtil::ToUTF8(registry.backupInfo.folderPath);
        out["registry"]["backupInfo"]["createTime"] = registry.backupInfo.createTime;
        out["registry"]["backupInfo"]["totalSize"] = registry.backupInfo.totalSize;

        // processes
        out["processes"]["timestamp"] = processes.timestamp;
        out["processes"]["totalProcesses"] = processes.totalProcesses;
        out["processes"]["totalThreads"] = processes.totalThreads;
        out["processes"]["totalHandles"] = processes.totalHandles;
        out["processes"]["totalGdiObjects"] = processes.totalGdiObjects;
        out["processes"]["totalUserObjects"] = processes.totalUserObjects;
        out["processes"]["processes"] = json::array();
        for (const auto& p : processes.processes) {
            json jp;
            jp["pid"] = p.pid;
            jp["parentPid"] = p.parentPid;
            jp["name"] = util::EncodingUtil::ToUTF8(p.name);
            jp["fullPath"] = util::EncodingUtil::ToUTF8(p.fullPath);
            jp["state"] = util::EncodingUtil::ToUTF8(p.state);
            jp["username"] = util::EncodingUtil::ToUTF8(p.username);
            jp["cpuUsage"] = p.cpuUsage;
            jp["memoryUsage"] = p.memoryUsage;
            jp["workingSetSize"] = p.workingSetSize;
            jp["pagefileUsage"] = p.pagefileUsage;
            jp["createTime"] = p.createTime;
            jp["priority"] = p.priority;
            jp["threadCount"] = p.threadCount;
            jp["commandLine"] = util::EncodingUtil::ToUTF8(p.commandLine);
            jp["handleCount"] = p.handleCount;
            jp["gdiCount"] = p.gdiCount;
            jp["userCount"] = p.userCount;
            out["processes"]["processes"].push_back(jp);
        }

        out["snapshotTimestamp"] = timestamp;
        out["id"] = util::EncodingUtil::ToUTF8(id);
        return out;
    }
};

} // namespace sysmonitor