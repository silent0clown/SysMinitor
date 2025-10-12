// API配置
const API_CONFIG = {
    timeout: 5000,
    headers: {
        'Content-Type': 'application/json'
    }
};

// API请求封装
class APIClient {
    constructor(config) {
        this.timeout = config.timeout;
    }

    async request(endpoint, options = {}) {
        const url = endpoint;
        const config = {
            method: 'GET',
            headers: API_CONFIG.headers,
            ...options
        };

        try {
            const response = await fetch(url, {
                ...config,
                timeout: this.timeout
            });

            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }

            return await response.json();
        } catch (error) {
            console.error('API请求失败:', error);
            throw error;
        }
    }

    get(endpoint) {
        return this.request(endpoint, { method: 'GET' });
    }

    post(endpoint, data) {
        return this.request(endpoint, {
            method: 'POST',
            body: JSON.stringify(data)
        });
    }
}

// 创建API客户端实例
const apiClient = new APIClient(API_CONFIG);

// 系统监控数据管理
class SystemMonitor {
    constructor() {
        this.charts = {};
        this.realTimeData = {
            cpu: [],
            memory: [],
            network: []
        };
        this.currentFilter = 'all';
        this.currentSort = { field: 'cpu', direction: 'desc' };
        this.performanceTimeRange = '1h'; // 默认时间范围
        this.alerts = [];
        this.processes = [];
        this.apiClient = apiClient; // 注入API客户端
        this.init();
    }

    async init() {
        try {
            // 初始化图表
            this.initCharts();

            // 获取初始数据
            await this.fetchInitialData();

            // 启动实时更新
            this.startRealTimeUpdates();

            // 绑定事件
            // this.bindEvents();

            // 更新界面显示
            this.updateProcessList();
            this.updateAlerts();
            this.updateUptime();
        } catch (error) {
            console.error('初始化失败:', error);
            // 出错时使用模拟数据作为后备
            // this.generateMockData();
            // this.updateProcessList();
            // this.updateAlerts();
        }
    }

    // 获取初始数据
    async fetchInitialData() {
        try {
            // 并行获取多个数据
            const [cpuInfo, systemInfo, diskInfo] = await Promise.all([
                this.apiClient.get('/api/cpu/info'),
                this.apiClient.get('/api/system/info'),
                this.apiClient.get('/api/disk/info')
            ]);

            // 更新CPU信息
            this.updateCPUInfo(cpuInfo);

            // 更新CPU使用率
            const cpuUsageData = await this.apiClient.get('/api/cpu/usage');
            this.updateCPUUsage(cpuUsageData);

            // 更新系统信息
            this.updateSystemInfo(systemInfo);

            // 更新磁盘信息
            this.updateDiskInfo(diskInfo);

            // 初始化磁盘IO图表
            this.initDiskIOCharts(diskInfo.partitions); // 根据实际结构访问分区信息

            // 进程和警报数据暂时保留占位符
            this.processes = [];
            this.alerts = [];

            // 初始化排序图标
            document.getElementById('sort-cpu').innerHTML = '↓';

        } catch (error) {
            console.error('获取初始数据失败:', error);
            throw error;
        }
    }
    // 更新CPU卡片的所有信息
    updateCPUInfo(cpuInfo) {
        // 更新CPU名称（显示在仪表盘下方）
        if (cpuInfo.name) {
            document.getElementById('cpuName').textContent = cpuInfo.name;
        }

        // 更新厂商信息
        if (cpuInfo.vendor) {
            document.getElementById('cpuVendor').textContent = '厂商: ' + cpuInfo.vendor;
        }

        // 更新架构信息
        if (cpuInfo.architecture) {
            document.getElementById('cpuArchitecture').textContent = '架构: ' + cpuInfo.architecture;
        }

        // 更新物理核心数
        if (cpuInfo.physicalCores !== undefined) {
            document.getElementById('cpuPhysicalCores').textContent = '物理核心: ' + cpuInfo.physicalCores;
        }

        // 更新逻辑核心数
        if (cpuInfo.logicalCores !== undefined) {
            document.getElementById('cpuLogicalCores').textContent = '逻辑核心: ' + cpuInfo.logicalCores;
        }

        // 更新基准频率
        if (cpuInfo.baseFrequency !== undefined) {
            const baseFreqGHz = (cpuInfo.baseFrequency / 1000).toFixed(2);
            document.getElementById('cpuBaseFrequency').textContent = '基准频率: ' + baseFreqGHz + ' GHz';
        }

        // 更新最大频率
        if (cpuInfo.maxFrequency !== undefined) {
            const maxFreqGHz = (cpuInfo.maxFrequency / 1000).toFixed(2);
            document.getElementById('cpuMaxFrequency').textContent = '最大频率: ' + maxFreqGHz + ' GHz';
        }
    }



