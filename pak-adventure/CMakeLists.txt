cmake_minimum_required(VERSION 3.10)
project(QuakePakViewer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Common source files
set(SOURCES
    main.cpp
    platform.cpp
)

if(EMSCRIPTEN)
    # Emscripten specific settings
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    
    # Add ImGui source files directly
    set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/third_party/imgui)
    set(IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    )
    
    # Add Emscripten specific files
    set(SOURCES ${SOURCES} web_main.cpp ${IMGUI_SOURCES})
    
    # Create the web target
    add_executable(${PROJECT_NAME} ${SOURCES})
    
    # Add include directories
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
    )
    
    # Set Emscripten linker flags
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "-sUSE_WEBGL2=1 -sWASM=1 -sALLOW_MEMORY_GROWTH=1 -sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','UTF8ToString'] -sEXPORTED_FUNCTIONS=['_malloc','_free','_init_web','_main'] -sUSE_GLFW=3 -sUSE_ZLIB=1 -sMAX_WEBGL_VERSION=2 -sMIN_WEBGL_VERSION=2 -sOFFSCREENCANVAS_SUPPORT=1 -sGL_DEBUG=1 -sASSERTIONS=2 -sINVOKE_RUN=0 -sWASM_ASYNC_COMPILATION=1 --shell-file ${CMAKE_SOURCE_DIR}/shell.html --bind"
    )
    
    # Copy necessary files to the build directory
    configure_file(${CMAKE_SOURCE_DIR}/index.html ${CMAKE_BINARY_DIR}/index.html COPYONLY)
else()
    # Desktop dependencies
    find_package(OpenGL REQUIRED)
    find_package(glfw3 REQUIRED)
    find_package(Imgui REQUIRED)
    find_package(tinyfiledialogs REQUIRED)
    
    # Desktop target
    add_executable(${PROJECT_NAME} ${SOURCES})
    target_link_libraries(${PROJECT_NAME} PRIVATE 
        OpenGL::GL 
        glfw 
        ImGui::ImGui 
        tinyfiledialogs
    )
endif()
