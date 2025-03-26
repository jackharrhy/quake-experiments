cmake_minimum_required(VERSION 3.10)
project(PakViewer)

set(CMAKE_CXX_STANDARD 17)
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(ZLIB REQUIRED)

# Add ImGui library
add_library(imgui
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_demo.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_glfw.cpp
    third_party/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ${CMAKE_SOURCE_DIR}/third_party/imgui
)

# Add tinyfiledialogs library
add_library(tinyfiledialogs
    third_party/tinyfiledialogs/tinyfiledialogs.c
)

target_include_directories(tinyfiledialogs PUBLIC
    ${CMAKE_SOURCE_DIR}/third_party/tinyfiledialogs
)

# Link GLFW with ImGui
target_link_libraries(imgui PUBLIC glfw)

add_executable(PakViewer main.cpp)
target_link_libraries(PakViewer glfw OpenGL::GL ZLIB::ZLIB imgui tinyfiledialogs)
