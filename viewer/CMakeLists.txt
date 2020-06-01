# CMakeList.txt : CMake project for enerMesh, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.10)

if (UNIX)
endif (UNIX)

string(FIND ${PROJECT_BINARY_DIR} "Release" isRelease)
string(FIND ${PROJECT_BINARY_DIR} "Debug" isDebug)

if (UNIX)
	list(APPEND CMAKE_CXX_FLAGS "-pthread -fPIC")

    if(NOT (isRelease EQUAL -1))
        set (CONFIG "WSL-GCC-Release")
    elseif(NOT (isDebug EQUAL -1))
        set (CONFIG "WSL-GCC-Debug")
    else()
        message(FATAL_ERROR "unknown PROJECT_BINARY_DIR")
    endif()
else(UNIX)
    if(NOT (isRelease EQUAL -1))
        set (CONFIG "x64-Release")
    elseif(NOT (isDebug EQUAL -1))
        set (CONFIG "x64-Debug")
    else()
        message(FATAL_ERROR "unknown PROJECT_BINARY_DIR")
    endif()
endif(UNIX)

include_directories(
include 
"../../triMesh/triMesh/include" 
"../../triMesh/stlReader/include" 
"../../VulkanQuickStart/VulkanQuickStart/include" 
"../springyHexMesh/include"
)

# Add source to this project's executable.

link_directories (
"../../triMesh/out/build/${CONFIG}/triMesh/" 
"../../triMesh/out/build/${CONFIG}/stlReader/" 
"../../VulkanQuickStart/out/build/${CONFIG}/VulkanQuickStart/" 
)

add_executable ("springyHexMeshViewer"
	"src/viewer.cxx" 
)

target_link_libraries("springyHexMeshViewer" 
	PUBLIC springyHexMeshLib 
    PUBLIC VulkanQuickStartLib
	PUBLIC triMesh
	PUBLIC stlReader 
)

# TODO: Add tests and install targets if needed.
