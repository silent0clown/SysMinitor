# 系统监控工具 API 文档

## 文档信息

- **文档版本**: v1.0.0
- **基础URL**: `http://localhost:8080`
- **默认端口**: 8080
- **数据格式**: JSON
- **字符编码**: UTF-8

## 通用响应格式

### 成功响应
```json
{
  "code": 200,
  "message": "success",
  "data": { ... }
}
```

### 错误响应
```json
{
  "code": 400,
  "message": "错误描述",
  "data": null
}
```

## 通用状态码

| 状态码 | 说明 |
|--------|------|
| 200 | 请求成功 |
| 400 | 请求参数错误 |
| 404 | 资源未找到 |
| 500 | 服务器内部错误 |

## API接口列表

### 1. CPU相关接口

#### 1.1 获取CPU信息
- **接口说明**: 获取CPU的硬件信息
- **请求URL**: `/api/cpu/info`
- **请求方法**: GET
- **认证要求**: 否

**响应示例**:
```json
{
  "code": 200,
  "message": "success",
  "data": {
    "name": "Intel(R) Core(TM) i7-10700K CPU @ 3.80GHz",
    "vendor": "GenuineIntel",
    "architecture": "x86_64",
    "physicalCores": 8,
    "logicalCores": 16,
    "packages": 1,
    "baseFrequency": 3.8,
    "maxFrequency": 5.1
  }
}
```

#### 1.2 获取当前CPU使用率
- **接口说明**: 获取当前CPU总使用率
- **请求URL**: `/api/cpu/usage`
- **请求方法**: GET
- **认证要求**: 否

**响应示例**:
```json
{
  "code": 200,
  "message": "success",
  "data": {
    "usage": 23.5,
    "timestamp": 1635427800000,
    "unit": "percent"
  }
}
```

#### 1.3 获取CPU历史数据
- **接口说明**: 获取CPU使用率的历史数据
- **请求URL**: `/api/cpu/history`
- **请求方法**: GET
- **认证要求**: 否

**响应示例**:
```json
{
  "code": 200,
  "message": "success",
  "data": [
    {"timestamp": 1635427800000, "value": 23.5},
    {"timestamp": 1635427860000, "value": 18.2},
    {"timestamp": 1635427920000, "value": 31.7}
  ]
}
```

#### 1.4 实时CPU使用率流
- **接口说明**: Server-Sent Events (SSE) 实时推送CPU使用率
- **请求URL**: `/api/cpu/stream`
- **请求方法**: GET
- **认证要求**: 否
- **Content-Type**: `text/event-stream`

**数据格式**:
```
data: {"usage": 23.5, "timestamp": 1635427800000}
```

### 2. 内存相关接口

#### 2.1 获取内存使用情况
- **接口说明**: 获取当前内存使用详情
- **请求URL**: `/api/memory/usage`
- **请求方法**: GET
- **认证要求**: 否

**响应示例**:
```json
{
  "code": 200,
  "message": "success",
  "data": {
    "totalPhysical": 17179869184,
    "availablePhysical": 8589934592,
    "usedPhysical": 8589934592,
    "usedPercent": 50.0,
    "timestamp": 1635427800000,
    "unit": "bytes"
  }
}
```

#### 2.2 获取内存历史数据
- **接口说明**: 获取内存使用率的历史数据
- **请求URL**: `/api/memory/history`
- **请求方法**: GET
- **认证要求**: 否

**响应示例**:
```json
{
  "code": 200,
  "message": "success",
  "data": [
    {"timestamp": 1635427800000, "value": 45.2},
    {"timestamp": 1635427860000, "value": 47.8},
    {"timestamp": 1635427920000, "value": 43.1}
  ]
}
```

### 3. 系统信息接口

#### 3.1 获取系统概要信息
- **接口说明**: 获取系统CPU和内存的概要信息
- **请求URL**: `/api/system/info`
- **请求方法**: GET
- **认证要求**: 否

