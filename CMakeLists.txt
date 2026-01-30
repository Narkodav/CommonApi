set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ON CACHE BOOL "" FORCE)
cmake_minimum_required(VERSION 3.10)
project(CommonApi)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SRC_FILES
    ${CMAKE_SOURCE_DIR}/src/**/*.cpp
)

option(BUILD_SHARED_LIBS "Build shared library" ON)

# Add executable
add_library(${PROJECT_NAME}
    ${SRC_FILES}
)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wextra -Wpedantic -Werror
    )
elseif (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4 /WX
    )
endif()

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE COMMON_API_EXPORTS
)

# Include directories
target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Dependencies
set(API_DIR E:/Program\ Files\ \(x86\)/API)
set(LOCAL_API_DIR E:/Program\ Files\ \(x86\)/Code/C_code/libraries)
message(STATUS "Api directory: ${API_DIR}")

# GLM
set(glm_DIR ${API_DIR}/glm/cmake/glm)
find_package(glm REQUIRED)
target_link_libraries(${PROJECT_NAME}
    PUBLIC glm::glm
)

target_include_directories(${PROJECT_NAME}
    SYSTEM PUBLIC ${GLM_INCLUDE_DIRS}
)

message(STATUS "GLM include dirs: ${GLM_INCLUDE_DIRS}")

# Install
install(TARGETS ${PROJECT_NAME}
    EXPORT CommonApiTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include
)
