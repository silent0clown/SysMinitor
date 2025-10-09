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
    auto& server = *server_;
    
    // 仪表板页面
    server.Get("/", [this](const httplib::Request& req, httplib::Response& res) {
        res.set_content(generate_html_dashboard(), "text/html");
    });
    
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
}

std::string WebServer::generate_html_dashboard() {
    std::ostringstream oss;
    
    oss << R"(<!DOCTYPE html>
<html>
<head>
    <title>System Snapshot Tool</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; }
        .card { border: 1px solid #ddd; padding: 20px; margin: 10px 0; border-radius: 5px; }
        .section { margin-bottom: 30px; }
        button { padding: 10px 15px; margin: 5px; background: #007cba; color: white; border: none; border-radius: 3px; cursor: pointer; }
        button:hover { background: #005a87; }
    </style>
</head>
<body>
    <h1>System Snapshot Tool Dashboard</h1>
    
    <div class="section">
        <h2>Quick Actions</h2>
        <div class="card">
            <button onclick="createSnapshot()\">Create New Snapshot</button>
            <button onclick="refreshSnapshots()\">Refresh Snapshots List</button>
        </div>
    </div>
    
    <div class="section">
        <h2>Available Snapshots</h2>
        <div id="snapshots-list" class="card">
            Loading...
        </div>
    </div>
    
    <div class="section">
        <h2>Current System Status</h2>
        <div id="current-status" class="card">
            <button onclick="getCurrentStatus()\">Refresh Current Status</button>
            <pre id="status-content"></pre>
        </div>
    </div>

    <script>
        function createSnapshot() {
            fetch('/api/snapshot/create', { method: 'POST' })
                .then(response => response.json())
                .then(data => {
                    alert('Snapshot created: ' + data.id);
                    refreshSnapshots();
                });
        }
        
        function refreshSnapshots() {
            fetch('/api/snapshots')
                .then(response => response.json())
                .then(data => {
                    const list = document.getElementById('snapshots-list');
                    list.innerHTML = '<ul>' + data.snapshots.map(id => 
                        '<li>' + id + ' <button onclick="viewSnapshot(\\'' + id + '\\')\">View</button></li>'
                    ).join('') + '</ul>';
                });
        }
        
        function getCurrentStatus() {
            fetch('/api/current')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('status-content').textContent = 
                        JSON.stringify(data, null, 2);
                });
        }
        
        function viewSnapshot(id) {
            fetch('/api/snapshot/' + id)
                .then(response => response.json())
                .then(data => {
                    alert('Snapshot ' + id + ':\\n' + JSON.stringify(data, null, 2));
                });
        }
        
        // 初始化加载
        refreshSnapshots();
        getCurrentStatus();
    </script>
</body>
</html>)";
    
    return oss.str();
}

} // namespace snapshot