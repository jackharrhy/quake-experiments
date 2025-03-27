#!/bin/bash
set -e

# Create build directory if it doesn't exist
mkdir -p builddir

# Navigate to build directory
cd builddir

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build the application
cmake --build . -j$(sysctl -n hw.ncpu)

# Create dist directory
mkdir -p ../dist

# Manually copy the app bundle before running install
echo "Copying app bundle to dist directory..."
cp -R PakViewer.app ../dist/

# Manually copy the libzip library to the Frameworks directory
mkdir -p ../dist/PakViewer.app/Contents/Frameworks
if [ -f third_party/libzip/lib/libzip.5.dylib ]; then
    echo "Copying libzip.5.dylib to Frameworks directory..."
    cp third_party/libzip/lib/libzip.5.dylib ../dist/PakViewer.app/Contents/Frameworks/
    
    # Fix the library paths
    echo "Fixing library paths with install_name_tool..."
    install_name_tool -change @rpath/libzip.5.dylib @executable_path/../Frameworks/libzip.5.dylib ../dist/PakViewer.app/Contents/MacOS/PakViewer
fi

# Copy other dylib files if they exist
if [ -f libimgui.dylib ]; then
    echo "Copying imgui library..."
    cp libimgui.dylib ../dist/PakViewer.app/Contents/Frameworks/
    install_name_tool -change @rpath/libimgui.dylib @executable_path/../Frameworks/libimgui.dylib ../dist/PakViewer.app/Contents/MacOS/PakViewer
fi

if [ -f libtinyfiledialogs.dylib ]; then
    echo "Copying tinyfiledialogs library..."
    cp libtinyfiledialogs.dylib ../dist/PakViewer.app/Contents/Frameworks/
    install_name_tool -change @rpath/libtinyfiledialogs.dylib @executable_path/../Frameworks/libtinyfiledialogs.dylib ../dist/PakViewer.app/Contents/MacOS/PakViewer
fi

# Run codesign to make the app valid (optional)
if command -v codesign &> /dev/null; then
    echo "Signing application..."
    codesign --force --deep --sign - ../dist/PakViewer.app
fi

echo "Build complete! Your application is now available at $(pwd)/../dist/PakViewer.app"
echo "To run the application, use: open ../dist/PakViewer.app"
echo "To create a distributable DMG, you can use create-dmg:"
echo "brew install create-dmg"
echo "create-dmg --volname \"PakViewer\" --window-pos 200 120 --window-size 600 300 --icon-size 100 --icon \"PakViewer.app\" 200 150 --hide-extension \"PakViewer.app\" --app-drop-link 400 150 \"PakViewer.dmg\" \"../dist/\"" 