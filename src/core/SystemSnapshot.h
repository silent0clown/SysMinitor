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
            jd["model"] = d.model;
            jd["serialNumber"] = d.serialNumber;
            jd["interfaceType"] = d.interfaceType;
            jd["mediaType"] = d.mediaType;
            jd["totalSize"] = d.totalSize;
            jd["bytesPerSector"] = d.bytesPerSector;
            jd["status"] = d.status;
            jd["deviceId"] = d.deviceId;
            out["disk"]["drives"].push_back(jd);
        }

        out["disk"]["partitions"] = json::array();
        for (const auto& p : disk.partitions) {
            json jp;
            jp["driveLetter"] = p.driveLetter;
            jp["label"] = p.label;
            jp["fileSystem"] = p.fileSystem;
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
            jperf["driveLetter"] = perf.driveLetter;
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
            js["deviceId"] = s.deviceId;
            js["temperature"] = s.temperature;
            js["healthStatus"] = s.healthStatus;
            js["powerOnHours"] = s.powerOnHours;
            js["powerOnCount"] = s.powerOnCount;
            js["badSectors"] = s.badSectors;
            js["readErrorCount"] = s.readErrorCount;
            js["writeErrorCount"] = s.writeErrorCount;
            js["overallHealth"] = s.overallHealth;
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
            jd["name"] = det.name;
            jd["displayName"] = det.displayName;
            jd["description"] = det.description;
            jd["state"] = det.state;
            jd["startType"] = det.startType;
            jd["binaryPath"] = det.binaryPath;
            jd["serviceType"] = det.serviceType;
            jd["errorControl"] = det.errorControl;
            jd["account"] = det.account;
            jd["group"] = det.group;
            jd["tagId"] = det.tagId;
            jd["driverType"] = det.driverType;
            jd["hardwareClass"] = det.hardwareClass;
            jd["pid"] = det.pid;
            jd["exitCode"] = det.exitCode;
            jd["win32ExitCode"] = det.win32ExitCode;
            jd["serviceSpecificExitCode"] = det.serviceSpecificExitCode;
            jd["version"]["fileVersion"] = det.version.fileVersion;
            jd["version"]["productVersion"] = det.version.productVersion;
            jd["version"]["companyName"] = det.version.companyName;
            jd["version"]["fileDescription"] = det.version.fileDescription;
            jd["version"]["legalCopyright"] = det.version.legalCopyright;
            jd["version"]["originalFilename"] = det.version.originalFilename;

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
                jd["name"] = det.name;
                jd["displayName"] = det.displayName;
                jd["state"] = det.state;
                jd["binaryPath"] = det.binaryPath;
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
        out["registry"]["systemInfoKeys"] = json::array();
        auto pushRegistryList = [&](const std::vector<RegistryKey>& list, const std::string& keyname) {
            out["registry"][keyname] = json::array();
            for (const auto& k : list) {
                json jk;
                jk["path"] = k.path;
                jk["lastModified"] = k.lastModified;
                jk["category"] = k.category;
                jk["autoStartType"] = k.autoStartType;
                jk["values"] = json::array();
                for (const auto& v : k.values) {
                    json jv;
                    jv["name"] = v.name;
                    jv["type"] = v.type;
                    jv["data"] = v.data;
                    jv["size"] = v.size;
                    jk["values"].push_back(jv);
                }
                jk["subkeys"] = k.subkeys;
                out["registry"][keyname].push_back(jk);
            }
        };
        pushRegistryList(registry.systemInfoKeys, "systemInfoKeys");
        pushRegistryList(registry.softwareKeys, "softwareKeys");
        pushRegistryList(registry.networkKeys, "networkKeys");
        pushRegistryList(registry.autoStartKeys, "autoStartKeys");

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
            jp["name"] = p.name;
            jp["fullPath"] = p.fullPath;
            jp["state"] = p.state;
            jp["username"] = p.username;
            jp["cpuUsage"] = p.cpuUsage;
            jp["memoryUsage"] = p.memoryUsage;
            jp["workingSetSize"] = p.workingSetSize;
            jp["pagefileUsage"] = p.pagefileUsage;
            jp["createTime"] = p.createTime;
            jp["priority"] = p.priority;
            jp["threadCount"] = p.threadCount;
            jp["commandLine"] = p.commandLine;
            jp["handleCount"] = p.handleCount;
            jp["gdiCount"] = p.gdiCount;
            jp["userCount"] = p.userCount;
            out["processes"]["processes"].push_back(jp);
        }

        out["snapshotTimestamp"] = timestamp;
        out["id"] = id;
        return out;
    }
};

} // namespace sysmonitor
