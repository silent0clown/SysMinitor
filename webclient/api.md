# 操作系统检测工具 - 前后端交互接口规范

## 接口概述
本文档定义了操作系统检测工具前端与后端的数据交互规范。后端负责采集Windows系统信息，前端负责数据可视化和用户交互。

## 基础信息
- 接口协议：HTTP/HTTPS
- 数据格式：JSON
- 编码：UTF-8
- 跨域支持：CORS

## 接口详情

### 1. 系统实时监控数据获取
**接口地址**: `/api/system/realtime`
**请求方法**: GET
**描述**: 获取当前系统实时运行状态数据

**请求参数**:
```json
{
  "timestamp": "2025-10-10T14:30:22Z"
}
```

**响应数据**:
```json
{
  "status": "success",
  "data": {
    "cpu": {
      "usage": 45.2,
      "cores": 8,
      "frequency": 3.6,
      "processes": 156,
      "top_processes": [
        {"pid": 1234, "name": "chrome.exe", "usage": 12.5, "memory": "1.2GB"},
        {"pid": 5678, "name": "vs_code.exe", "usage": 8.3, "memory": "800MB"}
      ]
    },
    "memory": {
      "total": "16.0GB",
      "used": "12.8GB",
      "usage": 80.0,
      "available": "3.2GB",
      "cached": "2.1GB"
    },
    "disk": {
      "drives": [
        {
          "letter": "C:",
          "total": "512GB",
          "used": "398GB",
          "free": "114GB",
          "usage": 77.7,
          "file_system": "NTFS"
        }
      ],
      "io_read": "45MB/s",
      "io_write": "23MB/s"
    },
    "network": {
      "upload_speed": "12.5Mbps",
      "download_speed": "89.2Mbps",
      "connections": 234
    },
    "system": {
      "os_version": "Windows 11 Pro 22H2",
      "uptime": "5天 12小时 30分",
      "boot_time": "2025-10-04T14:00:00Z",
      "hostname": "DESKTOP-ABC123"
    }
  }
}
```

### 2. 生成系统快照
**接口地址**: `/api/snapshot/create`
**请求方法**: POST
**描述**: 创建当前系统的完整快照

**请求参数**:
```json
{
  "name": "系统快照_2025-10-10_143022",
  "description": "测试前的系统基线快照",
  "include_registry": true,
  "include_processes": true,
  "include_files": true
}
```

**响应数据**:
```json
{
  "status": "success",
  "data": {
    "snapshot_id": "snap_20251010143022",
    "filename": "snapshot_2025-10-10_143022.json",
    "size": "2.3MB",
    "created_at": "2025-10-10T14:30:22Z",
    "duration": 4.8,
    "checksum": "sha256:abc123..."
  }
}
```

### 3. 获取快照列表
**接口地址**: `/api/snapshot/list`
**请求方法**: GET
**描述**: 获取所有保存的系统快照列表

**请求参数**:
```json
{
  "page": 1,
  "limit": 20,
  "sort_by": "created_at",
  "order": "desc"
}
```

**响应数据**:
```json
{
  "status": "success",
  "data": {
    "total": 15,
    "page": 1,
    "snapshots": [
      {
        "id": "snap_20251010143022",
        "name": "系统快照_2025-10-10_143022",
        "description": "测试前的系统基线快照",
        "created_at": "2025-10-10T14:30:22Z",
        "size": "2.3MB",
        "system_info": {
          "os_version": "Windows 11 Pro 22H2",
          "cpu_usage": 23.5,
          "memory_usage": 67.2,
          "disk_usage": 77.7
        },
        "tags": ["baseline", "pre-test"]
      }
    ]
  }
}
```

### 4. 快照对比分析
**接口地址**: `/api/snapshot/compare`
**请求方法**: POST
**描述**: 对比两个快照的差异

**请求参数**:
```json
{
  "baseline_id": "snap_20251010143022",
  "compare_id": "snap_20251010153022",
  "include_categories": ["registry", "processes", "files", "performance"]
}
```