    // 添加 updateCPUUsage 方法来专门更新CPU使用率
    updateCPUUsage(cpuUsageData) {
        const cpuUsage = cpuUsageData.usage.toFixed(1);
        document.getElementById('cpuUsage').textContent = cpuUsage + '%';

        // 更新图表
        this.charts.cpu.setOption({
            series: [{
                data: [{
                    value: cpuUsage,
                    itemStyle: { color: cpuUsage > 95 ? '#dc2626' : cpuUsage > 80 ? '#d97706' : '#059669' }
                }]
            }]
        });

        // 更新状态指示器
        this.updateCPUStatus(cpuUsage);
    }
    updateCPUStatus(cpuUsage) {
        const cpuStatus = document.getElementById('cpuStatus');

        if (cpuUsage > 95) {
            cpuStatus.textContent = '危险';
            cpuStatus.className = 'status-critical text-sm font-medium pulse-animation';
        } else if (cpuUsage > 80) {
            cpuStatus.textContent = '警告';
            cpuStatus.className = 'status-warning text-sm font-medium pulse-animation';
        } else {
            cpuStatus.textContent = '良好';
            cpuStatus.className = 'status-good text-sm font-medium';
        }
    }
    // 更新系统信息显示
    updateSystemInfo(systemInfo) {
        if (systemInfo.os) {
            document.getElementById('system-os').textContent = systemInfo.os;
        }
        if (systemInfo.hostname) {
            document.getElementById('system-hostname').textContent = systemInfo.hostname;
        }
        if (systemInfo.architecture) {
            document.getElementById('system-arch').textContent = systemInfo.architecture;
        }
        if (systemInfo.version) {
            document.getElementById('system-version').textContent = systemInfo.version;
        }
    }
    // 更新磁盘信息显示
    updateDiskInfo(diskInfo) {
        // 存储磁盘信息供后续使用
        this.currentDiskInfo = diskInfo.partitions || []; // 根据实际返回结构调整

        // 更新整体磁盘状态指示器
        this.updateDiskOverallStatus(this.currentDiskInfo);
    }
    // 格式化字节大小显示
    formatBytes(bytes) {
        if (bytes === 0) return '0 Bytes';

        const k = 1024;
        const sizes = ['Bytes', 'KB', 'MB', 'GB', 'TB'];
        const i = Math.floor(Math.log(bytes) / Math.log(k));

        return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
    }

    // 初始化图表
    initCharts() {
        // CPU使用率图表
        this.charts.cpu = echarts.init(document.getElementById('cpuChart'));
        this.initCpuChart();

        // 内存使用率图表
        this.charts.memory = echarts.init(document.getElementById('memoryChart'));
        this.initMemoryChart();

        // 性能趋势图表
        this.charts.performance = echarts.init(document.getElementById('performanceChart'));
        this.initPerformanceChart();
    }
    // 初始化CPU图表
    initCpuChart() {
        const option = {
            series: [{
                type: 'gauge',
                startAngle: 90,
                endAngle: -270,
                pointer: { show: true },
                progress: {
                    show: true,
                    overlap: false,
                    roundCap: true,
                    clip: false,
                    itemStyle: { borderWidth: 1, borderColor: '#fff' }
                },
                axisLine: { lineStyle: { width: 8 } },
                splitLine: { show: false },
                axisTick: { show: false },
                axisLabel: { show: false },
                data: [{
                    value: 0, // 初始值设为0
                    name: 'CPU使用率',
                    title: { offsetCenter: ['0%', '0%'] },
                    detail: { show: false },
                    itemStyle: { color: '#059669' }
                }],
                radius: '100%'
            }]
        };
        this.charts.cpu.setOption(option);
    }
    // 初始化内存使用率图表
    initMemoryChart() {
        const option = {
            series: [{
                type: 'gauge',
                startAngle: 90,
                endAngle: -270,
                pointer: { show: true },
                progress: {
                    show: true,
                    overlap: false,
                    roundCap: true,
                    clip: false,
                    itemStyle: { borderWidth: 1, borderColor: '#fff' }
                },
                axisLine: { lineStyle: { width: 8 } },
                splitLine: { show: false },
                axisTick: { show: false },
                axisLabel: { show: false },
                data: [{
                    value: 0, // 初始值设为0
                    name: '内存使用率',
                    title: { offsetCenter: ['0%', '0%'] },
                    detail: { show: false },
                    itemStyle: { color: '#d97706' }
                }],
                radius: '100%'
            }]
        };
        this.charts.memory.setOption(option);
    }

    
    // 初始化磁盘IO图表
    initDiskIOCharts(diskPartitions) {
        if (!diskPartitions || diskPartitions.length === 0) return;

        // 存储磁盘信息供后续使用
        this.currentDiskInfo = diskPartitions;

        // 更新磁盘显示
        // this.updateDiskDisplay(diskPartitions);
    }
    // 根据IO速度获取颜色
    getDiskIOColor(speed, type) {
        if (type === 'read') {
            // 读取速度颜色阈值
            if (speed > 8) return '#dc2626'; // 红色 - 危险
            if (speed > 5) return '#d97706'; // 橙色 - 警告
            return '#3b82f6'; // 蓝色 - 正常
        } else {
            // 写入速度颜色阈值
            if (speed > 4) return '#dc2626'; // 红色 - 危险
            if (speed > 2) return '#d97706'; // 橙色 - 警告
            return '#8b5cf6'; // 紫色 - 正常
        }
    }