**响应示例**:
```json
{
  "code": 200,
  "message": "success",
  "data": {
    "cpu": {
      "info": {
        "name": "Intel(R) Core(TM) i7-10700K",
        "cores": 8,
        "threads": 16
      },
      "currentUsage": 23.5
    },
    "timestamp": 1635427800000
  }
}
```

### 4. 进程管理接口

#### 4.1 获取进程列表
- **接口说明**: 获取系统当前所有进程列表
- **请求URL**: `/api/processes`
- **请求方法**: GET
- **认证要求**: 否

**响应示例**:
```json
{
  "code": 200,
  "message": "success",
  "data": {
    "timestamp": 1635427800000,
    "totalProcesses": 156,
    "totalThreads": 2345,
    "processes": [
      {
        "pid": 1234,
        "parentPid": 567,
        "name": "chrome.exe",
        "fullPath": "C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe",
        "state": "Running",
        "username": "DOMAIN\\user",
        "cpuUsage": 5.2,
        "memoryUsage": 250.5,
        "workingSetSize": 1024000000,
        "pagefileUsage": 512000000,
        "createTime": "2023-10-27T10:30:00Z",
        "priority": "Normal",
        "threadCount": 45,
        "commandLine": "chrome.exe --type=renderer"
      }
    ]
  }
}
```

#### 4.2 获取进程详情
- **接口说明**: 根据PID获取特定进程的详细信息
- **请求URL**: `/api/process/{pid}`
- **请求方法**: GET
- **路径参数**:
  - `pid`: 进程ID (数字)

**响应示例**:
```json
{
  "code": 200,
  "message": "success",
  "data": {
    "pid": 1234,
    "parentPid": 567,
    "name": "chrome.exe",
    "fullPath": "C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe",
    "state": "Running",
    "username": "DOMAIN\\user",
    "cpuUsage": 5.2,
    "memoryUsage": 250.5,
    "workingSetSize": 1024000000,
    "pagefileUsage": 512000000,
    "createTime": "2023-10-27T10:30:00Z",
    "priority": "Normal",
    "threadCount": 45,
    "commandLine": "chrome.exe --type=renderer"
  }
}
```

#### 4.3 查找进程
- **接口说明**: 根据进程名查找进程
- **请求URL**: `/api/process/find`
- **请求方法**: GET
- **查询参数**:
  - `name`: 进程名称 (必需)

**响应示例**:
```json
{
  "code": 200,
  "message": "success",
  "data": [
    {
      "pid": 1234,
      "name": "chrome.exe",
      "cpuUsage": 5.2,
      "memoryUsage": 250.5,
      "username": "DOMAIN\\user",
      "threadCount": 45
    }
  ]
}
```

#### 4.4 终止进程
- **接口说明**: 终止指定进程
- **请求URL**: `/api/process/{pid}/terminate`
- **请求方法**: POST
- **路径参数**:
  - `pid`: 进程ID (数字)
- **查询参数**:
  - `exitCode`: 退出代码 (可选，默认0)

**请求示例**:
```json
{
  "exitCode": 0
}
```

**响应示例**:
```json
{
  "code": 200,
  "message": "success",
  "data": {
    "success": true,
    "pid": 1234
  }
}
```

### 5. 磁盘信息接口

#### 5.1 获取磁盘信息
- **接口说明**: 获取磁盘驱动器和分区信息
- **请求URL**: `/api/disk/info`
- **请求方法**: GET
- **认证要求**: 否
- **CORS**: 支持

**响应示例**:
```json
{
  "code": 200,
  "message": "success",
  "data": {
    "timestamp": 1635427800000,
    "drives": [
      {
        "model": "Samsung SSD 860 EVO 1TB",
        "serialNumber": "S3Z5NB0K123456",
        "interfaceType": "SATA",
        "mediaType": "SSD",
        "totalSize": 1000204886016,
        "bytesPerSector": 512,
        "status": "OK",
        "deviceId": "\\\\.\\PHYSICALDRIVE0"
      }
    ],
    "partitions": [
      {
        "driveLetter": "C:",
        "label": "System",
        "fileSystem": "NTFS",
        "totalSize": 256060514304,
        "freeSpace": 128030257152,
        "usedSpace": 128030257152,
        "usagePercentage": 50.0,
        "serialNumber": "1234-ABCD"
      }
    ]
  }
}
```

