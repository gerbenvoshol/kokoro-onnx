cmake_minimum_required(VERSION 3.15)
project(kokoro_c VERSION 1.0.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Options
option(BUILD_SHARED_LIBS "Build shared library" ON)
option(BUILD_EXAMPLES "Build example programs" ON)

# Find dependencies
find_library(ONNXRUNTIME_LIB onnxruntime 
    HINTS 
    /usr/lib 
    /usr/local/lib
    $ENV{ONNXRUNTIME_DIR}/lib
)

find_path(ONNXRUNTIME_INCLUDE_DIR onnxruntime_c_api.h
    HINTS 
    /usr/include 
    /usr/local/include
    $ENV{ONNXRUNTIME_DIR}/include
)

find_library(ESPEAK_LIB espeak-ng
    HINTS 
    /usr/lib 
    /usr/local/lib
)

find_path(ESPEAK_INCLUDE_DIR espeak-ng/speak_lib.h
    HINTS 
    /usr/include 
    /usr/local/include
)

# Check if dependencies were found
if(NOT ONNXRUNTIME_LIB)
    message(FATAL_ERROR "ONNX Runtime library not found. Please install ONNX Runtime or set ONNXRUNTIME_DIR")
endif()

if(NOT ONNXRUNTIME_INCLUDE_DIR)
    message(FATAL_ERROR "ONNX Runtime headers not found. Please install ONNX Runtime or set ONNXRUNTIME_DIR")
endif()

if(NOT ESPEAK_LIB)
    message(WARNING "espeak-ng library not found. Audio synthesis may not work properly.")
endif()

if(NOT ESPEAK_INCLUDE_DIR)
    message(WARNING "espeak-ng headers not found. Audio synthesis may not work properly.")
endif()

# Library source files
set(KOKORO_SOURCES
    src/kokoro.c
)

set(KOKORO_HEADERS
    include/kokoro.h
)

# Create library
add_library(kokoro ${KOKORO_SOURCES} ${KOKORO_HEADERS})

# Include directories
target_include_directories(kokoro
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${ONNXRUNTIME_INCLUDE_DIR}
        ${ESPEAK_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(kokoro
    PRIVATE
        ${ONNXRUNTIME_LIB}
        ${ESPEAK_LIB}
        m  # Math library
)

# Set library properties
set_target_properties(kokoro PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER include/kokoro.h
)

# Build examples
if(BUILD_EXAMPLES)
    add_executable(kokoro_example examples/example.c)
    target_link_libraries(kokoro_example PRIVATE kokoro)
    target_include_directories(kokoro_example PRIVATE include)
    
    add_executable(kokoro_compile_test examples/compile_test.c)
    target_link_libraries(kokoro_compile_test PRIVATE kokoro)
    target_include_directories(kokoro_compile_test PRIVATE include)
endif()

# Installation
include(GNUInstallDirs)

install(TARGETS kokoro
    EXPORT kokoroTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install CMake package config
install(EXPORT kokoroTargets
    FILE kokoroTargets.cmake
    NAMESPACE kokoro::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/kokoro
)

# Create package config
include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/kokoroConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/kokoroConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/kokoro
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/kokoroConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/kokoroConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/kokoroConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/kokoro
)