    // 更新磁盘IO状态指示器
    updateDiskIOStatus(diskInfo) {
        if (!diskInfo || diskInfo.length === 0) return;

        // 检查是否有任何磁盘IO过高
        let hasCritical = false;
        let hasWarning = false;

        diskInfo.forEach(disk => {
            if (disk.read_speed > 8 || disk.write_speed > 4) {
                hasCritical = true;
            } else if (disk.read_speed > 5 || disk.write_speed > 2) {
                hasWarning = true;
            }
        });

        // 注意：这里不再更新diskIOStatus元素，因为该元素已被移除
        // 状态更新由updateDiskOverallStatus方法统一处理
    }


  
    // 更新整体磁盘状态指示器
    updateDiskOverallStatus(diskInfo) {
        if (!diskInfo || diskInfo.length === 0) return;

        const diskOverallStatusElement = document.getElementById('diskOverallStatus');
        if (!diskOverallStatusElement) return;

        // 检查磁盘使用率和IO状态
        let hasCritical = false;
        let hasWarning = false;

        // 检查磁盘使用率
        diskInfo.forEach(disk => {
            const usagePercent = Math.round((disk.used_space / disk.total_size) * 100);
            if (usagePercent > 90) {
                hasCritical = true;
            } else if (usagePercent > 80) {
                hasWarning = true;
            }
        });

        // 如果使用率没有问题，检查IO状态
        if (!hasCritical && !hasWarning) {
            diskInfo.forEach(disk => {
                if (disk.read_speed > 8 || disk.write_speed > 4) {
                    hasCritical = true;
                } else if (disk.read_speed > 5 || disk.write_speed > 2) {
                    hasWarning = true;
                }
            });
        }

        if (hasCritical) {
            diskOverallStatusElement.textContent = '危险';
            diskOverallStatusElement.className = 'status-critical text-sm font-medium pulse-animation';
        } else if (hasWarning) {
            diskOverallStatusElement.textContent = '警告';
            diskOverallStatusElement.className = 'status-warning text-sm font-medium pulse-animation';
        } else {
            diskOverallStatusElement.textContent = '良好';
            diskOverallStatusElement.className = 'status-good text-sm font-medium';
        }
    }
    // 初始化性能趋势图表
    initPerformanceChart() {
        const option = {
            tooltip: {
                trigger: 'axis',
                axisPointer: {
                    type: 'cross',
                    label: {
                        backgroundColor: '#6a7985'
                    }
                },
                formatter: (params) => {
                    let tooltipText = `${params[0].axisValueLabel}<br/>`;
                    params.forEach(param => {
                        tooltipText += `<div class="flex items-center">
                        <div class="w-3 h-3 rounded-full mr-2" style="background-color: ${param.color}"></div>
                        ${param.seriesName}: ${param.value}%
                    </div>`;
                    });
                    return tooltipText;
                }
            },
            legend: {
                data: ['CPU使用率', '内存使用率', '磁盘IO'],
                top: '5%'
            },
            grid: {
                left: '3%',
                right: '4%',
                bottom: '15%',
                top: '20%',
                containLabel: true
            },
            xAxis: {
                type: 'category',
                boundaryGap: false,
                data: [], // 初始为空
                axisLabel: {
                    fontSize: 10,
                    rotate: 45
                }
            },
            yAxis: {
                type: 'value',
                axisLabel: {
                    formatter: '{value}%',
                    fontSize: 10
                },
                splitLine: {
                    lineStyle: {
                        type: 'dashed'
                    }
                }
            },
            dataZoom: [
                {
                    type: 'inside',
                    start: 0,
                    end: 100
                },
                {
                    type: 'slider',
                    start: 0,
                    end: 100,
                    height: 15,
                    bottom: 10
                }
            ],
            series: [
                {
                    name: 'CPU使用率',
                    type: 'line',
                    smooth: true,
                    data: [], // 初始为空
                    itemStyle: { color: '#059669' },
                    areaStyle: {
                        color: 'rgba(5, 150, 105, 0.1)',
                        opacity: 0.7
                    },
                    showSymbol: false,
                    emphasis: {
                        focus: 'series'
                    }
                },
                {
                    name: '内存使用率',
                    type: 'line',
                    smooth: true,
                    data: [], // 初始为空
                    itemStyle: { color: '#d97706' },
                    areaStyle: {
                        color: 'rgba(217, 119, 6, 0.1)',
                        opacity: 0.7
                    },
                    showSymbol: false,
                    emphasis: {
                        focus: 'series'
                    }
                },
                {
                    name: '磁盘IO',
                    type: 'line',
                    smooth: true,
                    data: [], // 初始为空
                    itemStyle: { color: '#3b82f6' },
                    areaStyle: {
                        color: 'rgba(59, 130, 246, 0.1)',
                        opacity: 0.7
                    },
                    showSymbol: false,
                    emphasis: {
                        focus: 'series'
                    }
                }
            ]
        };
        this.charts.performance.setOption(option);
    }
    // 开始实时更新
    startRealTimeUpdates() {
        setInterval(() => {
            this.updateRealTimeData();
        }, 1000);

        // 根据时间范围设置不同的更新频率
        const updateInterval = setInterval(() => {
            this.updatePerformanceChartData();
        }, this.getUpdateInterval());

        // 保存interval ID以便后续清理
        this.performanceUpdateInterval = updateInterval;
    }
    // 获取更新间隔（毫秒）
    getUpdateInterval() {
        switch (this.performanceTimeRange) {
            case '1h':
                return 60000; // 1分钟
            case '6h':
                return 300000; // 5分钟
            case '24h':
                return 900000; // 15分钟
            case '7d':
                return 7200000; // 2小时
            default:
                return 60000;
        }
    }
    // 更新实时数据
    async updateRealTimeData() {
        try {
            // 获取CPU使用率数据
            const cpuUsageData = await this.apiClient.get('/api/cpu/usage');

            // 获取系统信息数据
            const systemInfo = await this.apiClient.get('/api/system/info');

            // 更新CPU使用率显示
            this.updateCPUUsage(cpuUsageData);

            // 更新内存数据（从systemInfo获取）
            if (systemInfo.memory) {
                const memoryUsage = systemInfo.memory.usage;
                const memoryUsed = systemInfo.memory.used;
                const memoryTotal = systemInfo.memory.total;
                document.getElementById('memoryUsage').textContent = memoryUsage.toFixed(1) + '%';
                document.getElementById('memoryUsed').textContent = memoryUsed + '/';
                document.getElementById('memoryTotal').textContent = memoryTotal;

                this.charts.memory.setOption({
                    series: [{
                        data: [{
                            value: memoryUsage,
                            itemStyle: { color: memoryUsage > 90 ? '#dc2626' : memoryUsage > 75 ? '#d97706' : '#059669' }
                        }]
                    }]
                });
            }

            // 更新状态指示器
            this.updateStatusIndicators(cpuUsage, systemInfo.memory?.usage || 0);

            // 更新运行时间
            if (systemInfo.uptime) {
                this.updateUptimeFromServer(systemInfo.uptime);
            }

            // 更新磁盘IO信息
            const diskInfo = await this.apiClient.get('/api/disk/info');
            this.updateDiskIOInfo(diskInfo.partitions); // 根据实际返回结构调整

        } catch (error) {
            console.error('更新实时数据失败:', error);
        }
    }

