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
)

# Add source to this project's executable.
add_library (
springyHexMeshLib
	"src/meshProcessor.cpp"
	"src/hm_paramsRec.cpp"
	"src/hm_gridVert.cpp"
	"src/hm_gridCell.cpp"
	"src/hm_gridBase.cpp"
	"src/hm_gridEdge.cpp" 
	"src/hm_model.cpp"   
	"src/hm_polylineFitter.cpp" 
	"src/hm_gridCellEnergy.cpp" 
	"src/hm_dump.cpp" 
	"src/hm_gridFace.cpp" 
	"src/hm_splitter.cpp" 
	"src/hm_grid.cpp" 
	"src/hm_topolRef.cpp" 
	"src/hm_gridIO_Read_v_1.cpp" 
	"src/hm_types.cpp"
)

link_directories (
"../../triMesh/out/build/${CONFIG}/triMesh/" 
"../../triMesh/out/build/${CONFIG}/stlReader/" 
)

target_link_libraries("springyHexMeshLib" 
	PUBLIC triMesh
	PUBLIC stlReader 
)

add_executable ("springyHexMesh"
	"src/springyHexMesh.cxx" 
)

target_link_libraries("springyHexMesh" 
	PUBLIC triMesh
	PUBLIC stlReader 
	PUBLIC springyHexMeshLib 
)

# TODO: Add tests and install targets if needed.
