cmake_minimum_required(VERSION 3.20)
project(CommonApi LANGUAGES CXX)

# =========================
# Source files for library
# =========================
file(GLOB_RECURSE SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

option(COMMONAPI_BUILD_SHARED "Build CommonApi as a shared library" OFF)

# =========================
# Library target
# =========================
if(COMMONAPI_BUILD_SHARED)
    add_library(${PROJECT_NAME} SHARED ${SRC_FILES})
else()
    add_library(${PROJECT_NAME} STATIC ${SRC_FILES})
endif()

add_library(CommonApi::CommonApi ALIAS ${PROJECT_NAME})

# C++20 and warnings
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# Preprocessor definition
target_compile_definitions(${PROJECT_NAME} PRIVATE COMMON_API_EXPORTS)

# Include directories
target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# =========================
# GLM (INTERFACE library)
# =========================
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Vendor/glm>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(${PROJECT_NAME} PUBLIC glm)

# =========================
# Test executable
# =========================
file(GLOB_RECURSE TEST_SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp
)

add_executable(CommonApiTests ${TEST_SRC_FILES})

# Link the library
target_link_libraries(CommonApiTests PRIVATE ${PROJECT_NAME})

# Match C++ standard and warnings
target_compile_features(CommonApiTests PUBLIC cxx_std_20)
target_compile_options(CommonApiTests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

target_include_directories(CommonApiTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Enable CTest
enable_testing()
add_test(NAME CommonApiTests COMMAND CommonApiTests)

# =========================
# Install rules
# =========================
install(TARGETS ${PROJECT_NAME} glm
    EXPORT CommonApiTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/glm/glm
    DESTINATION include
)

install(EXPORT CommonApiTargets
    FILE CommonApiTargets.cmake
    NAMESPACE CommonApi::
    DESTINATION lib/cmake/CommonApi
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/install/CommonApiConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/CommonApiConfig.cmake
    INSTALL_DESTINATION lib/cmake/CommonApi
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/CommonApiConfig.cmake
    DESTINATION lib/cmake/CommonApi
)