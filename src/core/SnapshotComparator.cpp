#include "SnapshotComparator.h"
#include "../utils/AsyncLogger.h"
#include <algorithm>
#include <sstream>

namespace snapshot {

bool ComparisonResult::has_changes() const {
    return !registry_diff.added.empty() || !registry_diff.removed.empty() || !registry_diff.modified.empty() ||
           !disk_diff.changed.empty() ||
           !process_diff.added.empty() || !process_diff.removed.empty() || !process_diff.modified.empty();
}

std::string ComparisonResult::to_string() const {
    std::ostringstream oss;
    oss << "Comparison Result:\n";
    
    oss << "Registry Changes:\n";
    oss << "  Added: " << registry_diff.added.size() << "\n";
    oss << "  Removed: " << registry_diff.removed.size() << "\n";
    oss << "  Modified: " << registry_diff.modified.size() << "\n";
    
    oss << "Disk Changes: " << disk_diff.changed.size() << "\n";
    
    oss << "Process Changes:\n";
    oss << "  Added: " << process_diff.added.size() << "\n";
    oss << "  Removed: " << process_diff.removed.size() << "\n";
    oss << "  Modified: " << process_diff.modified.size() << "\n";
    
    return oss.str();
}

SnapshotComparator::SnapshotComparator(double cpu_threshold, double memory_threshold)
    : cpu_threshold_(cpu_threshold), memory_threshold_(memory_threshold) {}

ComparisonResult SnapshotComparator::compare(
    const std::shared_ptr<SystemSnapshot>& baseline,
    const std::shared_ptr<SystemSnapshot>& current) {
    
    // auto logger = AsyncLogger::get_instance();
    LOG_INFO("开始比较快照: {} vs {}", baseline->get_name(), current->get_name());
    
    ComparisonResult result;
    
    if (!baseline || !current) {
        LOG_ERROR("比较失败: 快照为空");
        return result;
    }
    
    try {
        result.registry_diff = compare_registry(baseline->get_registry_data(), 
                                               current->get_registry_data());
        result.disk_diff = compare_disks(baseline->get_disk_info(), 
                                        current->get_disk_info());
        result.process_diff = compare_processes(baseline->get_process_info(), 
                                               current->get_process_info());
        
        LOG_INFO("快照比较完成，发现 {} 处变化", 
                    result.registry_diff.added.size() + result.registry_diff.removed.size() + result.registry_diff.modified.size() +
                    result.disk_diff.changed.size() +
                    result.process_diff.added.size() + result.process_diff.removed.size() + result.process_diff.modified.size());
    } catch (const std::exception& e) {
        LOG_ERROR("比较快照时发生错误: {}", e.what());
    }
    
    return result;
}

ComparisonResult::RegistryDiff SnapshotComparator::compare_registry(
    const std::vector<RegistryValue>& baseline,
    const std::vector<RegistryValue>& current) {
    
    ComparisonResult::RegistryDiff diff;
    
    // 查找新增的注册表项
    for (const auto& current_val : current) {
        auto it = std::find(baseline.begin(), baseline.end(), current_val);
        if (it == baseline.end()) {
            diff.added.push_back(current_val);
        }
    }
    
    // 查找删除的注册表项
    for (const auto& baseline_val : baseline) {
        auto it = std::find(current.begin(), current.end(), baseline_val);
        if (it == current.end()) {
            diff.removed.push_back(baseline_val);
        }
    }
    
    return diff;
}

ComparisonResult::DiskDiff SnapshotComparator::compare_disks(
    const std::vector<DiskInfo>& baseline,
    const std::vector<DiskInfo>& current) {
    
    ComparisonResult::DiskDiff diff;
    
    for (const auto& current_disk : current) {
        for (const auto& baseline_disk : baseline) {
            if (current_disk.drive_letter == baseline_disk.drive_letter) {
                if (!(current_disk == baseline_disk)) {
                    diff.changed.push_back(current_disk);
                }
                break;
            }
        }
    }
    
    return diff;
}

ComparisonResult::ProcessDiff SnapshotComparator::compare_processes(
    const std::vector<ProcessInfo>& baseline,
    const std::vector<ProcessInfo>& current) {
    
    ComparisonResult::ProcessDiff diff;
    
    // 查找新增的进程
    for (const auto& current_proc : current) {
        auto it = std::find_if(baseline.begin(), baseline.end(),
            [&](const ProcessInfo& proc) { return proc.pid == current_proc.pid; });
        
        if (it == baseline.end()) {
            diff.added.push_back(current_proc);
        }
    }
    
    // 查找结束的进程
    for (const auto& baseline_proc : baseline) {
        auto it = std::find_if(current.begin(), current.end(),
            [&](const ProcessInfo& proc) { return proc.pid == baseline_proc.pid; });
        
        if (it == current.end()) {
            diff.removed.push_back(baseline_proc);
        } else {
            // 检查资源使用变化
            const auto& current_proc = *it;
            double cpu_change = std::abs(current_proc.cpu_usage - baseline_proc.cpu_usage);
            double memory_change = std::abs(static_cast<double>(current_proc.memory_usage - baseline_proc.memory_usage) / 
                                          baseline_proc.memory_usage * 100.0);
            
            if (cpu_change > cpu_threshold_ || memory_change > memory_threshold_) {
                diff.modified.push_back({baseline_proc, current_proc});
            }
        }
    }
    
    return diff;
}

} // namespace snapshot