    // 更新磁盘IO信息显示
    updateDiskIOInfo(diskInfo) {
        if (!diskInfo || diskInfo.length === 0) return;

        // 存储磁盘信息供后续使用
        this.currentDiskInfo = diskInfo;

        // 更新磁盘显示
        // this.updateDiskDisplay(diskInfo);

        // 更新整体磁盘状态指示器
        this.updateDiskOverallStatus(diskInfo);
    }

    // 更新磁盘显示（整合使用率和IO信息）
    updateDiskDisplay(diskInfo) {
        if (!diskInfo || diskInfo.length === 0) return;

        const container = document.getElementById('diskContainer');
        container.innerHTML = ''; // 清空现有内容

        diskInfo.forEach(disk => {
            // 计算使用率百分比
            const usagePercent = Math.round((disk.used_space / disk.total_size) * 100);

            // 确定使用率状态颜色
            let usageStatusColor = 'bg-green-500';
            if (usagePercent > 90) {
                usageStatusColor = 'bg-red-500';
            } else if (usagePercent > 80) {
                usageStatusColor = 'bg-yellow-500';
            }

            // 确定IO状态颜色
            const readColor = this.getDiskIOColor(disk.read_speed, 'read');
            const writeColor = this.getDiskIOColor(disk.write_speed, 'write');

            // 创建磁盘信息元素
            const diskElement = document.createElement('div');
            diskElement.className = 'bg-gray-50 rounded-lg p-4';
            diskElement.innerHTML = `
            <div class="flex justify-between items-start mb-3">
                <h4 class="font-medium text-gray-800">${disk.drive_letter}: 盘</h4>
                <span class="text-sm font-medium ${usagePercent > 90 ? 'text-red-600' : usagePercent > 80 ? 'text-yellow-600' : 'text-green-600'}">
                    ${usagePercent}%
                </span>
            </div>
            
            <!-- 使用率进度条 -->
            <div class="w-full bg-gray-200 rounded-full h-2 mb-4">
                <div class="h-2 rounded-full ${usageStatusColor}" style="width: ${usagePercent}%"></div>
            </div>
            
            <!-- 空间信息 -->
            <div class="text-xs text-gray-600 mb-4">
                ${this.formatBytes(disk.used_space)} / ${this.formatBytes(disk.total_size)} 已使用
            </div>
            
            <!-- IO信息 -->
            <div class="grid grid-cols-2 gap-3">
                <div class="bg-white rounded-lg p-3">
                    <div class="flex items-center justify-between mb-2">
                        <span class="text-xs text-gray-500">读取</span>
                        <div class="w-3 h-3 rounded-full" style="background-color: ${readColor}"></div>
                    </div>
                    <div class="text-lg font-medium">${disk.read_speed.toFixed(1)} <span class="text-xs">MB/s</span></div>
                </div>
                
                <div class="bg-white rounded-lg p-3">
                    <div class="flex items-center justify-between mb-2">
                        <span class="text-xs text-gray-500">写入</span>
                        <div class="w-3 h-3 rounded-full" style="background-color: ${writeColor}"></div>
                    </div>
                    <div class="text-lg font-medium">${disk.write_speed.toFixed(1)} <span class="text-xs">MB/s</span></div>
                </div>
            </div>
        `;

            container.appendChild(diskElement);
        });
    }
    // 更新运行时间（从服务器获取）
    updateUptimeFromServer(uptimeSeconds) {
        const uptimeElement = document.getElementById('uptime');
        const days = Math.floor(uptimeSeconds / (24 * 3600));
        const hours = Math.floor((uptimeSeconds % (24 * 3600)) / 3600);
        const minutes = Math.floor((uptimeSeconds % 3600) / 60);
        const secs = uptimeSeconds % 60;

        uptimeElement.textContent = `${days}天 ${hours.toString().padStart(2, '0')}:${minutes.toString().padStart(2, '0')}:${secs.toString().padStart(2, '0')}`;
    }
    // 更新状态指示器
    updateStatusIndicators(cpuUsage, memoryUsage) {
        const cpuStatus = document.getElementById('cpuStatus');
        const memoryStatus = document.getElementById('memoryStatus');

        if (cpuUsage > 95) {
            cpuStatus.textContent = '危险';
            cpuStatus.className = 'status-critical text-sm font-medium pulse-animation';
        }
        else if (cpuUsage > 80) {
            cpuStatus.textContent = '警告';
            cpuStatus.className = 'status-warning text-sm font-medium pulse-animation';
        }
        else {
            cpuStatus.textContent = '良好';
            cpuStatus.className = 'status-good text-sm font-medium';
        }

        if (memoryUsage > 90) {
            memoryStatus.textContent = '危险';
            memoryStatus.className = 'status-critical text-sm font-medium pulse-animation';
        } else if (memoryUsage > 75) {
            memoryStatus.textContent = '警告';
            memoryStatus.className = 'status-warning text-sm font-medium';
        } else {
            memoryStatus.textContent = '良好';
            memoryStatus.className = 'status-good text-sm font-medium';
        }
    }

