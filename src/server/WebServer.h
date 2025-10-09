#pragma once
#include "../core/SystemSnapshot.h"
#include "../core/SnapshotManager.h"
#include "../core/SnapshotComparator.h"
#include "../third_party/httplib.h"
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>

namespace snapshot {

class WebServer {
public:
  WebServer(std::shared_ptr<SnapshotManager> snapshot_manager,
            std::shared_ptr<SnapshotComparator> comparator);
  ~WebServer();

  bool start(int port = 8080);
  void stop();
  bool is_running() const { return running_; }

  void set_current_snapshot(std::shared_ptr<SystemSnapshot> snapshot) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_snapshot_ = snapshot;
  }

private:
  std::shared_ptr<SnapshotManager> snapshot_manager_;
  std::shared_ptr<SnapshotComparator> comparator_;
  std::unique_ptr<httplib::Server> server_;
  std::thread server_thread_;
  std::atomic<bool> running_{false};

  std::shared_ptr<SystemSnapshot> current_snapshot_;
  mutable std::mutex mutex_;

  void setup_routes();
};

} // namespace snapshot