# PakViewer

A viewer for Quake PAK files.

## Building for macOS

This project uses CMake for building and packaging on macOS:

1. Run the build script:

   ```bash
   ./build_macos_app.sh
   ```

2. The packaged application will be created in the `dist/PakViewer.app` directory.

3. To run the application:
   ```bash
   open dist/PakViewer.app
   ```

## Creating a distributable DMG

To create a disk image (DMG) for distribution:

1. Install create-dmg:

   ```bash
   brew install create-dmg
   ```

2. Create the DMG:
   ```bash
   create-dmg --volname "PakViewer" --window-pos 200 120 --window-size 600 300 \
      --icon-size 100 --icon "PakViewer.app" 200 150 --hide-extension "PakViewer.app" \
      --app-drop-link 400 150 "PakViewer.dmg" "dist/"
   ```

## Application Icon

The application expects an icon file named `PakViewer.icns` in the project root directory.
To create an .icns file from a PNG:

1. Create PNG images at the following sizes: 16x16, 32x32, 64x64, 128x128, 256x256, 512x512, and 1024x1024.
2. Create the .iconset directory:
   ```bash
   mkdir PakViewer.iconset
   ```
3. Copy your PNG files to the .iconset directory with specific names:
   ```bash
   cp icon_16x16.png PakViewer.iconset/icon_16x16.png
   cp icon_32x32.png PakViewer.iconset/icon_32x32.png
   cp icon_64x64.png PakViewer.iconset/icon_64x64@2x.png
   cp icon_128x128.png PakViewer.iconset/icon_128x128.png
   cp icon_256x256.png PakViewer.iconset/icon_256x256.png
   cp icon_512x512.png PakViewer.iconset/icon_512x512.png
   cp icon_1024x1024.png PakViewer.iconset/icon_512x512@2x.png
   ```
4. Use the iconutil command to generate the .icns file:
   ```bash
   iconutil -c icns PakViewer.iconset
   ```
5. Clean up:
   ```bash
   rm -rf PakViewer.iconset
   ```

## Packaging Details

The macOS app bundle is created using the following approach:

1. CMake's MACOSX_BUNDLE option is used to generate the initial .app structure
2. The build script manually copies all dependencies into the Frameworks directory
3. install_name_tool is used to fix library paths to use @executable_path/../Frameworks
4. The app is signed with an ad-hoc signature to avoid Gatekeeper warnings

For notarization and distribution on App Store, additional steps would be required.
