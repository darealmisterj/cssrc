cmake_minimum_required(VERSION 3.15)
project(DX11ImGuiApp)

# Specify C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable solution folders for Visual Studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Configure output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Add source files
set(SOURCES
    src/main.cpp
    src/ResourceManager.cpp
    src/UIManager.cpp
    src/Config.cpp
    src/ErrorHandler.cpp
    src/MenuSystem.cpp
)

set(HEADERS
    include/ResourceManager.h
    include/UIManager.h
    include/Config.h
    include/ErrorHandler.h
    include/MenuSystem.h
)

# Find DirectX11
find_package(DirectX REQUIRED)

# Add ImGui as a subdirectory (assuming it's in a subdirectory called 'imgui')
add_subdirectory(imgui)

# Create executable
add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${DirectX_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    imgui
    d3d11
    d3dcompiler
    dxgi
)

# Set Windows subsystem
set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE}"
)

# Copy ImGui shaders and resources if needed
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE}/resources
)

# Enable Unity build for faster compilation (optional)
set_target_properties(${PROJECT_NAME} PROPERTIES UNITY_BUILD ON)

# Set warning level
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

