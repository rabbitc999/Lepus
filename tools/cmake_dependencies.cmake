# 只在构建目录缓存经校验的依赖；源码与导出工程均使用同一锁定清单。
find_package(Python3 COMPONENTS Interpreter REQUIRED)
if(NOT DEFINED LEPUS_DEPENDENCY_DIR)
    set(LEPUS_DEPENDENCY_DIR "${CMAKE_BINARY_DIR}/dependencies")
endif()
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/fetch_dependencies.py"
    --dest "${LEPUS_DEPENDENCY_DIR}"
    RESULT_VARIABLE LEPUS_DEPENDENCY_STATUS)
if(NOT LEPUS_DEPENDENCY_STATUS STREQUAL "0")
    message(FATAL_ERROR "SQLite/miniz/yyjson/tomlc17 dependency download or checksum verification failed")
endif()
set(LEPUS_SQLITE_DIR "${LEPUS_DEPENDENCY_DIR}/sqlite")
set(LEPUS_MINIZ_DIR "${LEPUS_DEPENDENCY_DIR}/miniz")
set(LEPUS_YYJSON_DIR "${LEPUS_DEPENDENCY_DIR}/yyjson")
set(LEPUS_TOMLC17_DIR "${LEPUS_DEPENDENCY_DIR}/tomlc17")
if(NOT TARGET lepus_dependencies_check)
    add_custom_target(lepus_dependencies_check
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/fetch_dependencies.py"
            --dest "${LEPUS_DEPENDENCY_DIR}"
        COMMENT "Verify locked SQLite/miniz/yyjson/tomlc17 build dependencies")
endif()
