# ELM

This is a renderer for discontinuous finite elements aiming for interactive use.
It (currently) uses some form of ray casting to generate slices, surfaces, and isosurfaces.
Ray casting ensures render time scales strongly with screen size and weakly with computational domain size (with the correct acceleration structures).

## Building

This program has been tested on Linux and macOS.
In theory it should work on Windows but I've never tried.

Building uses GNU Make with a supporting `local.mk` configuration file.
An example local.mk file is provided to get started.
You'll need a working Vulkan 1.3 implementation, GLFW, and glm to build the main program.
Shader compilation currently requries glslc because I couldn't live without `#include`.

### macOS

Apple does not provide a Vulkan implementation, but Vulkan is still usable through the MoltenVK compatibility layer.
The Vulkan SDK for macOS, including MoltenVK and glslc, can be downloaded from LunarG.

## Examples

TODO

## Controls

TODO
