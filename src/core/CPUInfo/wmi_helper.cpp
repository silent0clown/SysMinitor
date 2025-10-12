#include "wmi_helper.h"
#include <windows.h>
#include <wbemidl.h>
#include <iostream>

#pragma comment(lib, "wbemuuid.lib")

// Helper: convert BSTR to UTF-8 std::string without using _bstr_t/comsupp
static std::string BSTRToUtf8(BSTR bstr) {
    if (!bstr) return std::string();
    const wchar_t* wstr = static_cast<const wchar_t*>(bstr);
    if (!wstr) return std::string();
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    if (len <= 0) return std::string();
    std::string out(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &out[0], len, NULL, NULL);
    if (!out.empty() && out.back() == '\0') out.pop_back();
    return out;
}

namespace sysmonitor {

bool WMIHelper::isInitialized_ = false;

bool WMIHelper::Initialize() {
    if (isInitialized_) return true;
    
    HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) {
        return false;
    }
    
    hres = CoInitializeSecurity(
        NULL, -1, NULL, NULL,
        RPC_C_AUTHN_LEVEL_NONE,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL, EOAC_NONE, NULL
    );
    
    isInitialized_ = SUCCEEDED(hres);
    return isInitialized_;
}

void WMIHelper::Uninitialize() {
    if (isInitialized_) {
        CoUninitialize();
        isInitialized_ = false;
    }
}

CPUInfo WMIHelper::GetDetailedCPUInfo() {
    CPUInfo info;
    
    if (!Initialize()) {
        return info;
    }
    
    IWbemLocator* pLoc = nullptr;
    IWbemServices* pSvc = nullptr;
    
    HRESULT hres = CoCreateInstance(
        CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID*)&pLoc
    );
    
    if (SUCCEEDED(hres)) {
        BSTR ns = SysAllocString(L"ROOT\\CIMV2");
        hres = pLoc->ConnectServer(ns, NULL, NULL, 0, NULL, 0, 0, &pSvc);
        SysFreeString(ns);
        
        if (SUCCEEDED(hres)) {
            hres = CoSetProxyBlanket(
                pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                NULL, RPC_C_AUTHN_LEVEL_CALL,
                RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE
            );
            
            if (SUCCEEDED(hres)) {
                IEnumWbemClassObject* pEnumerator = nullptr;
                BSTR lang = SysAllocString(L"WQL");
                BSTR query = SysAllocString(L"SELECT * FROM Win32_Processor");
                hres = pSvc->ExecQuery(lang, query,
                    WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                    NULL, &pEnumerator
                );
                SysFreeString(lang);
                SysFreeString(query);
                
                if (SUCCEEDED(hres)) {
                    IWbemClassObject* pclsObj = nullptr;
                    ULONG uReturn = 0;
                    
                    while (pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn) == S_OK) {
                        VARIANT vtProp;
                        
                        if (pclsObj->Get(L"Name", 0, &vtProp, 0, 0) == S_OK) {
                            info.name = BSTRToUtf8(vtProp.bstrVal);
                            VariantClear(&vtProp);
                        }
                        
                        if (pclsObj->Get(L"NumberOfCores", 0, &vtProp, 0, 0) == S_OK) {
                            info.physicalCores = vtProp.uintVal;
                            VariantClear(&vtProp);
                        }
                        
                        if (pclsObj->Get(L"NumberOfLogicalProcessors", 0, &vtProp, 0, 0) == S_OK) {
                            info.logicalCores = vtProp.uintVal;
                            VariantClear(&vtProp);
                        }
                        
                        if (pclsObj->Get(L"MaxClockSpeed", 0, &vtProp, 0, 0) == S_OK) {
                            info.maxFrequency = vtProp.uintVal;
                            VariantClear(&vtProp);
                        }
                        
                        pclsObj->Release();
                        break; // 只取第一个处理器
                    }
                    
                    pEnumerator->Release();
                }
            }
            pSvc->Release();
        }
        pLoc->Release();
    }
    
    return info;
}

} // namespace sysmonitor