    // 更新进程列表
    async updateProcessList() {
        try {
            // 获取最新进程数据
            // this.processes = await this.apiClient.get('/processes');
        } catch (error) {
            console.error('获取进程数据失败:', error);
        }

        this.renderProcessList();
    }


    // 渲染进程列表
    renderProcessList() {
        // 先根据过滤条件筛选进程
        let filteredProcesses = this.processes;
        if (this.currentFilter === 'user') {
            filteredProcesses = this.processes.filter(p => p.user === 'user');
        } else if (this.currentFilter === 'system') {
            filteredProcesses = this.processes.filter(p => p.user === 'system');
        }

        // 根据当前排序规则排序
        filteredProcesses.sort((a, b) => {
            let aValue, bValue;

            switch (this.currentSort.field) {
                case 'name':
                    aValue = a.name.toLowerCase();
                    bValue = b.name.toLowerCase();
                    break;
                case 'pid':
                    aValue = a.pid;
                    bValue = b.pid;
                    break;
                case 'cpu':
                    aValue = a.cpu;
                    bValue = b.cpu;
                    break;
                case 'memory':
                    // 提取内存数值进行比较
                    aValue = this.parseMemoryValue(a.memory);
                    bValue = this.parseMemoryValue(b.memory);
                    break;
                default:
                    return 0;
            }

            if (aValue < bValue) {
                return this.currentSort.direction === 'asc' ? -1 : 1;
            }
            if (aValue > bValue) {
                return this.currentSort.direction === 'asc' ? 1 : -1;
            }
            return 0;
        });

        const processList = document.getElementById('processList');
        processList.innerHTML = '';

        filteredProcesses.forEach(process => {
            const row = document.createElement('tr');
            row.className = 'hover:bg-gray-50 transition-colors';

            // 根据CPU使用率确定颜色
            let cpuColorClass = 'text-gray-900';
            if (process.cpu > 20) cpuColorClass = 'text-yellow-600';
            if (process.cpu > 50) cpuColorClass = 'text-orange-600';
            if (process.cpu > 80) cpuColorClass = 'text-red-600';

            // 根据内存使用量确定颜色
            const memoryValue = this.parseMemoryValue(process.memory);
            let memoryColorClass = 'text-gray-900';
            if (memoryValue > 500) memoryColorClass = 'text-yellow-600';
            if (memoryValue > 1000) memoryColorClass = 'text-orange-600';
            if (memoryValue > 1500) memoryColorClass = 'text-red-600';

            row.innerHTML = `
            <td class="px-4 py-3 whitespace-nowrap text-sm font-medium text-gray-900">${process.name}</td>
            <td class="px-4 py-3 whitespace-nowrap text-sm text-gray-500">${process.pid}</td>
            <td class="px-4 py-3 whitespace-nowrap text-sm font-medium ${cpuColorClass}">${process.cpu}%</td>
            <td class="px-4 py-3 whitespace-nowrap text-sm font-medium ${memoryColorClass}">${process.memory}</td>
        `;

            processList.appendChild(row);
        });
    }

