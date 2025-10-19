#include <windows.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <cstdint>

#define GET_LOCAL_TIME_MS() get_windows_time_since_epoch()

// 返回64位时间戳，需按照毫秒精度转换
// Convert Windows FILETIME to milliseconds since 1970-01-01
inline uint64_t get_windows_time_since_epoch() {
    const uint64_t EPOCH_DIFFERENCE = 116444736000000000ULL; // 100-ns intervals from 1601 to 1970
    
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    
    ULARGE_INTEGER ull;
    ull.LowPart = ft.dwLowDateTime;
    ull.HighPart = ft.dwHighDateTime;
    
    return (ull.QuadPart - EPOCH_DIFFERENCE) / 10000; // Convert to milliseconds
}