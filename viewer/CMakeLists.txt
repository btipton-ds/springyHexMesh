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
    set(GLSL_VALIDATOR "glslangValidator")
    add_definitions(-DGLFW_EXPOSE_NATIVE_X11 -D_GCC)
    set(GLFW_LIB_DIR "../../ThirdParty/glfw/src")

    if(NOT (isRelease EQUAL -1))
        set (CONFIG "WSL-GCC-Release")
    elseif(NOT (isDebug EQUAL -1))
        set (CONFIG "WSL-GCC-Debug")
    else()
        message(FATAL_ERROR "unknown PROJECT_BINARY_DIR")
    endif()
else(UNIX)

    set(GLSL_VALIDATOR "/VulkanSDK/1.2.135.0/Bin32/glslangValidator.exe")
    add_definitions(-DGLFW_EXPOSE_NATIVE_WIN32)
    set(GLFW_LIB_DIR "../../ThirdParty/glfw-3.3.2/lib-vc2019")

    if(NOT (isRelease EQUAL -1))
        set (CONFIG "x64-Release")
        set(FREE_TYPE_LIB_DER "../../ThirdParty/freetype-2.10.2/x64/Release Static")
    elseif(NOT (isDebug EQUAL -1))
        set (CONFIG "x64-Debug")
        set(FREE_TYPE_LIB_DER "../../ThirdParty/freetype-2.10.2/x64/Debug Static")
    else()
        message(FATAL_ERROR "unknown PROJECT_BINARY_DIR")
    endif()
endif(UNIX)

link_directories (
"${GLFW_LIB_DIR}"
"${FREE_TYPE_LIB_DER}"
"../../triMesh/out/build/${CONFIG}/triMesh/"
"../../triMesh/out/build/${CONFIG}/stlReader/"
)

include_directories(
include 
"../../triMesh/triMesh/include" 
"../../triMesh/stlReader/include" 
"../../VulkanQuickStart/VulkanQuickStart/include" 
"../springyHexMesh/include"
"../../ThirdParty/glm-0.9.9.5/glm"
"../../ThirdParty/glfw-3.3.2/include"
"/VulkanSDK/1.2.135.0/Include"
)

# Add source to this project's executable.

link_directories (
"../../triMesh/out/build/${CONFIG}/triMesh/" 
"../../triMesh/out/build/${CONFIG}/stlReader/" 
"../../VulkanQuickStart/out/build/${CONFIG}/VulkanQuickStart/" 
)

add_library ("viewer"
 "src/hm_ui_root.cpp"
   "src/hm_ui_grid.cpp" "src/hm_ui_modelEdges.cpp")

add_executable ("springyHexMeshViewer"
 "src/viewer.cxx" 
   "src/hm_ui_grid.cpp")

find_package(Vulkan)

target_link_libraries("springyHexMeshViewer" 
	PUBLIC springyHexMeshLib 
    PUBLIC viewer
    PUBLIC VulkanQuickStartLib
    PUBLIC Vulkan::Vulkan
	PUBLIC triMesh
	PUBLIC stlReader 
    PUBLIC freetype
    PUBLIC glfw3
)


set(VQS_SHADER_DIR "../../VulkanQuickStart/VulkanQuickStart")


file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${VQS_SHADER_DIR}/shaders/*.frag"
    "${VQS_SHADER_DIR}/shaders/*.vert"
    )

foreach(GLSL ${GLSL_SOURCE_FILES})
  get_filename_component(RAW_FILE_NAME ${GLSL} NAME)
  string(REPLACE "." "_" FILE_NAME "${RAW_FILE_NAME}")
  set(SPIRV "${PROJECT_BINARY_DIR}/viewer/shaders/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/viewer/shaders/"
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    Shaders 
    DEPENDS ${SPIRV_BINARY_FILES}
    )

add_dependencies(springyHexMeshViewer Shaders)