    // 解析内存值用于排序
    parseMemoryValue(memoryStr) {
        const match = memoryStr.match(/([\d.]+)([A-Z]+)/);
        if (!match) return 0;

        const value = parseFloat(match[1]);
        const unit = match[2];

        switch (unit) {
            case 'KB': return value / 1024;
            case 'MB': return value;
            case 'GB': return value * 1024;
            default: return value;
        }
    }

    // 更新警报列表
    async updateAlerts() {
        try {
            // 获取最新警报数据
            this.alerts = await this.apiClient.get('/alerts');
        } catch (error) {
            console.error('获取警报数据失败:', error);
        }

        const alertsList = document.getElementById('alertsList');
        alertsList.innerHTML = '';

        this.alerts.forEach(alert => {
            const alertElement = document.createElement('div');
            alertElement.className = `p-4 rounded-lg border-l-4 ${alert.level === 'critical' ? 'bg-red-50 border-red-500' :
                alert.level === 'warning' ? 'bg-yellow-50 border-yellow-500' :
                    'bg-blue-50 border-blue-500'
                }`;

            alertElement.innerHTML = `
                <div class="flex items-start space-x-3">
                    <span class="text-lg">${alert.icon}</span>
                    <div class="flex-1">
                        <h4 class="font-medium text-gray-900">${alert.title}</h4>
                        <p class="text-sm text-gray-600 mt-1">${alert.message}</p>
                        <div class="text-xs text-gray-500 mt-2">${alert.time}</div>
                    </div>
                </div>
            `;

            alertsList.appendChild(alertElement);
        });
    }

    // 更新运行时间
    updateUptime() {
        // 优先使用服务器时间，如果没有则使用本地计时
        // 这里保留原有的本地计时逻辑作为后备
        const uptimeElement = document.getElementById('uptime');
        let seconds = 5 * 24 * 3600 + 12 * 3600 + 30 * 60 + 22; // 初始值

        setInterval(() => {
            seconds++;
            const days = Math.floor(seconds / (24 * 3600));
            const hours = Math.floor((seconds % (24 * 3600)) / 3600);
            const minutes = Math.floor((seconds % 3600) / 60);
            const secs = seconds % 60;

            uptimeElement.textContent = `${days}天 ${hours.toString().padStart(2, '0')}:${minutes.toString().padStart(2, '0')}:${secs.toString().padStart(2, '0')}`;
        }, 1000);
    }

    // 更新性能图表数据
    async updatePerformanceChartData() {
        try {
            // 获取性能数据
            const performanceData = await this.apiClient.get('/system/performance/history');

            // 根据当前时间范围过滤数据
            const filteredData = this.filterPerformanceData(performanceData, this.performanceTimeRange);

            // 更新图表
            this.charts.performance.setOption({
                xAxis: {
                    data: filteredData.timestamps
                },
                series: [
                    {
                        name: 'CPU使用率',
                        data: filteredData.cpu
                    },
                    {
                        name: '内存使用率',
                        data: filteredData.memory
                    },
                    {
                        name: '磁盘IO',
                        data: filteredData.diskIO
                    }
                ]
            });

            // 更新时间信息显示
            this.updateChartTimeInfo();
        } catch (error) {
            console.error('更新性能图表失败:', error);
        }
    }

