cmake_minimum_required(VERSION 3.20)
project(SystemSnapshotTool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录 - 这是关键修改！
# 默认将源文件视为 UTF-8
if(MSVC)
    add_compile_options(/utf-8)
endif()

# 设置输出目录
if(CMAKE_CONFIGURATION_TYPES)
    # 多配置生成器（Visual Studio）
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
else()
    # 单配置生成器
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>)
endif()

if(MSVC)
    # MSVC-specific options
    add_compile_options(/W4 /Zi)
    add_link_options(/DEBUG)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wall -Wextra -Wpedantic -g -finput-charset=UTF-8)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -g -finput-charset=UTF-8)
else()
    add_compile_options(-Wall -Wextra -g)
endif()

# 可执行文件
add_executable(SnapshotTool
    src/main.cpp
    src/core/SystemSnapshot.cpp
    src/core/DataCollector.cpp
    src/core/SnapshotManager.cpp
    src/core/SnapshotComparator.cpp
    src/core/CPUInfo/cpu_monitor.cpp
    src/core/Process/process_monitor.cpp
    src/core/Disk/disk_monitor.cpp
    src/core/CPUInfo/wmi_helper.cpp
    src/server/WebServer.cpp
)

# 包含目录
target_include_directories(SnapshotTool PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(SnapshotTool PROPERTIES
    OUTPUT_NAME "SnapshotTool"
)

# 特别为Visual Studio设置调试器工作目录
if(MSVC)
    set_target_properties(SnapshotTool PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    )
endif()
# Ensure debug/release output dirs are explicit for single-config generators
set_target_properties(SnapshotTool PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)
# Windows 库链接
if(WIN32)
    target_link_libraries(SnapshotTool
        advapi32
        psapi      
        pdh        
        wtsapi32
        setupapi
        ws2_32 
        kernel32
        wbemuuid
        ole32
        oleaut32
    )

    # Make PDH optional: MinGW may not provide it or it may not be desired.
    find_library(PDH_LIB NAMES pdh)
    if(PDH_LIB)
        message(STATUS "Found PDH library: ${PDH_LIB}")
        target_link_libraries(SnapshotTool PRIVATE ${PDH_LIB})
    else()
        message(WARNING "PDH library not found. Some performance counters may be disabled. Proceeding without PDH.")
    endif()
endif()

# 获取输出目录（即 exe 所在路径）
set(OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# 构建完成后，把 ${CMAKE_SOURCE_DIR}/webclient 复制到输出目录
add_custom_command(TARGET SnapshotTool POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/webclient"
        "$<TARGET_FILE_DIR:SnapshotTool>/webclient"
    COMMENT "Copying webclient to output directory"
)

# 安装目标
install(TARGETS SnapshotTool RUNTIME DESTINATION bin)