#### 5.2 获取磁盘性能
- **接口说明**: 获取磁盘性能计数器数据
- **请求URL**: `/api/disk/performance`
- **请求方法**: GET
- **认证要求**: 否
- **CORS**: 支持

**响应示例**:
```json
{
  "code": 200,
  "message": "success",
  "data": {
    "timestamp": 1635427800000,
    "performance": [
      {
        "driveLetter": "C:",
        "readSpeed": 150.5,
        "writeSpeed": 85.2,
        "readBytesPerSec": 157286400,
        "writeBytesPerSec": 89338624,
        "readCountPerSec": 125,
        "writeCountPerSec": 89,
        "queueLength": 0.5,
        "usagePercentage": 15.2,
        "responseTime": 5.2
      }
    ]
  }
}
```

### 6. 注册表管理接口

#### 6.1 获取注册表快照
- **接口说明**: 获取当前系统注册表快照
- **请求URL**: `/api/registry/snapshot`
- **请求方法**: GET
- **认证要求**: 否
- **CORS**: 支持

**响应示例**:
```json
{
  "code": 200,
  "message": "success",
  "data": {
    "timestamp": 1635427800000,
    "systemInfo": [
      {
        "path": "HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet",
        "values": [
          {
            "name": "CurrentUser",
            "type": "REG_SZ",
            "data": "Administrator",
            "size": 26
          }
        ],
        "subkeys": ["Control", "Enum", "Services"]
      }
    ],
    "software": [...],
    "network": [...]
  }
}
```

#### 6.2 保存注册表快照
- **接口说明**: 保存当前注册表状态为快照
- **请求URL**: `/api/registry/snapshot/save`
- **请求方法**: POST
- **认证要求**: 否
- **CORS**: 支持
- **请求体**: JSON

**请求示例**:
```json
{
  "snapshotName": "before_install",
  "systemInfo": [...],
  "software": [...],
  "network": [...]
}
```

**响应示例**:
```json
{
  "code": 200,
  "message": "success",
  "data": {
    "success": true,
    "snapshotName": "before_install",
    "timestamp": 1635427800000,
    "systemInfoCount": 45,
    "softwareCount": 123,
    "networkCount": 12,
    "message": "快照保存成功"
  }
}
```

#### 6.3 获取已保存的快照列表
- **接口说明**: 获取所有已保存的注册表快照列表
- **请求URL**: `/api/registry/snapshots`
- **请求方法**: GET
- **认证要求**: 否
- **CORS**: 支持

**响应示例**:
```json
{
  "code": 200,
  "message": "success",
  "data": [
    {
      "name": "before_install",
      "timestamp": 1635427800000,
      "systemInfoCount": 45,
      "softwareCount": 123,
      "networkCount": 12
    }
  ]
}
```

#### 6.4 比较注册表快照
- **接口说明**: 比较两个注册表快照的差异
- **请求URL**: `/api/registry/snapshots/compare`
- **请求方法**: POST
- **认证要求**: 否
- **CORS**: 支持
- **请求体**: JSON

**请求示例**:
```json
{
  "snapshot1": "before_install",
  "snapshot2": "after_install"
}
```

**响应示例**:
```json
{
  "code": 200,
  "message": "success",
  "data": {
    "snapshot1": "before_install",
    "snapshot2": "after_install",
    "timestamp1": 1635427800000,
    "timestamp2": 1635431400000,
    "systemInfoChanges": {
      "added": ["HKEY_LOCAL_MACHINE\\SYSTEM\\NewKey"],
      "removed": ["HKEY_LOCAL_MACHINE\\SYSTEM\\OldKey"],
      "modified": ["HKEY_LOCAL_MACHINE\\SYSTEM\\ModifiedKey"]
    },
    "softwareChanges": {...},
    "networkChanges": {...}
  }
}
```

