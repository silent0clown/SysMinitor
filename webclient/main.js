// 系统监控数据管理
class SystemMonitor {
    constructor() {
        this.charts = {};
        this.realTimeData = {
            cpu: [],
            memory: [],
            network: []
        };
        this.alerts = [];
        this.processes = [];
        this.init();
    }

    init() {
        this.initCharts();
        this.generateMockData();
        this.startRealTimeUpdates();
        this.bindEvents();
        this.updateProcessList();
        this.updateAlerts();
        this.updateUptime();
    }

    // 初始化图表
    initCharts() {
        // CPU使用率图表
        this.charts.cpu = echarts.init(document.getElementById('cpuChart'));
        this.charts.cpu.setOption({
            series: [{
                type: 'gauge',
                startAngle: 90,
                endAngle: -270,
                pointer: { show: false },
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
                    value: 45.2,
                    name: 'CPU使用率',
                    title: { offsetCenter: ['0%', '0%'] },
                    detail: { show: false },
                    itemStyle: { color: '#059669' }
                }],
                radius: '100%'
            }]
        });

        // 内存使用率图表
        this.charts.memory = echarts.init(document.getElementById('memoryChart'));
        this.charts.memory.setOption({
            series: [{
                type: 'gauge',
                startAngle: 90,
                endAngle: -270,
                pointer: { show: false },
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
                    value: 80.0,
                    name: '内存使用率',
                    title: { offsetCenter: ['0%', '0%'] },
                    detail: { show: false },
                    itemStyle: { color: '#d97706' }
                }],
                radius: '100%'
            }]
        });

        // 网络速度图表
        this.charts.network = echarts.init(document.getElementById('networkChart'));
        this.charts.network.setOption({
            grid: { top: 10, bottom: 10, left: 10, right: 10 },
            xAxis: { type: 'category', show: false },
            yAxis: { type: 'value', show: false },
            series: [{
                type: 'line',
                data: this.generateNetworkWave(),
                smooth: true,
                symbol: 'none',
                lineStyle: { color: '#3b82f6', width: 2 },
                areaStyle: { color: 'rgba(59, 130, 246, 0.1)' }
            }]
        });

        // 性能趋势图表
        this.charts.performance = echarts.init(document.getElementById('performanceChart'));
        this.initPerformanceChart();
    }

    // 初始化性能趋势图表
    initPerformanceChart() {
        const option = {
            tooltip: {
                trigger: 'axis',
                axisPointer: { type: 'cross' }
            },
            legend: {
                data: ['CPU使用率', '内存使用率', '磁盘IO']
            },
            grid: {
                left: '3%',
                right: '4%',
                bottom: '3%',
                containLabel: true
            },
            xAxis: {
                type: 'category',
                boundaryGap: false,
                data: this.generateTimeLabels()
            },
            yAxis: {
                type: 'value',
                axisLabel: { formatter: '{value}%' }
            },
            series: [
                {
                    name: 'CPU使用率',
                    type: 'line',
                    smooth: true,
                    data: this.generatePerformanceData('cpu', 60),
                    itemStyle: { color: '#059669' },
                    areaStyle: { color: 'rgba(5, 150, 105, 0.1)' }
                },
                {
                    name: '内存使用率',
                    type: 'line',
                    smooth: true,
                    data: this.generatePerformanceData('memory', 60),
                    itemStyle: { color: '#d97706' },
                    areaStyle: { color: 'rgba(217, 119, 6, 0.1)' }
                },
                {
                    name: '磁盘IO',
                    type: 'line',
                    smooth: true,
                    data: this.generatePerformanceData('disk', 60),
                    itemStyle: { color: '#3b82f6' },
                    areaStyle: { color: 'rgba(59, 130, 246, 0.1)' }
                }
            ]
        };
        this.charts.performance.setOption(option);
    }

    // 生成模拟数据
    generateMockData() {
        // 生成进程数据
        this.processes = [
            { pid: 1234, name: 'chrome.exe', cpu: 12.5, memory: '1.2GB', user: 'user', status: 'normal' },
            { pid: 5678, name: 'vscode.exe', cpu: 8.3, memory: '800MB', user: 'user', status: 'normal' },
            { pid: 9012, name: 'explorer.exe', cpu: 2.1, memory: '150MB', user: 'system', status: 'normal' },
            { pid: 3456, name: 'spotify.exe', cpu: 5.7, memory: '350MB', user: 'user', status: 'normal' },
            { pid: 7890, name: 'teams.exe', cpu: 15.2, memory: '900MB', user: 'user', status: 'warning' },
            { pid: 2468, name: 'svchost.exe', cpu: 1.8, memory: '45MB', user: 'system', status: 'normal' },
            { pid: 1357, name: 'memory_leak_app.exe', cpu: 25.3, memory: '2.1GB', user: 'user', status: 'critical' },
            { pid: 8642, name: 'discord.exe', cpu: 3.4, memory: '280MB', user: 'user', status: 'normal' }
        ];

        // 生成警报数据
        this.alerts = [
            {
                level: 'warning',
                title: '内存使用率过高',
                message: '当前内存使用率达到80%，建议关闭不必要的应用程序',
                time: new Date().toLocaleTimeString(),
                icon: '⚠️'
            },
            {
                level: 'critical',
                title: '检测到内存泄漏',
                message: 'memory_leak_app.exe 进程内存持续增长，可能存在内存泄漏',
                time: new Date(Date.now() - 300000).toLocaleTimeString(),
                icon: '🚨'
            },
            {
                level: 'info',
                title: '系统更新可用',
                message: '发现新的Windows更新，建议及时安装以保持系统安全',
                time: new Date(Date.now() - 600000).toLocaleTimeString(),
                icon: 'ℹ️'
            }
        ];
    }

    // 生成网络波动数据
    generateNetworkWave() {
        const data = [];
        for (let i = 0; i < 20; i++) {
            data.push(Math.sin(i * 0.3) * 20 + Math.random() * 10);
        }
        return data;
    }

    // 生成时间标签
    generateTimeLabels() {
        const labels = [];
        const now = new Date();
        for (let i = 59; i >= 0; i--) {
            const time = new Date(now.getTime() - i * 60000);
            labels.push(time.toLocaleTimeString('zh-CN', { hour: '2-digit', minute: '2-digit' }));
        }
        return labels;
    }

    // 生成性能数据
    generatePerformanceData(type, count) {
        const data = [];
        let baseValue = type === 'cpu' ? 45 : type === 'memory' ? 80 : 30;
        for (let i = 0; i < count; i++) {
            baseValue += (Math.random() - 0.5) * 10;
            baseValue = Math.max(0, Math.min(100, baseValue));
            data.push(Math.round(baseValue * 10) / 10);
        }
        return data;
    }

    // 开始实时更新
    startRealTimeUpdates() {
        setInterval(() => {
            this.updateRealTimeData();
        }, 1000);
    }

    // 更新实时数据
    updateRealTimeData() {
        // 更新CPU数据
        const cpuVariation = (Math.random() - 0.5) * 5;
        let cpuUsage = parseFloat(document.getElementById('cpuUsage').textContent);
        cpuUsage = Math.max(0, Math.min(100, cpuUsage + cpuVariation));
        document.getElementById('cpuUsage').textContent = cpuUsage.toFixed(1) + '%';
        
        this.charts.cpu.setOption({
            series: [{
                data: [{
                    value: cpuUsage,
                    itemStyle: { color: cpuUsage > 80 ? '#dc2626' : cpuUsage > 60 ? '#d97706' : '#059669' }
                }]
            }]
        });

        // 更新内存数据
        const memoryVariation = (Math.random() - 0.5) * 2;
        let memoryUsage = parseFloat(document.getElementById('memoryUsage').textContent);
        memoryUsage = Math.max(0, Math.min(100, memoryUsage + memoryVariation));
        document.getElementById('memoryUsage').textContent = memoryUsage.toFixed(1) + '%';
        
        this.charts.memory.setOption({
            series: [{
                data: [{
                    value: memoryUsage,
                    itemStyle: { color: memoryUsage > 90 ? '#dc2626' : memoryUsage > 75 ? '#d97706' : '#059669' }
                }]
            }]
        });

        // 更新网络数据
        const uploadVariation = (Math.random() - 0.5) * 5;
        const downloadVariation = (Math.random() - 0.5) * 20;
        let uploadSpeed = parseFloat(document.getElementById('uploadSpeed').textContent);
        let downloadSpeed = parseFloat(document.getElementById('downloadSpeed').textContent);
        
        uploadSpeed = Math.max(0, uploadSpeed + uploadVariation);
        downloadSpeed = Math.max(0, downloadSpeed + downloadVariation);
        
        document.getElementById('uploadSpeed').textContent = uploadSpeed.toFixed(1) + ' Mbps';
        document.getElementById('downloadSpeed').textContent = downloadSpeed.toFixed(1) + ' Mbps';

        // 更新状态指示器
        this.updateStatusIndicators(cpuUsage, memoryUsage);
    }

    // 更新状态指示器
    updateStatusIndicators(cpuUsage, memoryUsage) {
        const cpuStatus = document.getElementById('cpuStatus');
        const memoryStatus = document.getElementById('memoryStatus');
        
        if (cpuUsage > 80) {
            cpuStatus.textContent = '警告';
            cpuStatus.className = 'status-warning text-sm font-medium pulse-animation';
        } else if (cpuUsage > 95) {
            cpuStatus.textContent = '危险';
            cpuStatus.className = 'status-critical text-sm font-medium pulse-animation';
        } else {
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
    updateProcessList() {
        const processList = document.getElementById('processList');
        processList.innerHTML = '';

        this.processes.forEach(process => {
            const processElement = document.createElement('div');
            processElement.className = 'flex items-center justify-between p-3 bg-gray-50 rounded-lg hover:bg-gray-100 transition-colors';
            
            const statusColor = process.status === 'critical' ? 'text-red-600' : 
                               process.status === 'warning' ? 'text-yellow-600' : 'text-green-600';
            
            processElement.innerHTML = `
                <div class="flex items-center space-x-3">
                    <div class="w-2 h-2 rounded-full ${process.status === 'critical' ? 'bg-red-500' : 
                        process.status === 'warning' ? 'bg-yellow-500' : 'bg-green-500'}"></div>
                    <div>
                        <div class="font-medium text-gray-900">${process.name}</div>
                        <div class="text-sm text-gray-500">PID: ${process.pid} • ${process.user}</div>
                    </div>
                </div>
                <div class="text-right">
                    <div class="font-medium ${statusColor}">${process.cpu}%</div>
                    <div class="text-sm text-gray-500">${process.memory}</div>
                </div>
            `;
            
            processList.appendChild(processElement);
        });
    }

    // 更新警报列表
    updateAlerts() {
        const alertsList = document.getElementById('alertsList');
        alertsList.innerHTML = '';

        this.alerts.forEach(alert => {
            const alertElement = document.createElement('div');
            alertElement.className = `p-4 rounded-lg border-l-4 ${
                alert.level === 'critical' ? 'bg-red-50 border-red-500' :
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

    // 绑定事件
    bindEvents() {
        // 系统体检按钮
        document.getElementById('healthCheckBtn').addEventListener('click', () => {
            this.showHealthCheckModal();
        });

        // 生成快照按钮
        document.getElementById('createSnapshotBtn').addEventListener('click', () => {
            this.showSnapshotModal();
        });

        // 对比分析按钮
        document.getElementById('compareBtn').addEventListener('click', () => {
            window.location.href = 'snapshot.html';
        });

        // 关闭模态框
        document.getElementById('closeHealthModal').addEventListener('click', () => {
            document.getElementById('healthCheckModal').classList.add('hidden');
        });

        document.getElementById('closeSnapshotModal').addEventListener('click', () => {
            document.getElementById('snapshotModal').classList.add('hidden');
        });
    }

    // 显示系统体检模态框
    showHealthCheckModal() {
        const modal = document.getElementById('healthCheckModal');
        const progress = document.getElementById('healthProgress');
        const status = document.getElementById('healthStatus');
        
        modal.classList.remove('hidden');
        
        let progressValue = 0;
        const intervals = [
            { progress: 20, status: '正在检查CPU性能...', delay: 500 },
            { progress: 40, status: '正在分析内存使用...', delay: 800 },
            { progress: 60, status: '正在检查磁盘状态...', delay: 600 },
            { progress: 80, status: '正在扫描注册表...', delay: 700 },
            { progress: 100, status: '检查完成！', delay: 500 }
        ];
        
        intervals.forEach((interval, index) => {
            setTimeout(() => {
                progress.style.width = interval.progress + '%';
                status.textContent = interval.status;
                
                if (index === intervals.length - 1) {
                    setTimeout(() => {
                        modal.classList.add('hidden');
                        this.showHealthReport();
                    }, 1000);
                }
            }, interval.delay * (index + 1));
        });
    }

    // 显示健康报告
    showHealthReport() {
        const healthScore = Math.floor(Math.random() * 30) + 60; // 60-90分
        alert(`系统健康检查完成！\n\n健康得分: ${healthScore}/100\n\n详细报告请查看"报告分析"页面。`);
    }

    // 显示快照模态框
    showSnapshotModal() {
        const modal = document.getElementById('snapshotModal');
        const progress = document.getElementById('snapshotProgress');
        const status = document.getElementById('snapshotStatus');
        
        modal.classList.remove('hidden');
        
        let progressValue = 0;
        const intervals = [
            { progress: 25, status: '正在采集系统信息...', delay: 600 },
            { progress: 50, status: '正在扫描进程列表...', delay: 800 },
            { progress: 75, status: '正在读取注册表...', delay: 700 },
            { progress: 100, status: '快照生成完成！', delay: 600 }
        ];
        
        intervals.forEach((interval, index) => {
            setTimeout(() => {
                progress.style.width = interval.progress + '%';
                status.textContent = interval.status;
                
                if (index === intervals.length - 1) {
                    setTimeout(() => {
                        modal.classList.add('hidden');
                        this.showSnapshotSuccess();
                    }, 1000);
                }
            }, interval.delay * (index + 1));
        });
    }

    // 显示快照成功信息
    showSnapshotSuccess() {
        const timestamp = new Date().toLocaleString('zh-CN');
        alert(`系统快照创建成功！\n\n快照名称: 系统快照_${new Date().toISOString().slice(0, 19).replace(/:/g, '')}\n创建时间: ${timestamp}\n文件大小: 2.3MB\n\n快照已保存到本地，可在"快照管理"页面查看。`);
    }
}

// 进程筛选功能
function filterProcesses(type) {
    // 更新按钮状态
    document.querySelectorAll('[onclick^="filterProcesses"]').forEach(btn => {
        btn.className = 'text-sm px-3 py-1 bg-gray-100 rounded-lg hover:bg-gray-200 transition-colors';
    });
    event.target.className = 'text-sm px-3 py-1 bg-blue-100 text-blue-700 rounded-lg';
    
    // 这里可以实现实际的进程筛选逻辑
    console.log('筛选进程类型:', type);
}

// 时间范围切换功能
function changeTimeRange(range) {
    // 更新按钮状态
    document.querySelectorAll('[onclick^="changeTimeRange"]').forEach(btn => {
        btn.className = 'text-sm px-3 py-1 bg-gray-100 rounded-lg hover:bg-gray-200 transition-colors';
    });
    event.target.className = 'text-sm px-3 py-1 bg-blue-100 text-blue-700 rounded-lg';
    
    // 这里可以实现实际的时间范围切换逻辑
    console.log('切换时间范围:', range);
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

// 导出到全局作用域
window.filterProcesses = filterProcesses;
window.changeTimeRange = changeTimeRange;