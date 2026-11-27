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
# Source lists are read from the .vcxproj files so Visual Studio stays the
# source of truth.  Building re-runs the configure step automatically when a
# .vcxproj changes.  Test directories can contain .cpp files that are test data,
# so never glob *.cpp to find sources.

cmake_minimum_required(VERSION 3.19)
project(TestSculptLib CXX)


get_filename_component(CODAPHELA_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(COMMON_DIR "${CODAPHELA_ROOT}/CommonLibraries/Libraries")
set(BASELIB_DIR "${COMMON_DIR}/BaseLib")
set(TESTLIB_DIR "${COMMON_DIR}/TestLib")
set(STANDARDLIB_DIR "${COMMON_DIR}/StandardLib")
set(SUPPORTLIB_DIR "${COMMON_DIR}/SupportLib")
set(RASTERLIB_DIR "${COMMON_DIR}/RasterLib")
set(RENDERLIB_DIR "${COMMON_DIR}/RenderLib")
set(RASTERRENDERLIB_DIR "${COMMON_DIR}/RasterRenderLib")
set(VULKANLIB_DIR "${COMMON_DIR}/VulkanLib")
set(INPUTLIB_DIR "${COMMON_DIR}/InputLib")
set(WINDOWLIB_DIR "${COMMON_DIR}/WindowLib")
set(WINREFLIB_DIR "${COMMON_DIR}/WinRefLib")
set(SCENEVIEWLIB_DIR "${COMMON_DIR}/SceneViewLib")
set(PROCEDURALLIB_DIR "${COMMON_DIR}/ProceduralLib")
set(THREADLIB_DIR "${COMMON_DIR}/ThreadLib")
set(SCULPT_LIBRARIES_DIR "${CODAPHELA_ROOT}/HCP65-Sculpt/Libraries")
set(SCULPTLIB_DIR "${SCULPT_LIBRARIES_DIR}/SculptLib")
set(TESTSCULPTLIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/TestSculptLib")

include("${COMMON_DIR}/../CMake/Codaphela.cmake")


vcxproj_sources(BASELIB_SOURCES "${BASELIB_DIR}/BaseLib.vcxproj")

vcxproj_sources(TESTLIB_SOURCES "${TESTLIB_DIR}/TestLib.vcxproj")

vcxproj_sources(STANDARDLIB_SOURCES "${STANDARDLIB_DIR}/StandardLib.vcxproj")

vcxproj_sources(SUPPORTLIB_SOURCES "${SUPPORTLIB_DIR}/SupportLib.vcxproj")

vcxproj_sources(RASTERLIB_SOURCES "${RASTERLIB_DIR}/RasterLib.vcxproj")

vcxproj_sources(RENDERLIB_SOURCES "${RENDERLIB_DIR}/RenderLib.vcxproj")

vcxproj_sources(RASTERRENDERLIB_SOURCES "${RASTERRENDERLIB_DIR}/RasterRenderLib.vcxproj")

vcxproj_sources(VULKANLIB_SOURCES "${VULKANLIB_DIR}/VulkanLib.vcxproj")

vcxproj_sources(INPUTLIB_SOURCES "${INPUTLIB_DIR}/InputLib.vcxproj")

vcxproj_sources(WINDOWLIB_SOURCES "${WINDOWLIB_DIR}/WindowLib.vcxproj")

vcxproj_sources(WINREFLIB_SOURCES "${WINREFLIB_DIR}/WinRefLib.vcxproj")

vcxproj_sources(SCENEVIEWLIB_SOURCES "${SCENEVIEWLIB_DIR}/SceneViewLib.vcxproj")

vcxproj_sources(PROCEDURALLIB_SOURCES "${PROCEDURALLIB_DIR}/ProceduralLib.vcxproj")
vcxproj_sources(THREADLIB_SOURCES "${THREADLIB_DIR}/ThreadLib.vcxproj")

vcxproj_sources(SCULPTLIB_SOURCES "${SCULPTLIB_DIR}/SculptLib.vcxproj")

vcxproj_sources(TESTSCULPTLIB_SOURCES "${TESTSCULPTLIB_DIR}/TestSculptLib.vcxproj")


codaphela_options("${COMMON_DIR}" "${SCULPT_LIBRARIES_DIR}")


# CMutex uses pthreads on Linux.
find_package(Threads REQUIRED)

add_library(BaseLib STATIC ${BASELIB_SOURCES})
target_link_libraries(BaseLib PUBLIC baselib_options)
target_link_libraries(BaseLib PUBLIC Threads::Threads)

# Uses AVX intrinsics unconditionally; MSVC allows that without a flag, g++ does not.

add_library(TestLib STATIC ${TESTLIB_SOURCES})
target_link_libraries(TestLib PUBLIC BaseLib)

add_library(StandardLib STATIC ${STANDARDLIB_SOURCES})
target_link_libraries(StandardLib PUBLIC BaseLib)

# The Debug configurations of StandardLib (only) define DEBUG_POINTER.
target_compile_definitions(StandardLib PRIVATE $<$<CONFIG:Debug>:DEBUG_POINTER>)

add_library(SupportLib STATIC ${SUPPORTLIB_SOURCES})
target_link_libraries(SupportLib PUBLIC StandardLib BaseLib)

add_library(RasterLib STATIC ${RASTERLIB_SOURCES})
target_link_libraries(RasterLib PUBLIC StandardLib BaseLib)

add_library(RenderLib STATIC ${RENDERLIB_SOURCES})
target_link_libraries(RenderLib PUBLIC SupportLib StandardLib BaseLib)

add_library(RasterRenderLib STATIC ${RASTERRENDERLIB_SOURCES})
target_link_libraries(RasterRenderLib PUBLIC RenderLib RasterLib SupportLib StandardLib BaseLib)

# Only Vulkan's headers (the SDK or libvulkan-dev): VulkanLib finds its functions in libvulkan.so.1 while it runs.
find_path(VULKAN_INCLUDE_DIR vulkan/vulkan.h HINTS "$ENV{VULKAN_SDK}/include" REQUIRED)
add_library(VulkanLib STATIC ${VULKANLIB_SOURCES})
target_include_directories(VulkanLib PUBLIC "${VULKAN_INCLUDE_DIR}")
target_link_libraries(VulkanLib PUBLIC RenderLib SupportLib StandardLib BaseLib)

add_library(InputLib STATIC ${INPUTLIB_SOURCES})
target_link_libraries(InputLib PUBLIC StandardLib BaseLib)

add_library(WindowLib STATIC ${WINDOWLIB_SOURCES})
target_link_libraries(WindowLib PUBLIC InputLib SupportLib StandardLib BaseLib)

add_library(WinRefLib STATIC ${WINREFLIB_SOURCES})
target_link_libraries(WinRefLib PUBLIC WindowLib SupportLib StandardLib BaseLib)

add_library(SceneViewLib STATIC ${SCENEVIEWLIB_SOURCES})
target_link_libraries(SceneViewLib PUBLIC WindowLib InputLib VulkanLib RasterRenderLib RenderLib RasterLib SupportLib StandardLib BaseLib)

add_library(ProceduralLib STATIC ${PROCEDURALLIB_SOURCES})
target_link_libraries(ProceduralLib PUBLIC SupportLib StandardLib BaseLib)

# The bake's rows are shared among a pool of threads (CWorkerThreadJobs).
add_library(ThreadLib STATIC ${THREADLIB_SOURCES})
target_link_libraries(ThreadLib PUBLIC BaseLib)

add_library(SculptLib STATIC ${SCULPTLIB_SOURCES})
target_link_libraries(SculptLib PUBLIC SceneViewLib WindowLib InputLib VulkanLib ProceduralLib RasterRenderLib RenderLib RasterLib SupportLib StandardLib BaseLib)

add_executable(TestSculptLib ${TESTSCULPTLIB_SOURCES})
target_link_libraries(TestSculptLib PRIVATE TestLib SculptLib SceneViewLib WinRefLib WindowLib InputLib VulkanLib RasterRenderLib RenderLib RasterLib SupportLib StandardLib ThreadLib BaseLib)

# The tests read their input (Input/ ...) relative to the project directory,
# as Visual Studio does.
enable_testing()
add_test(NAME TestSculptLib COMMAND TestSculptLib WORKING_DIRECTORY "${TESTSCULPTLIB_DIR}")
