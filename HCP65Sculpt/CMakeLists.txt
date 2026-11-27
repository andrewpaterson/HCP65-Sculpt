# Linux build of HCP65 Sculpt (BaseLib + StandardLib + SupportLib + RasterLib + RenderLib + RasterRenderLib + VulkanLib +
# ProceduralLib + InputLib + WindowLib + SceneViewLib + X11Lib + WinRefLib + SculptLib + Sculpt's sources).
# VulkanLib needs Vulkan's headers to build (the SDK, or libvulkan-dev); it loads libvulkan.so.1 when a scene is shown.
#
# HCP65Sculpt shows its window on the X display; HCP65SculptRef draws it on WinRefLib's reference screen, in memory with
# nothing on any display (see HCP65SculptRef.cpp).
#
# 64-bit only: there is no 32-bit libX11 installed.  Needs libx11-dev and libxext-dev.
#
# From the folder the HCP65-Sculpt repository and CommonLibraries are in:
#
#   cmake -S HCP65-Sculpt/HCP65Sculpt -B build/HCP65Sculpt -G Ninja -DCMAKE_BUILD_TYPE=Release
#   cmake --build build/HCP65Sculpt
#   cd HCP65-Sculpt/HCP65Sculpt/HCP65Sculpt && ../../../build/HCP65Sculpt/HCP65Sculpt
#
# Add -DBASELIB_ASAN=ON for AddressSanitizer and LeakSanitizer.
#
# Source lists are read from the .vcxproj files so Visual Studio stays the source of truth.  Of HCP65Sculpt.vcxproj's
# sources HCP65Sculpt_Windows.cpp (WinMain) is Windows only; HCP65Sculpt_Linux.cpp is the X11 main and HCP65SculptRef.cpp the
# reference screen's.

cmake_minimum_required(VERSION 3.19)
project(HCP65Sculpt CXX)


