cmake_minimum_required(VERSION 3.16)
project(Scalpel LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Use the MinGW Qt kit explicitly.
set(CMAKE_PREFIX_PATH "C:/Qt/6.12.0/mingw_64" CACHE PATH "Qt6 path" FORCE)

# Qt may probe for optional Vulkan support even when the project does not use it.
set(CMAKE_DISABLE_FIND_PACKAGE_Vulkan ON)

# Include the local library folder if it exists.
if(EXISTS "${CMAKE_SOURCE_DIR}/lib")
    include_directories(${CMAKE_SOURCE_DIR}/lib)
    link_directories(${CMAKE_SOURCE_DIR}/lib)
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets PrintSupport Network Sql)

# 递归查找所有源文件
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
)

# 递归查找所有头文件
file(GLOB_RECURSE HEADERS
    "src/*.h"
)

# 递归查找所有 UI 文件
file(GLOB_RECURSE UI_FILES
    "src/*.ui"
)

# 图片/资源文件
#set(RESOURCES
#    "src/resources.qrc"
#)

add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${UI_FILES}
#    ${RESOURCES}
)


if(EXISTS "${CMAKE_SOURCE_DIR}/lib/cbw64.lib")
    target_link_libraries(${PROJECT_NAME} PRIVATE cbw64)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets
    Qt6::PrintSupport
    Qt6::Network
    Qt6::Sql
)

# Optional MSVC UTF-8 encoding.
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()
