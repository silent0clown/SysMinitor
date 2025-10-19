# ϵͳ��ع��� API �ĵ�

## �ĵ���Ϣ

- **�ĵ��汾**: v1.0.0
- **����URL**: `http://localhost:8080`
- **Ĭ�϶˿�**: 8080
- **���ݸ�ʽ**: JSON
- **�ַ�����**: UTF-8

## ͨ����Ӧ��ʽ

### �ɹ���Ӧ
```json
{
  "code": 200,
  "message": "success",
  "data": { ... }
}
```

### ������Ӧ
```json
{
  "code": 400,
  "message": "��������",
  "data": null
}
```

## ͨ��״̬��

| ״̬�� | ˵�� |
|--------|------|
| 200 | ����ɹ� |
| 400 | ����������� |
| 404 | ��Դδ�ҵ� |
| 500 | �������ڲ����� |

## API�ӿ��б�

### 1. CPU��ؽӿ�

#### 1.1 ��ȡCPU��Ϣ
- **�ӿ�˵��**: ��ȡCPU��Ӳ����Ϣ
- **����URL**: `/api/cpu/info`
- **���󷽷�**: GET
- **��֤Ҫ��**: ��

**��Ӧʾ��**:
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

#### 1.2 ��ȡ��ǰCPUʹ����
- **�ӿ�˵��**: ��ȡ��ǰCPU��ʹ����
- **����URL**: `/api/cpu/usage`
- **���󷽷�**: GET
- **��֤Ҫ��**: ��

**��Ӧʾ��**:
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

#### 1.3 ��ȡCPU��ʷ����
- **�ӿ�˵��**: ��ȡCPUʹ���ʵ���ʷ����
- **����URL**: `/api/cpu/history`
- **���󷽷�**: GET
- **��֤Ҫ��**: ��

**��Ӧʾ��**:
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

#### 1.4 ʵʱCPUʹ������
- **�ӿ�˵��**: Server-Sent Events (SSE) ʵʱ����CPUʹ����
- **����URL**: `/api/cpu/stream`
- **���󷽷�**: GET
- **��֤Ҫ��**: ��
- **Content-Type**: `text/event-stream`

**���ݸ�ʽ**:
```
data: {"usage": 23.5, "timestamp": 1635427800000}
```

### 2. �ڴ���ؽӿ�

#### 2.1 ��ȡ�ڴ�ʹ�����
- **�ӿ�˵��**: ��ȡ��ǰ�ڴ�ʹ������
- **����URL**: `/api/memory/usage`
- **���󷽷�**: GET
- **��֤Ҫ��**: ��

**��Ӧʾ��**:
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

#### 2.2 ��ȡ�ڴ���ʷ����
- **�ӿ�˵��**: ��ȡ�ڴ�ʹ���ʵ���ʷ����
- **����URL**: `/api/memory/history`
- **���󷽷�**: GET
- **��֤Ҫ��**: ��

**��Ӧʾ��**:
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

### 3. ϵͳ��Ϣ�ӿ�

#### 3.1 ��ȡϵͳ��Ҫ��Ϣ
- **�ӿ�˵��**: ��ȡϵͳCPU���ڴ�ĸ�Ҫ��Ϣ
- **����URL**: `/api/system/info`
- **���󷽷�**: GET
- **��֤Ҫ��**: ��

**��Ӧʾ��**:
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

### 4. ���̹���ӿ�

#### 4.1 ��ȡ�����б�
- **�ӿ�˵��**: ��ȡϵͳ��ǰ���н����б�
- **����URL**: `/api/processes`
- **���󷽷�**: GET
- **��֤Ҫ��**: ��

**��Ӧʾ��**:
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

#### 4.2 ��ȡ��������
- **�ӿ�˵��**: ����PID��ȡ�ض����̵���ϸ��Ϣ
- **����URL**: `/api/process/{pid}`
- **���󷽷�**: GET
- **·������**:
  - `pid`: ����ID (����)

**��Ӧʾ��**:
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

#### 4.3 ���ҽ���
- **�ӿ�˵��**: ���ݽ��������ҽ���
- **����URL**: `/api/process/find`
- **���󷽷�**: GET
- **��ѯ����**:
  - `name`: �������� (����)

**��Ӧʾ��**:
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

#### 4.4 ��ֹ����
- **�ӿ�˵��**: ��ָֹ������
- **����URL**: `/api/process/{pid}/terminate`
- **���󷽷�**: POST
- **·������**:
  - `pid`: ����ID (����)
- **��ѯ����**:
  - `exitCode`: �˳����� (��ѡ��Ĭ��0)

