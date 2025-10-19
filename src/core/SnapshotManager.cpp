#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "SnapshotManager.h"
#include <filesystem>
#include <fstream>
#include <map>
#include <algorithm>
#include "../utils/AsyncLogger.h"
#include "../utils/encode.h"
#include "../third_party/nlohmann/json.hpp"
#include <Windows.h>

using namespace std::string_literals;
using json = nlohmann::json;

namespace snapshot {

// 获取 exe 文件所在目录
static std::string GetExeDirectory() {
#ifdef _WIN32
    char path[MAX_PATH];
    DWORD length = GetModuleFileNameA(NULL, path, MAX_PATH);
    if (length > 0) {
        std::string exePath(path, length);
        size_t pos = exePath.find_last_of("\\/");
        if (pos != std::string::npos) {
            return exePath.substr(0, pos);
        }
    }
#endif
    // 如果获取失败，返回当前目录
    return ".";
}

SnapshotManager::SnapshotManager(const std::string& dir) {
    // 如果传入的是相对路径，则与 exe 目录组合
    if (dir.empty() || dir == "snapshots") {
        dir_ = GetExeDirectory() + "/snapshots";
    } else if (std::filesystem::path(dir).is_relative()) {
        dir_ = GetExeDirectory() + "/" + dir;
    } else {
        // 绝对路径直接使用
        dir_ = dir;
    }
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
