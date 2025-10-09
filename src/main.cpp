#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <signal.h>
#include "core/SystemSnapshot.h"
#include "core/DataCollector.h"
#include "core/SnapshotManager.h"
#include "core/SnapshotComparator.h"
#include "server/WebServer.h"
#include "utils/AsyncLogger.h"

using namespace snapshot;

std::atomic<bool> running{true};

void signal_handler(int signal) {
    // auto logger = AsyncLogger::get_instance();
    LOG_INFO("接收到终止信号，正在关闭...");
    running = false;
}

void print_usage() {
    std::cout << "System Snapshot Tool Usage:\n";
    std::cout << "  create <name>    创建新的系统快照\n";
    std::cout << "  list            列出所有快照\n";
    std::cout << "  view <id>       查看特定快照\n";
    std::cout << "  compare <id1> <id2>  比较两个快照\n";
    std::cout << "  server [port]   启动HTTP服务器（默认端口8080）\n";
    std::cout << "  help            显示此帮助信息\n";
}

int main(int argc, char* argv[]) {
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // auto logger = AsyncLogger::get_instance();
    LOG_INFO("系统快照工具启动");
    
    // 初始化核心组件
    auto data_collector = std::make_shared<DataCollector>();
    auto snapshot_manager = std::make_shared<SnapshotManager>();
    auto comparator = std::make_shared<SnapshotComparator>();
    auto web_server = std::make_shared<WebServer>(snapshot_manager, comparator);
    
    // 命令行参数处理
    if (argc < 2) {
        print_usage();
        return 1;
    }
    
    std::string command = argv[1];
    
    try {
        if (command == "create") {
            std::string name = (argc > 2) ? argv[2] : "manual_snapshot";
            auto snapshot = data_collector->collect_snapshot(name);
            if (snapshot_manager->save_snapshot(snapshot)) {
                std::cout << "快照创建成功: " << snapshot->get_id() << std::endl;
            } else {
                std::cerr << "快照保存失败" << std::endl;
                return 1;
            }
            
        } else if (command == "list") {
            auto snapshots = snapshot_manager->list_snapshots();
            std::cout << "可用快照 (" << snapshots.size() << "):\n";
            for (const auto& id : snapshots) {
                std::cout << "  " << id << std::endl;
            }
            
        } else if (command == "view" && argc > 2) {
            std::string snapshot_id = argv[2];
            auto snapshot = snapshot_manager->load_snapshot(snapshot_id);
            if (snapshot) {
                std::cout << snapshot->to_json() << std::endl;
            } else {
                std::cerr << "快照未找到: " << snapshot_id << std::endl;
                return 1;
            }
            
        } else if (command == "compare" && argc > 3) {
            std::string id1 = argv[2];
            std::string id2 = argv[3];
            
            auto snapshot1 = snapshot_manager->load_snapshot(id1);
            auto snapshot2 = snapshot_manager->load_snapshot(id2);
            
            if (snapshot1 && snapshot2) {
                auto result = comparator->compare(snapshot1, snapshot2);
                std::cout << result.to_string() << std::endl;
            } else {
                std::cerr << "一个或多个快照未找到" << std::endl;
                return 1;
            }
            
        } else if (command == "server") {
            int port = (argc > 2) ? std::stoi(argv[2]) : 8080;
            
            if (web_server->start(port)) {
                std::cout << "HTTP服务器运行在 http://localhost:" << port << std::endl;
                std::cout << "按 Ctrl+C 停止服务器" << std::endl;
                
                // 创建初始快照用于Web显示
                auto current_snapshot = data_collector->collect_snapshot("web_current");
                web_server->set_current_snapshot(current_snapshot);
                
                // 主循环
                while (running) {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    
                    // 定期更新当前状态（可选）
                    static int counter = 0;
                    if (++counter % 30 == 0) { // 每30秒更新一次
                        auto new_snapshot = data_collector->collect_snapshot("web_current");
                        web_server->set_current_snapshot(new_snapshot);
                        counter = 0;
                    }
                }
                
                web_server->stop();
            } else {
                std::cerr << "无法启动HTTP服务器" << std::endl;
                return 1;
            }
            
        } else if (command == "help") {
            print_usage();
            
        } else {
            std::cerr << "未知命令: " << command << std::endl;
            print_usage();
            return 1;
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("程序执行错误: {}", e.what());
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
    
    LOG_INFO("系统快照工具正常退出");
    return 0;
}