**响应数据**:
```json
{
  "status": "success",
  "data": {
    "comparison_id": "comp_20251010153022",
    "baseline_time": "2025-10-10T14:30:22Z",
    "compare_time": "2025-10-10T15:30:22Z",
    "duration_minutes": 60,
    "summary": {
      "total_changes": 45,
      "critical_changes": 3,
      "warning_changes": 8,
      "info_changes": 34
    },
    "registry_changes": {
      "added": 12,
      "modified": 8,
      "deleted": 3,
      "critical_keys": [
        {
          "key": "HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services",
          "change_type": "added",
          "description": "新增服务项"
        }
      ]
    },
    "process_changes": {
      "started": 5,
      "stopped": 3,
      "memory_leaks": [
        {
          "name": "memory_leak_app.exe",
          "pid": 9876,
          "memory_increase": "150MB",
          "duration": "60分钟"
        }
      ]
    },
    "file_changes": {
      "created": 23,
      "modified": 15,
      "deleted": 7,
      "large_files": [
        {
          "path": "C:\\temp\\large_file.tmp",
          "size_change": "+2.1GB",
          "operation": "created"
        }
      ]
    },
    "performance_changes": {
      "cpu_trend": "+15.3%",
      "memory_trend": "+8.7%",
      "disk_io_trend": "+45.2%",
      "recommendations": [
        "检测到内存泄漏，建议重启相关进程",
        "磁盘IO显著增加，建议检查临时文件"
      ]
    }
  }
}
```

### 5. 系统健康检查
**接口地址**: `/api/system/health-check`
**请求方法**: GET
**描述**: 执行一键系统健康检查

**请求参数**:
```json
{
  "check_type": "full",
  "include_recommendations": true
}
```

**响应数据**:
```json
{
  "status": "success",
  "data": {
    "health_score": 72,
    "check_duration": 3.2,
    "checks": {
      "cpu": {
        "status": "warning",
        "score": 65,
        "details": "CPU使用率82%，建议关闭不必要的应用程序"
      },
      "memory": {
        "status": "critical",
        "score": 45,
        "details": "内存使用率93%，存在内存泄漏风险"
      },
      "disk": {
        "status": "good",
        "score": 85,
        "details": "磁盘空间充足"
      },
      "registry": {
        "status": "warning",
        "score": 70,
        "details": "发现3处注册表异常增长"
      },
      "security": {
        "status": "good",
        "score": 90,
        "details": "系统安全配置正常"
      }
    },
    "recommendations": [
      {
        "priority": "high",
        "category": "memory",
        "title": "关闭内存占用过高的进程",
        "description": "Chrome浏览器占用1.2GB内存，建议关闭不必要的标签页",
        "action": "close_process",
        "target": "chrome.exe"
      },
      {
        "priority": "medium",
        "category": "registry",
        "title": "清理注册表垃圾项",
        "description": "检测到15个无效的注册表项，建议清理",
        "action": "cleanup_registry"
      }
    ],
    "next_check_recommended": "2025-10-10T16:30:22Z"
  }
}
```

### 6. 导出报告
**接口地址**: `/api/report/export`
**请求方法**: POST
**描述**: 导出系统分析报告

**请求参数**:
```json
{
  "report_type": "pdf",
  "include_snapshots": ["snap_20251010143022", "snap_20251010153022"],
  "sections": ["summary", "performance", "recommendations"],
  "format": "detailed"
}
```

**响应数据**:
```json
{
  "status": "success",
  "data": {
    "report_id": "report_20251010153022",
    "filename": "系统检测报告_2025-10-10.pdf",
    "size": "1.8MB",
    "download_url": "/downloads/report_20251010153022.pdf",
    "expires_at": "2025-10-11T15:30:22Z"
  }
}
```

## 错误响应格式
所有接口的错误响应都遵循以下格式：
```json
{
  "status": "error",
  "error": {
    "code": "INVALID_PARAMETER",
    "message": "请求参数不正确",
    "details": "snapshot_id不能为空",
    "timestamp": "2025-10-10T14:30:22Z"
  }
}
```

## WebSocket 实时数据推送
**连接地址**: `ws://localhost:8080/ws/realtime`
**描述**: 实时推送系统监控数据

**消息格式**:
```json
{
  "type": "system_update",
  "timestamp": "2025-10-10T14:30:22Z",
  "data": {
    "cpu_usage": 45.2,
    "memory_usage": 80.0,
    "disk_io_read": 45.0,
    "disk_io_write": 23.0,
    "alert": {
      "level": "warning",
      "message": "CPU使用率超过80%",
      "suggestion": "建议检查运行中的进程"
    }
  }
}
```