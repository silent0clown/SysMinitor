cmake_minimum_required(VERSION 3.20)
project(SystemSnapshotTool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(/utf-8)

# 设置输出目录 - 这是关键修改！
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
    add_compile_options(/W4 /Zi)  # 添加调试信息
    add_link_options(/DEBUG)      # 生成PDB文件
else()
    add_compile_options(-Wall -Wextra -Wpedantic -g)
endif()

# 可执行文件
add_executable(SnapshotTool
    src/main.cpp
    src/core/SystemSnapshot.cpp
    src/core/DataCollector.cpp
    src/core/SnapshotManager.cpp
    src/core/SnapshotComparator.cpp
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
# Windows 库链接
if(WIN32)
    target_link_libraries(SnapshotTool
        advapi32    # 注册表操作
        psapi       # 进程信息
        pdh         # 性能数据
        ws2_32      # Winsock (HTTP库需要)
    )
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