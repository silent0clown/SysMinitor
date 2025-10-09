#pragma once
#include "SystemSnapshot.h"
#include <memory>

namespace snapshot {

struct ComparisonResult {
    struct RegistryDiff {
        std::vector<RegistryValue> added;
        std::vector<RegistryValue> removed;
        std::vector<std::pair<RegistryValue, RegistryValue>> modified;
    };
    
    struct DiskDiff {
        std::vector<DiskInfo> changed;
    };
    
    struct ProcessDiff {
        std::vector<ProcessInfo> added;
        std::vector<ProcessInfo> removed;
        std::vector<std::pair<ProcessInfo, ProcessInfo>> modified;
    };
    
    RegistryDiff registry_diff;
    DiskDiff disk_diff;
    ProcessDiff process_diff;
    
    bool has_changes() const;
    std::string to_string() const;
};

class SnapshotComparator {
public:
    SnapshotComparator(double cpu_threshold = 10.0, double memory_threshold = 20.0);
    
    ComparisonResult compare(const std::shared_ptr<SystemSnapshot>& baseline,
                           const std::shared_ptr<SystemSnapshot>& current);

private:
    double cpu_threshold_;
    double memory_threshold_;
    
    ComparisonResult::RegistryDiff compare_registry(
        const std::vector<RegistryValue>& baseline,
        const std::vector<RegistryValue>& current);
        
    ComparisonResult::DiskDiff compare_disks(
        const std::vector<DiskInfo>& baseline,
        const std::vector<DiskInfo>& current);
        
    ComparisonResult::ProcessDiff compare_processes(
        const std::vector<ProcessInfo>& baseline,
        const std::vector<ProcessInfo>& current);
};

} // namespace snapshot