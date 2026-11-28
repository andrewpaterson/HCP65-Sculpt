# Linux build of TestSculptLib (BaseLib + TestLib + StandardLib + SupportLib + RasterLib + RenderLib + RasterRenderLib +
# VulkanLib + ProceduralLib + InputLib + WindowLib + WinRefLib + SceneViewLib + SculptLib + TestSculptLib).
# VulkanLib needs Vulkan's headers to build (the SDK, or libvulkan-dev); the tests draw with the rasteriser.
#
# Builds 32-bit (-m32, needs g++-multilib) by default.  Add -DBASELIB_64=ON
# for a 64-bit build, in its own build directory:
#
#   cmake -S HCP65-Sculpt/Tests/TestSculptLib -B build/TestSculptLib64 -G Ninja -DBASELIB_64=ON
#
#   cmake -S HCP65-Sculpt/Tests/TestSculptLib -B build/TestSculptLib -G Ninja -DCMAKE_BUILD_TYPE=Debug
#   cmake --build build/TestSculptLib
#   ctest --test-dir build/TestSculptLib -V
#
# Add -DBASELIB_ASAN=ON for AddressSanitizer and LeakSanitizer.
#
# Add -DBASELIB_TSAN=ON (64-bit) for ThreadSanitizer, in its own build directory.  Run it with address space
# randomisation off (setarch -R ...): ThreadSanitizer doesn't start where the kernel maps memory high.
#
# Add -DBASELIB_SIZE16=ON (32-bit only) to check the W65C816's size model on the host: size is 16 bits and lsize 32,
# and -Wconversion warns of each wider value narrowed to a size ("to 'size'" in the warning).
#
# The targets are read from TestSculptLib.sln (codaphela_solution): the libraries its GCC-x64 rows build, each one's
# sources from its .vcxproj, so Visual Studio stays the source of truth.  Building re-runs the configure step when the
# solution or a project changes.  Test directories can contain .cpp files that are test data, so never glob *.cpp to
# find sources.

cmake_minimum_required(VERSION 3.24)
project(TestSculptLib CXX)


get_filename_component(CODAPHELA_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(COMMON_DIR "${CODAPHELA_ROOT}/CommonLibraries/Libraries")
set(SCULPT_LIBRARIES_DIR "${CODAPHELA_ROOT}/HCP65-Sculpt/Libraries")

include("${COMMON_DIR}/../CMake/Codaphela.cmake")

find_path(VULKAN_INCLUDE_DIR vulkan/vulkan.h HINTS "$ENV{VULKAN_SDK}/include" REQUIRED)
codaphela_options("${COMMON_DIR}" "${SCULPT_LIBRARIES_DIR}" "${VULKAN_INCLUDE_DIR}")

codaphela_solution("${CMAKE_CURRENT_SOURCE_DIR}/TestSculptLib.sln" TESTS TestSculptLib)
