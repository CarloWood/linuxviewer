cmake_minimum_required(VERSION 3.12...3.19)

include(AICxxProject)

find_package(Vulkan REQUIRED COMPONENTS glslc)
find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)

find_package(Eigen3 3.3 REQUIRED NO_MODULE)
find_package(Boost REQUIRED COMPONENTS serialization)

find_package(PkgConfig REQUIRED)
find_package(magic_enum REQUIRED)

# Prepend shaderc_ROOT in case this package was installed by gitache.
set(stored_CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}")
list(PREPEND CMAKE_PREFIX_PATH "${shaderc_ROOT}")
pkg_check_modules(Shaderc REQUIRED IMPORTED_TARGET shaderc)
set(CMAKE_PREFIX_PATH "${stored_CMAKE_PREFIX_PATH}")

if (OptionEnableLibcwd)
  pkg_check_modules(Libxmlpp30 libxml++-3.0 IMPORTED_TARGET)
  if (Libxmlpp30_FOUND)
    set(LIBXMLPP PkgConfig::Libxmlpp30)
    set(LIBXMLPP_VERSION 30)
  else ()
    pkg_check_modules(Libxmlpp26 libxml++-2.6 IMPORTED_TARGET)
    if (Libxmlpp26_FOUND)
      set(LIBXMLPP PkgConfig::Libxmlpp26)
      set(LIBXMLPP_VERSION 26)
    else ()
      message(FATAL_ERROR "Neither libxml++-3.0 nor libxml++-2.6 could be found!")
    endif ()
  endif ()
endif ()

#==============================================================================
# BUILD PROJECT
#

# This project is an OBJECT-library, used by other git submodules and the main project.
add_library(vulkan_ObjLib OBJECT)

# Set SOURCE_DIR to the current directory.
set_property(
  TARGET vulkan_ObjLib
  PROPERTY SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
)

file(GLOB SOURCE_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/rendergraph/*.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/*.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/partitions/*.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/queues/*.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/memory/*.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/vk_utils/*.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/infos/*.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/math/*.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/descriptor/*.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/tracy/*.cxx
)

file(GLOB HEADER_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/rendergraph/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/partitions/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/queues/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/memory/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vk_utils/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/infos/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/math/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/descriptor/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/tracy/*.h
)

# The list of source files.
target_sources(vulkan_ObjLib
  PRIVATE
    ${SOURCE_FILES}
    ${HEADER_FILES}
)

# Add debug source files.
if (OptionEnableLibcwd)
  file(GLOB DEBUG_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/debug/*.cxx
  )

  file(GLOB DEBUG_HEADER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/debug/*.h
  )

  # Add to the list of source files.
  target_sources(vulkan_ObjLib
    PRIVATE
      ${DEBUG_SOURCE_FILES}
      ${DEBUG_HEADER_FILES}
  )
endif ()

# Required include search-paths.
get_target_property(CWDS_INTERFACE_INCLUDE_DIRECTORIES AICxx::cwds INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(vulkan_ObjLib
  PRIVATE
    "${CMAKE_SOURCE_DIR}/external/peelo-xdg/include"
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CWDS_INTERFACE_INCLUDE_DIRECTORIES}" # For sys.h and debug.h.
)

target_compile_definitions(vulkan_ObjLib
  PUBLIC
    VK_USE_PLATFORM_XCB_KHR
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
    VULKAN_HPP_NO_UNION_CONSTRUCTORS
)

# Require support for C++20.
target_compile_features(vulkan_ObjLib
  PUBLIC cxx_std_20
)

# Find where vulkan_enums.hpp resides.
get_target_property(VULKAN_INCLUDE_DIRS Vulkan::Vulkan INTERFACE_INCLUDE_DIRECTORIES)
find_path(VULKAN_ENUMS_DIR "vulkan/vulkan_enums.hpp" PATHS ${VULKAN_INCLUDE_DIRS})

# Define the input and output files.
set(INPUT_FILE "${VULKAN_ENUMS_DIR}/vulkan/vulkan_enums.hpp")
set(OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/debug/vk_enums.h")

# Add a custom command to generate the output file.
add_custom_command(
  OUTPUT ${OUTPUT_FILE}
  COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/debug"
  COMMAND awk -f '${CMAKE_SOURCE_DIR}/scripts/enum_converter.awk' '${INPUT_FILE}' > '${OUTPUT_FILE}'
  DEPENDS "${INPUT_FILE}" "${CMAKE_SOURCE_DIR}/scripts/enum_converter.awk"
  COMMENT "Generating Vulkan enums header"
)

# Custom target for vk_enums.h.
add_custom_target(generate_vk_enums DEPENDS ${OUTPUT_FILE})

# Make sure that vk_enums.h is generated whenever building vulkan_ObjLib.
add_dependencies(vulkan_ObjLib generate_vk_enums)

# Needed in order to include debug/vk_enums.h.
target_include_directories(vulkan_ObjLib PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# Add a custom target that depends on the output file.
#add_custom_target(vulkan_enums DEPENDS ${OUTPUT_FILE})

# Set link dependencies.
target_link_libraries(vulkan_ObjLib
  PUBLIC
    LinuxViewer::shader_builder
    VulkanMemoryAllocator
    Vulkan::Vulkan
    AICxx::xcb-task
    AICxx::xcb-task::OrgFreedesktopXcbError
    AICxx::utils
    farmhash::farmhash
    ${CMAKE_DL_LIBS}
    PkgConfig::Shaderc
    ImGui::imgui
    Eigen3::Eigen
    Boost::serialization
    Tracy::TracyClient
    ${LIBXMLPP}
)

# Create an ALIAS target.
add_library(LinuxViewer::vulkan ALIAS vulkan_ObjLib)

# Test executable.
add_executable(allocator_test tests/allocator_test.cxx)
target_link_libraries(allocator_test ${AICXX_OBJECTS_LIST})

# Math library.
add_subdirectory(math)
add_subdirectory(shader_builder)