# This repository is beside CommonLibraries.
get_filename_component(SCULPT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
get_filename_component(CODAPHELA_ROOT "${SCULPT_ROOT}/.." ABSOLUTE)
set(COMMON_DIR "${CODAPHELA_ROOT}/CommonLibraries/Libraries")
set(BASELIB_DIR "${COMMON_DIR}/BaseLib")
set(STANDARDLIB_DIR "${COMMON_DIR}/StandardLib")
set(SUPPORTLIB_DIR "${COMMON_DIR}/SupportLib")
set(RASTERLIB_DIR "${COMMON_DIR}/RasterLib")
set(RENDERLIB_DIR "${COMMON_DIR}/RenderLib")
set(RASTERRENDERLIB_DIR "${COMMON_DIR}/RasterRenderLib")
set(VULKANLIB_DIR "${COMMON_DIR}/VulkanLib")
set(PROCEDURALLIB_DIR "${COMMON_DIR}/ProceduralLib")
set(THREADLIB_DIR "${COMMON_DIR}/ThreadLib")
set(INPUTLIB_DIR "${COMMON_DIR}/InputLib")
set(WINDOWLIB_DIR "${COMMON_DIR}/WindowLib")
set(X11LIB_DIR "${COMMON_DIR}/X11Lib")
set(SCENEVIEWLIB_DIR "${COMMON_DIR}/SceneViewLib")
set(WINREFLIB_DIR "${COMMON_DIR}/WinRefLib")
set(SCULPTLIB_DIR "${SCULPT_ROOT}/Libraries/SculptLib")
set(SCULPT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/HCP65Sculpt")

include("${COMMON_DIR}/../CMake/Codaphela.cmake")


vcxproj_sources(BASELIB_SOURCES "${BASELIB_DIR}/BaseLib.vcxproj")
vcxproj_sources(STANDARDLIB_SOURCES "${STANDARDLIB_DIR}/StandardLib.vcxproj")
vcxproj_sources(SUPPORTLIB_SOURCES "${SUPPORTLIB_DIR}/SupportLib.vcxproj")
vcxproj_sources(RASTERLIB_SOURCES "${RASTERLIB_DIR}/RasterLib.vcxproj")
vcxproj_sources(RENDERLIB_SOURCES "${RENDERLIB_DIR}/RenderLib.vcxproj")
vcxproj_sources(RASTERRENDERLIB_SOURCES "${RASTERRENDERLIB_DIR}/RasterRenderLib.vcxproj")
vcxproj_sources(VULKANLIB_SOURCES "${VULKANLIB_DIR}/VulkanLib.vcxproj")
vcxproj_sources(PROCEDURALLIB_SOURCES "${PROCEDURALLIB_DIR}/ProceduralLib.vcxproj")
vcxproj_sources(THREADLIB_SOURCES "${THREADLIB_DIR}/ThreadLib.vcxproj")
vcxproj_sources(INPUTLIB_SOURCES "${INPUTLIB_DIR}/InputLib.vcxproj")
vcxproj_sources(WINDOWLIB_SOURCES "${WINDOWLIB_DIR}/WindowLib.vcxproj")
vcxproj_sources(X11LIB_SOURCES "${X11LIB_DIR}/X11Lib.vcxproj")
vcxproj_sources(SCENEVIEWLIB_SOURCES "${SCENEVIEWLIB_DIR}/SceneViewLib.vcxproj")
vcxproj_sources(WINREFLIB_SOURCES "${WINREFLIB_DIR}/WinRefLib.vcxproj")
vcxproj_sources(SCULPTLIB_SOURCES "${SCULPTLIB_DIR}/SculptLib.vcxproj")
vcxproj_sources(SCULPT_SOURCES "${SCULPT_DIR}/HCP65Sculpt.vcxproj" EXCLUDE "HCP65Sculpt_Windows.cpp" "HCP65Sculpt_Linux.cpp")


codaphela_options(ONLY_64 "${COMMON_DIR}" "${SCULPT_ROOT}/Libraries")


# CMutex uses pthreads on Linux.
find_package(Threads REQUIRED)
find_package(X11 REQUIRED)

add_library(BaseLib STATIC ${BASELIB_SOURCES})
target_link_libraries(BaseLib PUBLIC baselib_options Threads::Threads)

add_library(StandardLib STATIC ${STANDARDLIB_SOURCES})
target_link_libraries(StandardLib PUBLIC BaseLib)

add_library(SupportLib STATIC ${SUPPORTLIB_SOURCES})
target_link_libraries(SupportLib PUBLIC StandardLib BaseLib)

add_library(RasterLib STATIC ${RASTERLIB_SOURCES})
target_link_libraries(RasterLib PUBLIC StandardLib BaseLib)

add_library(RenderLib STATIC ${RENDERLIB_SOURCES})
target_link_libraries(RenderLib PUBLIC SupportLib StandardLib BaseLib)

add_library(RasterRenderLib STATIC ${RASTERRENDERLIB_SOURCES})
target_link_libraries(RasterRenderLib PUBLIC RenderLib RasterLib SupportLib StandardLib BaseLib)

add_library(ProceduralLib STATIC ${PROCEDURALLIB_SOURCES})
target_link_libraries(ProceduralLib PUBLIC SupportLib StandardLib BaseLib)

# The bake's rows are shared among a pool of threads (CWorkerThreadJobs).
add_library(ThreadLib STATIC ${THREADLIB_SOURCES})
target_link_libraries(ThreadLib PUBLIC BaseLib)

# Only Vulkan's headers (the SDK or libvulkan-dev): VulkanLib finds its functions in libvulkan.so.1 while it runs.
find_path(VULKAN_INCLUDE_DIR vulkan/vulkan.h HINTS "$ENV{VULKAN_SDK}/include" REQUIRED)
add_library(VulkanLib STATIC ${VULKANLIB_SOURCES})
target_include_directories(VulkanLib PUBLIC "${VULKAN_INCLUDE_DIR}")
target_link_libraries(VulkanLib PUBLIC RenderLib SupportLib StandardLib BaseLib)

add_library(InputLib STATIC ${INPUTLIB_SOURCES})
target_link_libraries(InputLib PUBLIC StandardLib BaseLib)

add_library(WindowLib STATIC ${WINDOWLIB_SOURCES})
target_link_libraries(WindowLib PUBLIC InputLib SupportLib StandardLib BaseLib)

add_library(X11Lib STATIC ${X11LIB_SOURCES})
target_link_libraries(X11Lib PUBLIC WindowLib InputLib SupportLib StandardLib BaseLib X11::X11 X11::Xext)

add_library(SceneViewLib STATIC ${SCENEVIEWLIB_SOURCES})
target_link_libraries(SceneViewLib PUBLIC WindowLib InputLib VulkanLib RasterRenderLib RenderLib RasterLib SupportLib StandardLib BaseLib)

add_library(WinRefLib STATIC ${WINREFLIB_SOURCES})
target_link_libraries(WinRefLib PUBLIC WindowLib InputLib SupportLib StandardLib BaseLib)

add_library(SculptLib STATIC ${SCULPTLIB_SOURCES})
target_link_libraries(SculptLib PUBLIC SceneViewLib WindowLib InputLib VulkanLib ProceduralLib RasterRenderLib RenderLib RasterLib SupportLib StandardLib BaseLib)

# Everything but a main.
add_library(HCP65SculptLib STATIC ${SCULPT_SOURCES})
target_include_directories(HCP65SculptLib PUBLIC "${SCULPT_DIR}")
target_link_libraries(HCP65SculptLib PUBLIC SculptLib SceneViewLib WindowLib InputLib VulkanLib ProceduralLib RasterRenderLib RenderLib RasterLib SupportLib StandardLib ThreadLib BaseLib)

add_executable(HCP65Sculpt "${SCULPT_DIR}/HCP65Sculpt_Linux.cpp")
target_link_libraries(HCP65Sculpt PRIVATE HCP65SculptLib X11Lib)

add_executable(HCP65SculptRef "${SCULPT_DIR}/HCP65SculptRef.cpp")
target_link_libraries(HCP65SculptRef PRIVATE HCP65SculptLib WinRefLib)
