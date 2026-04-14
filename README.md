# Dearer ImGui

TODO: write stuff

## Build

### Windows - Visual Studio

To use this in Visual Studio just git clone the project in the root directory of 
solution, add it as existing project and add a reference to it in the project
that needs to use it.

> Note to myself: Compilation of GLFW is handled by PreBuildCommands.bat

### Linux

Requires CMake.

[Template_CMakeLists.txt](Res/Template_CMakeLists.txt) is a minimal template 
for a CMake file that links DearerImGui. Just adjust the `DEARER_IMGUI_DIR`
variable to the path where the repo was cloned

