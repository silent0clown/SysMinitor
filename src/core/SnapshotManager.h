#pragma once
#include "SystemSnapshot.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace snapshot {

class SnapshotManager {
public:
    SnapshotManager(const std::string& storage_path = "snapshots");
    ~SnapshotManager() = default;
    
    // ���չ���
    bool save_snapshot(const std::shared_ptr<SystemSnapshot>& snapshot);
    std::shared_ptr<SystemSnapshot> load_snapshot(const std::string& snapshot_id);
    bool delete_snapshot(const std::string& snapshot_id);
    
    // �����б�
    std::vector<std::string> list_snapshots() const;
    std::unordered_map<std::string, std::string> get_snapshot_metadata() const;
    
    // �洢·������
    void set_storage_path(const std::string& path) { storage_path_ = path; }
    const std::string& get_storage_path() const { return storage_path_; }

private:
    std::string storage_path_;
    
    std::string get_snapshot_path(const std::string& snapshot_id) const;
    bool ensure_storage_directory() const;
};

} // namespace snapshot