# Loft Viewer

Shows 3D glTF scenes.

## Building

### Linux

Download packages for Vulkan and SDL2. Then you should be good to go as normal with CMake.

### Windows

To build on Windows, you need to download SDL2 and Vulkan.

#### SDL

You can download SDL2 from https://github.com/libsdl-org/SDL/releases.
Choose the last version and download the version for your compiler.

If you are using IDE, use: \
**Visual Studio**: SDL2-2.28.5-win32-x64.zip \
**Rider**:  SDL2-devel-2.28.5-mingw.zip 

Then extract it into `examples/viewer/external/` or some other directory, but then edit variable `SDL2_PATH` in `examples/viewer/CMakeLists.txt` to point to SDL's cmake directory.

#### Vulkan

You can download Vulkan from https://vulkan.lunarg.com/. On Windows, you can download installer.

After installation, modify `examples/viewer/CMakeLists.txt` variable `VULKAN_PATH` to point to the installation. Most of the time, you will only need to edit the path to use correct version, because installer extracts to the same directory.

Then everything should be ready.