    // 根据时间范围过滤性能数据
    filterPerformanceData(data, timeRange) {
        const result = {
            timestamps: [],
            cpu: [],
            memory: [],
            diskIO: []
        };

        // 计算需要的数据点数量
        let pointsNeeded;
        switch (timeRange) {
            case '1h':
                pointsNeeded = 60; // 每分钟一个点，共60个点
                break;
            case '6h':
                pointsNeeded = 72; // 每5分钟一个点，共72个点
                break;
            case '24h':
                pointsNeeded = 96; // 每15分钟一个点，共96个点
                break;
            case '7d':
                pointsNeeded = 84; // 每2小时一个点，共84个点
                break;
            default:
                pointsNeeded = 60;
        }

        // 如果数据点少于需要的数量，直接返回所有数据
        if (data.timestamps.length <= pointsNeeded) {
            return data;
        }

        // 计算步长
        const step = Math.ceil(data.timestamps.length / pointsNeeded);

        // 从后往前取数据点（最新的数据）
        for (let i = data.timestamps.length - 1; i >= 0; i -= step) {
            result.timestamps.unshift(data.timestamps[i]);
            result.cpu.unshift(data.cpu[i]);
            result.memory.unshift(data.memory[i]);
            result.diskIO.unshift(data.diskIO[i]);

            // 如果已经收集了足够的数据点，就停止
            if (result.timestamps.length >= pointsNeeded) {
                break;
            }
        }

        return result;
    }
    // 更新图表时间信息显示
    updateChartTimeInfo() {
        const timeInfoElement = document.getElementById('chartTimeInfo');
        let timeText;

        switch (this.performanceTimeRange) {
            case '1h':
                timeText = '显示最近1小时的数据，每分钟更新';
                break;
            case '6h':
                timeText = '显示最近6小时的数据，每5分钟更新';
                break;
            case '24h':
                timeText = '显示最近24小时的数据，每15分钟更新';
                break;
            case '7d':
                timeText = '显示最近7天的数据，每2小时更新';
                break;
            default:
                timeText = '显示最近1小时的数据，每分钟更新';
        }

        timeInfoElement.textContent = timeText;
    }

    // 在 SystemMonitor 类中添加以下方法

    // 获取注册表数据
    async fetchRegistryData() {
        try {
            const root = document.getElementById('registryRoot')?.value || 'HKLM';
            const path = document.getElementById('registryPath').value;

            if (!path) {
                alert('请输入注册表路径');
                return;
            }

            const registryData = await this.apiClient.get(`/registry?root=${root}&path=${encodeURIComponent(path)}`);
            this.displayRegistryData(registryData);
        } catch (error) {
            console.error('获取注册表数据失败:', error);
            document.getElementById('registryData').innerHTML = '<p class="text-red-500">获取注册表数据失败</p>';
        }
    }

    // 显示注册表数据
    displayRegistryData(data) {
        const container = document.getElementById('registryData');

        if (!data.response || data.response.status !== 'success') {
            container.innerHTML = `<p class="text-red-500">获取数据失败: ${data.response?.errorMessage || '未知错误'}</p>`;
            return;
        }

        const response = data.response;
        let html = `
        <div class="mb-4 p-3 bg-gray-50 rounded-lg">
            <div class="font-medium">${response.hive}\\${response.keyPath}</div>
            <div class="text-sm text-gray-600">最后修改时间: ${response.lastWriteTime}</div>
        </div>
    `;

        // 显示值
        if (response.values && response.values.length > 0) {
            html += `
            <div class="mb-4">
                <h4 class="font-medium text-gray-700 mb-2">值 (${response.values.length})</h4>
                <div class="overflow-x-auto">
                    <table class="min-w-full divide-y divide-gray-200">
                        <thead class="bg-gray-50">
                            <tr>
                                <th class="px-4 py-2 text-left text-xs font-medium text-gray-500 uppercase">名称</th>
                                <th class="px-4 py-2 text-left text-xs font-medium text-gray-500 uppercase">类型</th>
                                <th class="px-4 py-2 text-left text-xs font-medium text-gray-500 uppercase">值</th>
                            </tr>
                        </thead>
                        <tbody class="bg-white divide-y divide-gray-200">
        `;

            response.values.forEach(value => {
                html += `
                <tr>
                    <td class="px-4 py-2 text-sm font-medium text-gray-900">${value.name || '(默认)'}</td>
                    <td class="px-4 py-2 text-sm text-gray-500">${value.type}</td>
                    <td class="px-4 py-2 text-sm text-gray-900">${this.formatRegistryValue(value)}</td>
                </tr>
            `;
            });

            html += `
                        </tbody>
                    </table>
                </div>
            </div>
        `;
        }

        // 显示子键
        if (response.subKeys && response.subKeys.length > 0) {
            html += `
            <div>
                <h4 class="font-medium text-gray-700 mb-2">子键 (${response.subKeys.length})</h4>
                <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-2">
        `;

            response.subKeys.forEach(subKey => {
                html += `
                <div class="p-3 bg-gray-50 rounded-lg">
                    <div class="font-medium text-gray-900">${subKey.name}</div>
                    <div class="text-xs text-gray-600">${subKey.childCount} 子键, ${subKey.valueCount} 值</div>
                </div>
            `;
            });

            html += `
                </div>
            </div>
        `;
        }

        container.innerHTML = html;
    }

