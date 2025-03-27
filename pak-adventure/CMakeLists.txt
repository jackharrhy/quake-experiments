cmake_minimum_required(VERSION 3.10)
project(PakViewer)

set(CMAKE_CXX_STANDARD 17)
# Add macOS specific settings
set(MACOSX_BUNDLE_ICON_FILE PakViewer.icns)
set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.yourname.pakviewer")
set(MACOSX_BUNDLE_BUNDLE_NAME "PakViewer")
set(MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0")
set(MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.0")

# Make sure RPATH is properly set for macOS bundle
if(APPLE)
    set(CMAKE_INSTALL_RPATH "@executable_path/../Frameworks")
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
endif()

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(ZLIB REQUIRED)

# Add libzip from submodule
add_subdirectory(third_party/libzip)

# Add ImGui library
add_library(imgui
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_demo.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_glfw.cpp
    third_party/imgui/backends/imgui_impl_opengl3.cpp
    third_party/imgui/misc/cpp/imgui_stdlib.cpp
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

# Set MACOSX_BUNDLE to TRUE for .app creation on macOS
add_executable(PakViewer MACOSX_BUNDLE main.cpp)
target_include_directories(PakViewer PRIVATE 
    ${CMAKE_SOURCE_DIR}/third_party/libzip/lib
    ${CMAKE_SOURCE_DIR}/third_party/stb
)
target_link_libraries(PakViewer glfw OpenGL::GL ZLIB::ZLIB zip imgui tinyfiledialogs)

# macOS-specific bundle configuration
if(APPLE)
    # Create Resources directory in the bundle
    set(APP_ICON_PATH ${CMAKE_SOURCE_DIR}/PakViewer.icns)
    set_source_files_properties(${APP_ICON_PATH} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    
    # Install bundle to appropriate location
    install(TARGETS PakViewer
        BUNDLE DESTINATION .
        RUNTIME DESTINATION bin
    )
    
    # Install required libraries into the bundle
    install(TARGETS zip imgui tinyfiledialogs
        LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/PakViewer.app/Contents/Frameworks
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/PakViewer.app/Contents/Frameworks
    )
    
    # Custom command to copy dynamic libraries into the bundle
    install(CODE "
        include(BundleUtilities)
        set(BU_CHMOD_BUNDLE_ITEMS TRUE)
        set(APPS \"\${CMAKE_INSTALL_PREFIX}/PakViewer.app\")
        set(LIBS_PATHS \"\${CMAKE_BINARY_DIR}\" \"\${CMAKE_BINARY_DIR}/third_party/libzip/lib\")
        
        # Function to copy a library into the bundle
        function(copy_library_into_bundle lib_path bundle_path)
            if(EXISTS \"\${lib_path}\")
                file(COPY \"\${lib_path}\" 
                     DESTINATION \"\${bundle_path}/Contents/Frameworks\"
                     FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE 
                                      GROUP_READ GROUP_EXECUTE 
                                      WORLD_READ WORLD_EXECUTE
                     FOLLOW_SYMLINK_CHAIN)
                execute_process(COMMAND install_name_tool -change @rpath/\${lib_path} @executable_path/../Frameworks/\${lib_path} \${bundle_path}/Contents/MacOS/PakViewer)
            endif()
        endfunction()
        
        # Copy libzip directly
        copy_library_into_bundle(\"libzip.5.dylib\" \"\${APPS}\")
        
        # Use BundleUtilities to grab remaining dependencies
        fixup_bundle(\"\${APPS}\" \"\" \"\${LIBS_PATHS}\")
        " COMPONENT Runtime)
endif()
