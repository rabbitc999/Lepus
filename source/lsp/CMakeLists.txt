cmake_minimum_required(VERSION 3.15)
project(LepusLsp LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 共用件是本项目自带的副本：`lsp/protocol_types.hpp` 与 `source/json/`。
set(LSP_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(JSON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../source/json)

add_library(lepus_lsp STATIC macro_analyzer.cpp language_server.cpp ../macro/signature.cpp ../syntax/source_text.cpp ${JSON_DIR}/json.cpp)
target_include_directories(lepus_lsp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${LSP_LIB_DIR}
    ../../source ../../source/preprocess)
if(MSVC)
  target_compile_options(lepus_lsp PRIVATE /W4 /utf-8)
else()
  target_compile_options(lepus_lsp PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(lepus-lsp lsp_main.cpp)
target_link_libraries(lepus-lsp PRIVATE lepus_lsp)
if(MSVC)
  target_compile_options(lepus-lsp PRIVATE /utf-8)
endif()

enable_testing()
add_executable(lepus-lsp-tests macro_analyzer_test.cpp)
target_link_libraries(lepus-lsp-tests PRIVATE lepus_lsp)
if(MSVC)
  target_compile_options(lepus-lsp-tests PRIVATE /utf-8)
endif()
add_test(NAME lepus-lsp-tests COMMAND lepus-lsp-tests)
