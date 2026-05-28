# boring-code
Ever felt overwhelmed by the CFG complexity when analyzing an obfuscated code trace? My extremely WIP tool is here to help!

## Features
    - Simple and thread-safe trace collection with reasonably low overhead
    - Basic CF compression and restoration by loop collapsing and block adjacency mapping
    - Detailed information and scoring for every analysed block
    - Attaching a binary alongside a trace allows for assembly preview for procedures
    - Fully functional search 

## To Do
    - Block/procedure renaming
    - Trace diffing
    - Fix tracing 
    - Added library dump (currently, all calls to outside of the image are not available in the code preview)
    - Allow to store multiple traces/binaries per project (to implement the previous point)
    - Fix the build system - BoringCode was originally intended to generate traces on Windows and analyse them on Linux and therefore feels chaotic at this point

## Dependencies
  - BoringTool deps:
    - [https://dynamorio.org/](DynamoRIO)
  - BoringCode deps:
    - [https://www.qt.io/](Qt6)
    - [https://www.capstone-engine.org/](capstone)
  - Common deps:
    - [https://www.zlib.net/](zlib)

