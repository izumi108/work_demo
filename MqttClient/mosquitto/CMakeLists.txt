file(GLOB_RECURSE MOSQUITTO_SOURCES
    "*.h"
    "*.cpp"
)

# 检查 CMake 版本并选择排除方法
#if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.20")
#    list(FILTER MOSQUITTO_SOURCES EXCLUDE REGEX "resource\\.qrc$")
#else()
#    list(REMOVE_ITEM MOSQUITTO_SOURCES
#        "${CMAKE_CURRENT_SOURCE_DIR}/resource.qrc"
#    )
#endif()

set(MOSQUITTO_SOURCES ${MOSQUITTO_SOURCES} PARENT_SCOPE)
