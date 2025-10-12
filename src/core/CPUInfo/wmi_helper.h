#pragma once
#include <string>
#include "system_info.h"

namespace sysmonitor {

class WMIHelper {
public:
    static bool Initialize();
    static void Uninitialize();
    static CPUInfo GetDetailedCPUInfo();
    static std::string GetWMICPUName();
    
private:
    static bool isInitialized_;
};

} // namespace sysmonitor