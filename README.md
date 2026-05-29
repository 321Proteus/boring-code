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

