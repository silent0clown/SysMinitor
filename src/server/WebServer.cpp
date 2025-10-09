#include "WebServer.h"
#include "../utils/AsyncLogger.h"
#include <sstream>
#include <iomanip>

namespace snapshot {

WebServer::WebServer(std::shared_ptr<SnapshotManager> snapshot_manager,
                     std::shared_ptr<SnapshotComparator> comparator)
    : snapshot_manager_(snapshot_manager), comparator_(comparator) {
}

WebServer::~WebServer() {
    stop();
}

bool WebServer::start(int port) {
    if (running_) {
        return true;
    }
    
    server_ = std::make_unique<httplib::Server>();
    setup_routes();
    
    server_thread_ = std::thread([this, port]() {
        // auto logger = AsyncLogger::get_instance();
        LOG_INFO("启动HTTP服务器，端口: {}", port);
        
        running_ = true;
        bool success = server_->listen("0.0.0.0", port);
        running_ = false;
        
        if (!success) {
            LOG_ERROR("HTTP服务器启动失败");
        } else {
            LOG_INFO("HTTP服务器已停止");
        }
    });
    
    // 等待服务器启动
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return running_;
}

void WebServer::stop() {
    if (server_) {
        server_->stop();
    }
    
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
    
    running_ = false;
}

void WebServer::setup_routes() {
  auto &server = *server_;
    
    // 仪表板页面
  // server.Get("/", [this](const httplib::Request& req, httplib::Response& res)
  // {
  //     res.set_content(generate_html_dashboard(), "text/html");
  // });
    
    // 获取快照列表
    server.Get("/api/snapshots", [this](const httplib::Request& req, httplib::Response& res) {
        auto snapshots = snapshot_manager_->list_snapshots();
        
        std::ostringstream oss;
        oss << "{\"snapshots\":[";
        bool first = true;
        for (const auto& id : snapshots) {
            if (!first) oss << ",";
            oss << "\"" << id << "\"";
            first = false;
        }
        oss << "]}";
        
        res.set_content(oss.str(), "application/json");
    });
    
    // 获取特定快照
    server.Get(R"(/api/snapshot/([a-f0-9]+))", [this](const httplib::Request& req, httplib::Response& res) {
        auto snapshot_id = req.matches[1];
        auto snapshot = snapshot_manager_->load_snapshot(snapshot_id);
        
        if (snapshot) {
            res.set_content(snapshot->to_json(), "application/json");
        } else {
            res.status = 404;
            res.set_content("{\"error\":\"Snapshot not found\"}", "application/json");
        }
    });
    
    // 比较两个快照
    server.Get(R"(/api/compare/([a-f0-9]+)/([a-f0-9]+))", [this](const httplib::Request& req, httplib::Response& res) {
        auto id1 = req.matches[1];
        auto id2 = req.matches[2];
        
        auto snapshot1 = snapshot_manager_->load_snapshot(id1);
        auto snapshot2 = snapshot_manager_->load_snapshot(id2);
        
        if (!snapshot1 || !snapshot2) {
            res.status = 404;
            res.set_content("{\"error\":\"One or both snapshots not found\"}", "application/json");
            return;
        }
        
        auto result = comparator_->compare(snapshot1, snapshot2);
        res.set_content(result.to_string(), "text/plain");
    });
    
    // 获取当前系统状态
    server.Get("/api/current", [this](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (current_snapshot_) {
            res.set_content(current_snapshot_->to_json(), "application/json");
        } else {
            res.status = 404;
            res.set_content("{\"error\":\"No current snapshot available\"}", "application/json");
        }
    });

  server.set_mount_point("/", "webclient");
}

} // namespace snapshot