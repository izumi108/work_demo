# ui/CMakeLists.txt
file(GLOB_RECURSE UI_SOURCES
    "*.cpp"
    "*.h"
    "*.ui"
#    "Entities/*.cpp"
#    "Entities/*.h"
#    "Daos/*.cpp"
#    "Daos/*.h"
)


#[[ # 检查 CMake 版本并选择排除方法
if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.20")
    list(FILTER UI_SOURCES EXCLUDE REGEX "Text1\\.(ui|cpp|h)$")
    list(FILTER UI_SOURCES EXCLUDE REGEX "Text2\\.(ui|cpp|h)$")
else()
    list(REMOVE_ITEM UI_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/Text1.ui"
        "${CMAKE_CURRENT_SOURCE_DIR}/Text1.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Text2.ui"
        "${CMAKE_CURRENT_SOURCE_DIR}/Text2.cpp"
    )
endif()
]]

set(UI_SOURCES ${UI_SOURCES} PARENT_SCOPE)
