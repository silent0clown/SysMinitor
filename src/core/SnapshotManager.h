#pragma once
#include <string>
#include <vector>
#include <optional>

namespace snapshot {

class SnapshotManager {
public:
    explicit SnapshotManager(const std::string& dir = "snapshots");
    bool Save(const std::string& id, const std::string& json);
    std::optional<std::string> Load(const std::string& id) const;
    bool Delete(const std::string& id) const;
    std::vector<std::string> List() const;

private:
    std::string dir_;
    std::string PathFor(const std::string& id) const;
};

} // namespace snapshot
