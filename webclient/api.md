# 系统信息接口文档

## 1. CPU 信息
- **接口路径**: `/api/cpu/info`
- **描述**: 获取 CPU 的详细信息
- **响应示例**:
  ```json
  {
      "architecture": "x64",
      "baseFrequency": 3187,
      "logicalCores": 16,
      "maxFrequency": 3435973836,
      "name": "13th Gen Intel(R) Core(TM) i5-13500H",
      "packages": 1,
      "physicalCores": 12,
      "vendor": ""
  }
  ```

## 2. 系统信息
- **接口路径**: `/api/system/info`
- **描述**: 获取系统整体信息，包括 CPU 使用情况
- **响应示例**:
  ```json
  {
      "cpu": {
          "currentUsage": 8.897485493230175,
          "info": {
              "cores": 12,
              "name": "13th Gen Intel(R) Core(TM) i5-13500H",
              "threads": 16
          }
      },
      "timestamp": 1012987656
  }
  ```

## 3. CPU 使用率
- **接口路径**: `/api/cpu/usage`
- **描述**: 获取当前 CPU 使用率
- **响应示例**:
  ```json
  {
      "timestamp": 1012949593,
      "unit": "percent",
      "usage": 12.366114897760468
  }
  ```

## 4. CPU 使用率流
- **接口路径**: `/api/cpu/stream`
- **描述**: 获取实时 CPU 使用率流
- **响应示例**:
  ```json
  data: {
      "timestamp": 1013046250,
      "usage": 7.783251231527093
  }
  ```

## 5. 进程列表
- **接口路径**: `/api/processes`
- **描述**: 获取当前运行的进程列表
- **响应示例**:
  ```json
  {
      "processes": [
          {
              "commandLine": "",
              "cpuUsage": -9.255963134931783e+61,
              "createTime": -3689348814741910324,
              "fullPath": "",
              "memoryUsage": 14757395258967641292,
              "name": "[System Process]",
              "pagefileUsage": 14757395258967641292,
              "parentPid": 0,
              "pid": 0,
              "priority": 0,
              "state": "Running",
              "threadCount": 48,
              "username": "",
              "workingSetSize": 14757395258967641292
          },
          {
              "commandLine": "",
              "cpuUsage": -9.255963134931783e+61,
              "createTime": -3689348814741910324,
              "fullPath": "",
              "memoryUsage": 14757395258967641292,
              "name": "System",
              "pagefileUsage": 14757395258967641292,
              "parentPid": 0,
              "pid": 4,
              "priority": 8,
              "state": "Running",
              "threadCount": 442,
              "username": "",
              "workingSetSize": 14757395258967641292
          },
          {
              "commandLine": "",
              "cpuUsage": 0.0,
              "createTime": 134047266154156169,
              "fullPath": "",
              "memoryUsage": 119578624,
              "name": "svchost.exe",
              "pagefileUsage": 64622592,
              "parentPid": 1784,
              "pid": 44364,
              "priority": 8,
              "state": "Running",
              "threadCount": 6,
              "username": "",
              "workingSetSize": 119578624
          }
      ],
      "timestamp": 1013085171,
      "totalProcesses": 354,
      "totalThreads": 6039
  }
  ```

## 6. 指定进程信息
- **接口路径**: `/api/process/{pid}`
- **描述**: 获取指定 PID 的进程详细信息
- **响应示例**:
  ```json
  {
      "commandLine": "",
      "cpuUsage": 0.0,
      "createTime": 134047266154156169,
      "fullPath": "",
      "memoryUsage": 127303680,
      "name": "svchost.exe",
      "pagefileUsage": 71913472,
      "parentPid": 1784,
      "pid": 44364,
      "priority": 8,
      "state": "Running",
      "threadCount": 6,
      "username": "",
      "workingSetSize": 127303680
  }
  ```

## 7. 查找进程
- **接口路径**: `/api/process/find?name={name}`
- **描述**: 根据名称查找进程
- **响应示例**:
  ```json
  [
      {
          "cpuUsage": -9.255963134931783e+61,
          "memoryUsage": 14757395258967641292,
          "name": "svchost.exe",
          "pid": 1960,
          "threadCount": 12,
          "username": ""
      },
      {
          "cpuUsage": -9.255963134931783e+61,
          "memoryUsage": 14757395258967641292,
          "name": "svchost.exe",
          "pid": 1056,
          "threadCount": 11,
          "username": ""
      },
      {
          "cpuUsage": 0.0,
          "memoryUsage": 127385600,
          "name": "svchost.exe",
          "pid": 44364,
          "threadCount": 4,
          "username": ""
      }
  ]
  ```

## 8. 终止进程
- **接口路径**: `/api/process/{pid}/terminate`
- **描述**: 终止指定 PID 的进程
- **响应示例**:
  ```json
  {
      "pid": 37232,
      "success": true
  }
  ```

## 9. 磁盘信息
- **接口路径**: `/api/disk/info`
- **描述**: 获取磁盘驱动器和分区的详细信息
- **响应示例**:
  ```json
  {
      "drives": [
          {
              "bytesPerSector": 512,
              "deviceId": "C:\\",
              "interfaceType": "Fixed",
              "mediaType": "Unknown",
              "model": "Windows-SSD",
              "serialNumber": "",
              "status": "Healthy",
              "totalSize": 214749409280
          },
          {
              "bytesPerSector": 512,
              "deviceId": "D:\\",
              "interfaceType": "Fixed",
              "mediaType": "Unknown",
              "model": "Data",
              "serialNumber": "",
              "status": "Healthy",
              "totalSize": 294972813312
          },
          {
              "bytesPerSector": 512,
              "deviceId": "\\\\.\\PHYSICALDRIVE0",
              "interfaceType": "SCSI",
              "mediaType": "Fixed hard disk media",
              "model": "Micron MTFDKBA512TFH",
              "serialNumber": "00A0_7501_3CD7_57CF.",
              "status": "Healthy",
              "totalSize": 2298335765560
          }
      ],
      "partitions": [
          {
              "driveLetter": "C:",
              "fileSystem": "NTFS",
              "freeSpace": 29133791232,
              "label": "Windows-SSD",
              "serialNumber": 2764861816,
              "totalSize": 214749409280,
              "usagePercentage": 86.4335872542429,
              "usedSpace": 185615618048
          },
          {
              "driveLetter": "D:",
              "fileSystem": "NTFS",
              "freeSpace": 85361258496,
              "label": "Data",
              "serialNumber": 513519313,
              "totalSize": 294972813312,
              "usagePercentage": 71.06131323170068,
              "usedSpace": 209611554816
          }
      ],
      "timestamp": 1013836593
  }
  ```

## 10. 磁盘性能
- **接口路径**: `/api/disk/performance`
- **描述**: 获取磁盘性能数据
- **响应示例**:
  ```json
  {
      "performance": [
          {
              "driveLetter": "C:",
              "queueLength": 0.0,
              "readBytesPerSec": 0,
              "readCountPerSec": 0,
              "readSpeed": 0.0,
              "responseTime": 0,
              "usagePercentage": 0.0,
              "writeBytesPerSec": 0,
              "writeCountPerSec": 0,
              "writeSpeed": 0.0
          },
          {
              "driveLetter": "D:",
              "queueLength": 0.0,
              "readBytesPerSec": 0,
              "readCountPerSec": 0,
              "readSpeed": 0.0,
              "responseTime": 0,
              "usagePercentage": 0.0,
              "writeBytesPerSec": 0,
              "writeCountPerSec": 0,
              "writeSpeed": 0.0
          }
      ],
      "timestamp": 1013904296
  }
  ```