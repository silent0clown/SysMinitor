cmake_minimum_required(VERSION 3.20)
project(SystemSnapshotTool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(/utf-8)

# 设置输出�?�? - 这是关键�?改！
if(CMAKE_CONFIGURATION_TYPES)
    # 多配�?生成�?（Visual Studio�?
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
else()
    # 单配�?生成�?
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>)
endif()

if(MSVC)
    add_compile_options(/W4 /Zi)  # 添加调试信息
    add_link_options(/DEBUG)      # 生成PDB文件
    # add_compile_options("$<$<C_COMPILER_ID:MSVC>:/source-charset:GB2312>")
    # add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/source-charset:GB2312>")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    
    # 设置执�?�字符集（可选）
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/execution-charset:GB2312>")

        # ����/FSѡ����PDB�ļ����ʳ�ͻ
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/FS>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/FS>")
    
else()
    add_compile_options(-Wall -Wextra -Wpedantic -g)
endif()

# �?执�?�文�?
add_executable(SnapshotTool
    src/main.cpp
    # src/core/SystemSnapshot.cpp
    # src/core/DataCollector.cpp
    # src/core/SnapshotManager.cpp
    # src/core/SnapshotComparator.cpp
    src/core/CPUInfo/cpu_monitor.cpp
    src/core/CPUInfo/wmi_helper.cpp
    src/core/Process/process_monitor.cpp
    src/core/Disk/disk_monitor.cpp
    src/core/Register/registry_monitor.cpp
    src/core/Memory/memory_monitor.cpp
    src/core/Driver/driver_monitor.cpp
    src/server/WebServer.cpp
    src/utils/encode.cpp
    src/utils/registry_encode.cpp
)

# 包含�?�?
target_include_directories(SnapshotTool PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(SnapshotTool PROPERTIES
    OUTPUT_NAME "SnapshotTool"
)

# 特别为Visual Studio设置调试器工作目�?
if(MSVC)
    set_target_properties(SnapshotTool PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    )
endif()
# Windows 库链�?
if(WIN32)
    target_link_libraries(SnapshotTool
        advapi32    # 注册表操�?
        psapi       # 进程信息
        pdh         # 性能数据
        wtsapi32
        setupapi
        ws2_32      # Winsock (HTTP库需�?)
        kernel32
        wbemuuid
        ole32
        oleaut32
    )
endif()

# 获取输出�?录（�? exe 所在路径）
set(OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# 构建完成后，�? ${CMAKE_SOURCE_DIR}/webclient 复制到输出目�?
add_custom_command(TARGET SnapshotTool POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/webclient"
        "$<TARGET_FILE_DIR:SnapshotTool>/webclient"
    COMMENT "Copying webclient to output directory"
)

# 安�?�目�?
install(TARGETS SnapshotTool RUNTIME DESTINATION bin)