#### 6.5 删除注册表快照
- **接口说明**: 删除指定的注册表快照
- **请求URL**: `/api/registry/snapshots/delete/{snapshotName}`
- **请求方法**: DELETE
- **路径参数**:
  - `snapshotName`: 快照名称

**响应示例**:
```json
{
  "code": 200,
  "message": "success",
  "data": {
    "success": true,
    "message": "快照删除成功"
  }
}
```

### 7. 驱动程序接口

#### 7.1 获取驱动快照
- **接口说明**: 获取系统驱动程序快照
- **请求URL**: `/api/drivers/snapshot`
- **请求方法**: GET
- **认证要求**: 否
- **CORS**: 支持

**响应示例**:
```json
{
  "code": 200,
  "message": "success",
  "data": {
    "timestamp": 1635427800000,
    "statistics": {
      "totalDrivers": 256,
      "runningCount": 198,
      "stoppedCount": 58,
      "kernelCount": 145,
      "fileSystemCount": 23,
      "autoStartCount": 167,
      "thirdPartyCount": 89
    },
    "kernelDrivers": [
      {
        "name": "disk.sys",
        "displayName": "Disk Driver",
        "description": "磁盘驱动程序",
        "state": "Running",
        "startType": "Auto",
        "binaryPath": "C:\\Windows\\System32\\drivers\\disk.sys",
        "serviceType": "Kernel Driver",
        "errorControl": "Normal",
        "account": "LocalSystem",
        "driverType": "Kernel",
        "pid": 0
      }
    ],
    "fileSystemDrivers": [...],
    "runningDrivers": [...],
    "stoppedDrivers": [...],
    "autoStartDrivers": [...],
    "thirdPartyDrivers": [...]
  }
}
```

#### 7.2 获取驱动详情
- **接口说明**: 获取特定驱动程序的详细信息
- **请求URL**: `/api/drivers/detail`
- **请求方法**: GET
- **查询参数**:
  - `name`: 驱动名称 (必需)

**响应示例**:
```json
{
  "code": 200,
  "message": "success",
  "data": {
    "name": "disk.sys",
    "displayName": "Disk Driver",
    "description": "磁盘驱动程序",
    "state": "Running",
    "startType": "Auto",
    "binaryPath": "C:\\Windows\\System32\\drivers\\disk.sys",
    "serviceType": "Kernel Driver",
    "errorControl": "Normal",
    "account": "LocalSystem",
    "group": "SCSI Miniport",
    "tagId": 27,
    "driverType": "Kernel",
    "pid": 0,
    "exitCode": 0,
    "win32ExitCode": 0,
    "serviceSpecificExitCode": 0
  }
}
```

### 8. 系统快照（SystemSnapshot）API

本组接口用于创建、保存、获取和删除整机系统快照（包含 CPU/内存/磁盘/驱动/注册表/进程等信息）。

注意事项:
- 快照名称支持 UTF-8（例如可以使用中文）。服务端会以 `snapshot_YYYYMMDD_HHMMSS` 为默认名称（精确到秒）当未提供名称时使用。
- 当快照存储到磁盘时，文件名会使用快照名称并追加 `.json` 后缀。请避免在名称中使用文件系统不允许的字符（例如 Windows 中的 `<>:\"/\\|?*`）。

#### 8.1 创建系统快照（不持久化）
- 接口说明: 生成当前系统快照并返回 JSON 内容（仅保存在内存，未写入磁盘）
- 请求URL: `/api/system/snapshot/create`
- 请求方法: POST
- 请求体 (可选): JSON，可以包含 { "name": "自定义名称" } 来指定返回的内存快照名称

