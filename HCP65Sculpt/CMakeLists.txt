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
# The targets are read from HCP65Sculpt.sln (codaphela_solution): the libraries its GCC-x64 rows build, each one's
# sources from its .vcxproj, so Visual Studio stays the source of truth.  HCP65Sculpt.vcxproj's configurations say which
# main each program has: HCP65Sculpt_Linux.cpp the X11 one's, HCP65SculptRef.cpp the reference screen's (its Reference
# configuration), whose rows build WinRefLib and no X11Lib.

cmake_minimum_required(VERSION 3.24)
project(HCP65Sculpt CXX)


# This repository is beside CommonLibraries.
get_filename_component(SCULPT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
get_filename_component(CODAPHELA_ROOT "${SCULPT_ROOT}/.." ABSOLUTE)
set(COMMON_DIR "${CODAPHELA_ROOT}/CommonLibraries/Libraries")

include("${COMMON_DIR}/../CMake/Codaphela.cmake")

find_path(VULKAN_INCLUDE_DIR vulkan/vulkan.h HINTS "$ENV{VULKAN_SDK}/include" REQUIRED)
codaphela_options(ONLY_64 "${COMMON_DIR}" "${SCULPT_ROOT}/Libraries" "${VULKAN_INCLUDE_DIR}")

codaphela_solution("${CMAKE_CURRENT_SOURCE_DIR}/HCP65Sculpt.sln"
	PROGRAMS HCP65Sculpt
	REFERENCE HCP65Sculpt=HCP65SculptRef)
