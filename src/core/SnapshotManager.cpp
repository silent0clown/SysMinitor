#include "SnapshotManager.h"
#include "../utils/AsyncLogger.h"
#include <fstream>
#include <filesystem>

namespace snapshot {

SnapshotManager::SnapshotManager(const std::string& storage_path) 
    : storage_path_(storage_path) {
    ensure_storage_directory();
}

bool SnapshotManager::save_snapshot(const std::shared_ptr<SystemSnapshot>& snapshot) {
    // auto logger = AsyncLogger::get_instance();
    
    if (!snapshot) {
        LOG_ERROR("尝试保存空的快照");
        return false;
    }
    
    if (!ensure_storage_directory()) {
        LOG_ERROR("无法创建存储目录: {}", storage_path_);
        return false;
    }
    
    std::string file_path = get_snapshot_path(snapshot->get_id());
    
    try {
        std::ofstream file(file_path);
        if (!file.is_open()) {
            LOG_ERROR("无法打开文件进行写入: {}", file_path);
            return false;
        }
        
        std::string json_data = snapshot->to_json();
        file << json_data;
        file.close();
        
        LOG_INFO("快照保存成功: {} -> {}", snapshot->get_name(), file_path);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("保存快照时发生错误: {}", e.what());
        return false;
    }
}

std::shared_ptr<SystemSnapshot> SnapshotManager::load_snapshot(const std::string& snapshot_id) {
    // auto logger = AsyncLogger::get_instance();
    std::string file_path = get_snapshot_path(snapshot_id);
    
    if (!std::filesystem::exists(file_path)) {
        LOG_ERROR("快照文件不存在: {}", file_path);
        return nullptr;
    }
    
    try {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            LOG_ERROR("无法打开快照文件: {}", file_path);
            return nullptr;
        }
        
        std::string json_data((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
        file.close();
        
        auto snapshot = SystemSnapshot::from_json(json_data);
        if (snapshot) {
            LOG_INFO("快照加载成功: {}", snapshot_id);
        }
        
        return snapshot;
    } catch (const std::exception& e) {
        LOG_ERROR("加载快照时发生错误: {}", e.what());
        return nullptr;
    }
}

bool SnapshotManager::delete_snapshot(const std::string& snapshot_id) {
    // auto logger = AsyncLogger::get_instance();
    std::string file_path = get_snapshot_path(snapshot_id);
    
    try {
        if (std::filesystem::remove(file_path)) {
            LOG_INFO("快照删除成功: {}", snapshot_id);
            return true;
        } else {
            LOG_ERROR("快照文件不存在，无法删除: {}", snapshot_id);
            return false;
        }
    } catch (const std::filesystem::filesystem_error& e) {
        LOG_ERROR("删除快照时发生错误: {}", e.what());
        return false;
    }
}

std::vector<std::string> SnapshotManager::list_snapshots() const {
    std::vector<std::string> snapshots;
    
    if (!std::filesystem::exists(storage_path_)) {
        return snapshots;
    }
    
    for (const auto& entry : std::filesystem::directory_iterator(storage_path_)) {
        if (entry.path().extension() == ".json") {
            snapshots.push_back(entry.path().stem().string());
        }
    }
    
    return snapshots;
}

std::string SnapshotManager::get_snapshot_path(const std::string& snapshot_id) const {
    return storage_path_ + "/" + snapshot_id + ".json";
}

bool SnapshotManager::ensure_storage_directory() const {
    try {
        if (!std::filesystem::exists(storage_path_)) {
            return std::filesystem::create_directories(storage_path_);
        }
        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        LOG_ERROR("创建存储目录失败: {}", e.what());
        return false;
    }
}

} // namespace snapshot