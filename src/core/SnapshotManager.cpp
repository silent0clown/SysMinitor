#include "SnapshotManager.h"
#include <filesystem>
#include <fstream>
#include "../utils/AsyncLogger.h"
#include "../utils/encode.h"

using namespace std::string_literals;

namespace snapshot {

SnapshotManager::SnapshotManager(const std::string& dir) : dir_(dir) {
    try { std::filesystem::create_directories(dir_); } catch (...) {}
}

std::string SnapshotManager::PathFor(const std::string& id) const {
    return util::EncodingUtil::UTF8ToGB2312(dir_ + "/" + id + ".json");
}

bool SnapshotManager::Save(const std::string& id, const std::string& json) {
    try {
        std::ofstream out(PathFor(id));
        if (!out.is_open()) return false;
        out << json;
        out.close();
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("SnapshotManager::Save error: {}", e.what());
        return false;
    }
}

std::optional<std::string> SnapshotManager::Load(const std::string& id) const {
    try {
        std::ifstream in(PathFor(id));
        if (!in.is_open()) return std::nullopt;
        std::string s((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        in.close();
        return s;
    } catch (const std::exception& e) {
        LOG_ERROR("SnapshotManager::Load error: {}", e.what());
        return std::nullopt;
    }
}

bool SnapshotManager::Delete(const std::string& id) const {
    try { return std::filesystem::remove(PathFor(id)); } catch (...) { return false; }
}

std::vector<std::string> SnapshotManager::List() const {
    std::vector<std::string> out;
    try {
        for (const auto& e : std::filesystem::directory_iterator(dir_)) {
            if (e.path().extension() == ".json") out.push_back(e.path().stem().string());
        }
    } catch (...) {}
    return out;
}

} // namespace snapshot