响应示例:
```json
{
  "code": 200,
  "message": "success",
  "data": {
    "success": true,
    "name": "snapshot_20251018_153045",
    "timestamp": "2025-10-18 15:30:45"
  }
}
```

#### 8.2 保存系统快照到磁盘
- 接口说明: 将内存中的快照保存到磁盘，或在请求中提交快照内容并保存
- 请求URL: `/api/system/snapshot/save`
- 请求方法: POST
- 请求参数: 可通过查询参数 `name` 指定保存名称，或在 JSON 请求体中提供 `{ "name": "自定义名称", ...snapshot... }`。
- 请求体 (可选): 如果请求体为空，服务端会尝试将当前内存快照（按 name 查找）保存；如果指定了完整 snapshot JSON，服务端会直接持久化它。

请求示例（通过查询参数指定名称并保存当前内存快照）:
```text
POST /api/system/snapshot/save?name=我的快照
```

响应示例:
```json
{
  "code": 200,
  "message": "success",
  "data": {
    "success": true,
    "name": "我的快照"
  }
}
```

#### 8.3 列表已保存的系统快照
- 接口说明: 列出当前内存缓存和磁盘上已保存的快照名称（按名称排序，默认最新的排在前面）
- 请求URL: `/api/system/snapshot/list`
- 请求方法: GET

响应示例:
```json
{
  "code": 200,
  "message": "success",
  "data": [
    "snapshot_20251018_153045",
    "snapshot_20251018_152800",
    "我的快照"
  ]
}
```

#### 8.4 获取已保存的系统快照内容
- 接口说明: 根据快照名称返回完整 JSON 快照内容（优先在内存中查找，找不到则从磁盘读取）
- 请求URL: `/api/system/snapshot/get`
- 请求方法: GET
- 查询参数: `name` (必需)

请求示例:
```text
GET /api/system/snapshot/get?name=snapshot_20251018_153045
```

响应示例（返回快照 JSON）:
```json
{
  "code": 200,
  "message": "success",
  "data": { /* 完整的 SystemSnapshot JSON 内容 */ }
}
```

#### 8.5 删除系统快照
- 接口说明: 从内存缓存和磁盘删除指定名称的快照
- 请求URL: `/api/system/snapshot/delete/{snapshotName}`
- 请求方法: DELETE
- 路径参数: `snapshotName`（快照名称）

响应示例:
```json
{
  "code": 200,
  "message": "success",
  "data": {
    "success": true,
    "deletedInMemory": true,
    "deletedOnDisk": true
  }
}
```

错误情况示例:

```json
{
  "code": 404,
  "message": "Snapshot not found",
  "data": null
}
```

## 测试建议

### 1. 基础功能测试
1. 启动服务后访问 `http://localhost:8080` 查看测试页面
2. 依次调用各GET接口验证数据返回
3. 检查响应格式是否符合文档规范

### 2. 性能测试
1. 频繁调用CPU使用率接口检查性能
2. 测试流式接口的连接稳定性
3. 验证历史数据接口的数据完整性

### 3. 错误处理测试
1. 测试不存在的PID访问进程详情
2. 测试缺少必需参数的接口
3. 验证错误响应的格式

### 4. 兼容性测试
1. 验证CORS头是否正确设置
2. 测试不同浏览器的兼容性
3. 验证UTF-8编码处理

## 注意事项

1. **实时数据**: CPU和内存使用率数据每1秒更新一次
2. **历史数据**: 默认保存最近1000个样本点
3. **进程操作**: 终止进程操作需要管理员权限
4. **注册表操作**: 涉及系统关键数据，操作前请备份
5. **流式连接**: CPU流接口为长连接，注意连接管理

## 故障排除

- 如果端口8080被占用，可以修改启动端口
- 确保有足够的权限访问系统信息
- 检查防火墙设置是否允许本地连接
- 查看控制台输出获取详细错误信息

---

*文档最后更新: 2025年10月15日*