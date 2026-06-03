# boring-code
Ever felt overwhelmed by the CFG complexity when analyzing an obfuscated code trace? My extremely WIP tool is here to help!

## Features
  - Simple and thread-safe trace collection with reasonably low overhead
  - Basic CF compression and restoration by loop collapsing and block adjacency mapping
  - Detailed information and scoring for every analysed block
  - Attaching a binary alongside a trace allows for assembly preview for procedures
  - Fully functional search bar

## To Do
  - Block/procedure renaming
  - Trace diffing
  - Fix tracing
  - Advanced analysis and deobfuscation option
  - Added library dump (currently, all calls to outside of the image are not available in the code preview)
  - Allow to store multiple traces/binaries per project (to implement the previous point)
  - Fix the build system - BoringCode was originally intended to generate traces on Windows and analyze them on Linux and therefore the builds feel chaotic at this point
  - Target BoringCode as an IDA/Ghidra plugin

## Dependencies
  - BoringTool deps:
    - [DynamoRIO](https://dynamorio.org/)
  - BoringCode deps:
    - [Qt6](https://www.qt.io/)
    - [capstone](https://www.capstone-engine.org/)
  - Common deps:
    - [zlib](https://www.zlib.net/)

## Setup and Contribution
BoringCode is built by [CMake](https://cmake.org/) with [Ninja](https://ninja-build.org/) as the generator. By default, the buildscript targets all components at x64, while x86 is only enabled for the tracer.

To build, run `build.(sh/bat) <target> [--rebuild]`, where the target is the desired CMake preset.

NOTE: After configuring and building, the command updates the clangd configuration by copying `.clangd-template` and appending the respective `compilationDatabase` to the config file, which resets the clangd server (in case it didn't refresh automatically, you can use Ctrl+Shift+P > "Restart Language Server" in VS Code). On Windows, it also sets MSVC environmental variables by running the matching `vcvars.bat`. **TL;DR: `.clangd` is not the place for saving configs, `.clangd_template` is!** 

On Linux, dependencies can be quickly fetched by installing `qtcreator` (recommended, but `qt6-base-dev` should be enough), `dynamorio`, `pkgconf`, `libcapstone-dev` and `zlib` from your favourite package manager.

On Windows, we recommend downloading:
  - Qt using the [community installer](https://www.qt.io/development/download-qt-installer-oss)
  - `pkgconf`, `capstone` and `zlib` from [vcpkg](https://vcpkg.io/) (**Remember to fetch from both `x86-windows` and `x64-windows` triplets!**)
  - DynamoRIO from their [releases page](https://dynamorio.org/page_releases.html)
The default dependency paths are listed as variables in `build.bat` - you can follow or change them to match your real locations.