**����ʾ��**:
```json
{
  "exitCode": 0
}
```

**��Ӧʾ��**:
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

### 5. ������Ϣ�ӿ�

#### 5.1 ��ȡ������Ϣ
- **�ӿ�˵��**: ��ȡ�����������ͷ�����Ϣ
- **����URL**: `/api/disk/info`
- **���󷽷�**: GET
- **��֤Ҫ��**: ��
- **CORS**: ֧��

**��Ӧʾ��**:
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

#### 5.2 ��ȡ��������
- **�ӿ�˵��**: ��ȡ�������ܼ���������
- **����URL**: `/api/disk/performance`
- **���󷽷�**: GET
- **��֤Ҫ��**: ��
- **CORS**: ֧��

**��Ӧʾ��**:
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

### 6. ע������ӿ�

#### 6.1 ��ȡע������
- **�ӿ�˵��**: ��ȡ��ǰϵͳע������
- **����URL**: `/api/registry/snapshot`
- **���󷽷�**: GET
- **��֤Ҫ��**: ��
- **CORS**: ֧��

**��Ӧʾ��**:
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

#### 6.2 ����ע������
- **�ӿ�˵��**: ���浱ǰע���״̬Ϊ����
- **����URL**: `/api/registry/snapshot/save`
- **���󷽷�**: POST
- **��֤Ҫ��**: ��
- **CORS**: ֧��
- **������**: JSON

**����ʾ��**:
```json
{
  "snapshotName": "before_install",
  "systemInfo": [...],
  "software": [...],
  "network": [...]
}
```

