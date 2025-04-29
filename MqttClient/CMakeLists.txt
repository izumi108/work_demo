cmake_minimum_required(VERSION 3.10)
project(MqttClient VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找Qt核心组件
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network Widgets)

#set(TS_FILES MqttClient_zh_CN.ts)

# set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/../MqttClient_build)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

# 添加子模块
add_subdirectory(ui)
add_subdirectory(mosquitto)

set(PROJECT_SOURCES
    main.cpp
    ${UI_SOURCES}
    ${MOSQUITTO_SOURCES}
)

# 查找libmosquitto库
find_package(PkgConfig REQUIRED)
pkg_check_modules(MOSQUITPO REQUIRED libmosquitto)
include_directories(${MOSQUITPO_INCLUDE_DIRS})


if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(MqttClient
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(MqttClient
        ${PROJECT_SOURCES}
    )
endif()

# 添加头文件搜索路径
target_include_directories(MqttClient PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ui,
    ${CMAKE_CURRENT_SOURCE_DIR}/mosquitto
)

# 链接Qt组件
target_link_libraries(MqttClient PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Widgets
    # 链接libmosquitto
    ${MOSQUITPO_LIBRARIES}
)

# 包含目录
target_include_directories(MqttClient PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${MOSQUITPO_INCLUDE_DIRS}
)

# 设置目标属性
set_target_properties(MqttClient PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# 安装配置
include(GNUInstallDirs)
install(TARGETS MqttClient
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(MqttClient)
endif()
