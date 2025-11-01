# ASCII Vid Converter - build instructions

This repository contains an ASCII video converter project. The CMake setup created here is a minimal scaffold that:

- Builds sources under `src/` (a minimal `main.cpp` is created if none exist).
- Attempts to use the bundled FFmpeg headers and libraries under `3rd_party/ffmpeg_static` by default.
- Includes `stb_image` from `3rd_party/stb_image` if present.

Quick build (PowerShell):

```powershell
mkdir build; cd build
cmake -G "NMake Makefiles" ..
cmake --build . --config Release
```

Or use the provided VS/GCC tasks in VS Code (Build with MSVC / Build with GCC).

If you have FFmpeg installed system-wide and want to use it, set the CMake option `-DUSE_SYSTEM_FFMPEG=ON` or set `-DFFMPEG_ROOT=` to the bundle location.