**��Ӧʾ��**:
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
    "message": "���ձ���ɹ�"
  }
}
```

#### 6.3 ��ȡ�ѱ���Ŀ����б�
- **�ӿ�˵��**: ��ȡ�����ѱ����ע�������б�
- **����URL**: `/api/registry/snapshots`
- **���󷽷�**: GET
- **��֤Ҫ��**: ��
- **CORS**: ֧��

**��Ӧʾ��**:
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

#### 6.4 �Ƚ�ע������
- **�ӿ�˵��**: �Ƚ�����ע�����յĲ���
- **����URL**: `/api/registry/snapshots/compare`
- **���󷽷�**: POST
- **��֤Ҫ��**: ��
- **CORS**: ֧��
- **������**: JSON

**����ʾ��**:
```json
{
  "snapshot1": "before_install",
  "snapshot2": "after_install"
}
```

**��Ӧʾ��**:
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

#### 6.5 ɾ��ע������
- **�ӿ�˵��**: ɾ��ָ����ע������
- **����URL**: `/api/registry/snapshots/delete/{snapshotName}`
- **���󷽷�**: DELETE
- **·������**:
  - `snapshotName`: ��������

**��Ӧʾ��**:
```json
{
  "code": 200,
  "message": "success",
  "data": {
    "success": true,
    "message": "����ɾ���ɹ�"
  }
}
```

### 7. ��������ӿ�

#### 7.1 ��ȡ��������
- **�ӿ�˵��**: ��ȡϵͳ�����������
- **����URL**: `/api/drivers/snapshot`
- **���󷽷�**: GET
- **��֤Ҫ��**: ��
- **CORS**: ֧��

**��Ӧʾ��**:
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
        "description": "������������",
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

#### 7.2 ��ȡ��������
- **�ӿ�˵��**: ��ȡ�ض������������ϸ��Ϣ
- **����URL**: `/api/drivers/detail`
- **���󷽷�**: GET
- **��ѯ����**:
  - `name`: �������� (����)

**��Ӧʾ��**:
```json
{
  "code": 200,
  "message": "success",
  "data": {
    "name": "disk.sys",
    "displayName": "Disk Driver",
    "description": "������������",
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

### 8. ϵͳ���գ�SystemSnapshot��API

����ӿ����ڴ��������桢��ȡ��ɾ������ϵͳ���գ����� CPU/�ڴ�/����/����/ע���/���̵���Ϣ����

ע������:
- ��������֧�� UTF-8���������ʹ�����ģ�������˻��� `snapshot_YYYYMMDD_HHMMSS` ΪĬ�����ƣ���ȷ���룩��δ�ṩ����ʱʹ�á�
- �����մ洢������ʱ���ļ�����ʹ�ÿ������Ʋ�׷�� `.json` ��׺���������������ʹ���ļ�ϵͳ��������ַ������� Windows �е� `<>:\"/\\|?*`����

#### 8.1 ����ϵͳ���գ����־û���
- �ӿ�˵��: ���ɵ�ǰϵͳ���ղ����� JSON ���ݣ����������ڴ棬δд����̣�
- ����URL: `/api/system/snapshot/create`
- ���󷽷�: POST
- ������ (��ѡ): JSON�����԰��� { "name": "�Զ�������" } ��ָ�����ص��ڴ��������

��Ӧʾ��:
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

#### 8.2 ����ϵͳ���յ�����
- �ӿ�˵��: ���ڴ��еĿ��ձ��浽���̣������������ύ�������ݲ�����
- ����URL: `/api/system/snapshot/save`
- ���󷽷�: POST
- �������: ��ͨ����ѯ���� `name` ָ���������ƣ����� JSON ���������ṩ `{ "name": "�Զ�������", ...snapshot... }`��
- ������ (��ѡ): ���������Ϊ�գ�����˻᳢�Խ���ǰ�ڴ���գ��� name ���ң����棻���ָ�������� snapshot JSON������˻�ֱ�ӳ־û�����

����ʾ����ͨ����ѯ����ָ�����Ʋ����浱ǰ�ڴ���գ�:
```text
POST /api/system/snapshot/save?name=�ҵĿ���
```

��Ӧʾ��:
```json
{
  "code": 200,
  "message": "success",
  "data": {
    "success": true,
    "name": "�ҵĿ���"
  }
}
```

#### 8.3 �б��ѱ����ϵͳ����
- �ӿ�˵��: �г���ǰ�ڴ滺��ʹ������ѱ���Ŀ������ƣ�����������Ĭ�����µ�����ǰ�棩
- ����URL: `/api/system/snapshot/list`
- ���󷽷�: GET

��Ӧʾ��:
```json
{
  "code": 200,
  "message": "success",
  "data": [
    "snapshot_20251018_153045",
    "snapshot_20251018_152800",
    "�ҵĿ���"
  ]
}
```

#### 8.4 ��ȡ�ѱ����ϵͳ��������
- �ӿ�˵��: ���ݿ������Ʒ������� JSON �������ݣ��������ڴ��в��ң��Ҳ�����Ӵ��̶�ȡ��
- ����URL: `/api/system/snapshot/get`
- ���󷽷�: GET
- ��ѯ����: `name` (����)

����ʾ��:
```text
GET /api/system/snapshot/get?name=snapshot_20251018_153045
```

��Ӧʾ�������ؿ��� JSON��:
```json
{
  "code": 200,
  "message": "success",
  "data": { /* ������ SystemSnapshot JSON ���� */ }
}
```

#### 8.5 ɾ��ϵͳ����
- �ӿ�˵��: ���ڴ滺��ʹ���ɾ��ָ�����ƵĿ���
- ����URL: `/api/system/snapshot/delete/{snapshotName}`
- ���󷽷�: DELETE
- ·������: `snapshotName`���������ƣ�

��Ӧʾ��:
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

�������ʾ��:

```json
{
  "code": 404,
  "message": "Snapshot not found",
  "data": null
}
```

## ���Խ���

### 1. �������ܲ���
1. ������������ `http://localhost:8080` �鿴����ҳ��
2. ���ε��ø�GET�ӿ���֤���ݷ���
3. �����Ӧ��ʽ�Ƿ�����ĵ��淶

### 2. ���ܲ���
1. Ƶ������CPUʹ���ʽӿڼ������
2. ������ʽ�ӿڵ������ȶ���
3. ��֤��ʷ���ݽӿڵ�����������

### 3. ���������
1. ���Բ����ڵ�PID���ʽ�������
2. ����ȱ�ٱ�������Ľӿ�
3. ��֤������Ӧ�ĸ�ʽ

### 4. �����Բ���
1. ��֤CORSͷ�Ƿ���ȷ����
2. ���Բ�ͬ������ļ�����
3. ��֤UTF-8���봦��

## ע������

1. **ʵʱ����**: CPU���ڴ�ʹ��������ÿ1�����һ��
2. **��ʷ����**: Ĭ�ϱ������1000��������
3. **���̲���**: ��ֹ���̲�����Ҫ����ԱȨ��
4. **ע������**: �漰ϵͳ�ؼ����ݣ�����ǰ�뱸��
5. **��ʽ����**: CPU���ӿ�Ϊ�����ӣ�ע�����ӹ���

## �����ų�

- ����˿�8080��ռ�ã������޸������˿�
- ȷ�����㹻��Ȩ�޷���ϵͳ��Ϣ
- ������ǽ�����Ƿ�����������
- �鿴����̨�����ȡ��ϸ������Ϣ

---

*�ĵ�������: 2025��10��15��*