    // 格式化注册表值显示
    formatRegistryValue(value) {
        if (value.type === 'REG_BINARY') {
            // 截断过长的二进制数据
            if (typeof value.value === 'string' && value.value.length > 100) {
                return value.value.substring(0, 100) + '...';
            }
            return value.value;
        }

        if (Array.isArray(value.value)) {
            return value.value.join(', ');
        }

        return value.value;
    }

    // 加载预设注册表数据
    loadRegistryData(root) {
        const paths = {
            'HKLM': 'SOFTWARE\\Microsoft\\Windows\\CurrentVersion',
            'HKCU': 'Software\\Microsoft\\Windows\\CurrentVersion',
            'HKCR': 'Applications'
        };

        document.getElementById('registryPath').value = paths[root] || '';
    }
}
// 进程筛选功能
function filterProcesses(type) {
    // 更新按钮状态
    document.querySelectorAll('[onclick^="filterProcesses"]').forEach(btn => {
        btn.className = 'text-sm px-3 py-1 bg-gray-100 rounded-lg hover:bg-gray-200 transition-colors';
    });

    const activeButton = Array.from(document.querySelectorAll('[onclick^="filterProcesses"]'))
        .find(btn => btn.textContent.toLowerCase() ===
            (type === 'all' ? '全部' : type === 'user' ? '用户' : '系统'));
    if (activeButton) {
        activeButton.className = 'text-sm px-3 py-1 bg-blue-100 text-blue-700 rounded-lg';
    }

    window.systemMonitor.currentFilter = type;
    window.systemMonitor.renderProcessList();
}

// 进程排序功能
function sortProcesses(field) {
    const monitor = window.systemMonitor;

    // 如果点击的是当前排序字段，则切换排序方向，否则默认降序
    if (monitor.currentSort.field === field) {
        monitor.currentSort.direction = monitor.currentSort.direction === 'asc' ? 'desc' : 'asc';
    } else {
        monitor.currentSort.field = field;
        monitor.currentSort.direction = 'desc';
    }

    // 更新排序图标
    const sortFields = ['name', 'pid', 'cpu', 'memory'];
    sortFields.forEach(f => {
        const element = document.getElementById(`sort-${f}`);
        if (element) {
            element.innerHTML = '';
        }
    });

    const currentSortElement = document.getElementById(`sort-${field}`);
    if (currentSortElement) {
        currentSortElement.innerHTML = monitor.currentSort.direction === 'asc' ? '↑' : '↓';
    }

    monitor.renderProcessList();
}
// 时间范围切换功能
function changeTimeRange(range) {
    // 更新按钮状态
    const buttons = ['1h', '6h', '24h', '7d'];
    buttons.forEach(btn => {
        const element = document.getElementById(`btn-${btn}`);
        if (element) {
            if (btn === range) {
                element.className = 'text-sm px-3 py-1 bg-blue-100 text-blue-700 rounded-lg';
            } else {
                element.className = 'text-sm px-3 py-1 bg-gray-100 rounded-lg hover:bg-gray-200 transition-colors';
            }
        }
    });

    // 更新时间范围并重新加载数据
    window.systemMonitor.performanceTimeRange = range;

    // 清除旧的更新间隔
    if (window.systemMonitor.performanceUpdateInterval) {
        clearInterval(window.systemMonitor.performanceUpdateInterval);
    }

    // 设置新的更新间隔
    const updateInterval = setInterval(() => {
        window.systemMonitor.updatePerformanceChartData();
    }, window.systemMonitor.getUpdateInterval());

    window.systemMonitor.performanceUpdateInterval = updateInterval;

    // 立即更新图表
    window.systemMonitor.updatePerformanceChartData();
}

// 页面加载完成后初始化
document.addEventListener('DOMContentLoaded', () => {
    window.systemMonitor = new SystemMonitor();

    // 响应式图表调整
    window.addEventListener('resize', () => {
        Object.values(window.systemMonitor.charts).forEach(chart => {
            chart.resize();
        });
    });
});



// 添加全局函数
function fetchRegistryData() {
    window.systemMonitor.fetchRegistryData();
}

function loadRegistryData(root) {
    window.systemMonitor.loadRegistryData(root);
}


// 导出到全局作用域
window.filterProcesses = filterProcesses;
window.changeTimeRange = changeTimeRange;