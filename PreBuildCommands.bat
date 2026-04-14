@echo off

set CONFIG=%1
if "%CONFIG%"=="" (
    set CONFIG=Debug
)

set GLFW_BUILD_DIR=%cd%\Vendor\GLFW\build
set GLFW_LIB_DIR=%GLFW_BUILD_DIR%\src\%CONFIG%
set GLFW_LIB_DEST=%cd%\Vendor\Libs\GLFW\
set GLFW_LIB_DEST_LIB=%GLFW_LIB_DEST%\%CONFIG%

if not exist "%GLFW_LIB_DEST_LIB%" (
    echo Building GLFW %CONFIG%...

    cmake -S Vendor\glfw -B "%GLFW_BUILD_DIR%" -G "Visual Studio 17 2022" -A x64 ^
        -DBUILD_SHARED_LIBS=OFF ^
        -DGLFW_BUILD_EXAMPLES=OFF ^
        -DGLFW_BUILD_TESTS=OFF ^
        -DGLFW_BUILD_DOCS=OFF
    cmake --build "%GLFW_BUILD_DIR%" --config %CONFIG%

    mkdir %GLFW_LIB_DEST%
    :: move only the lib file in a separate directory (Vendor\Libs\GLFW\)
    move %GLFW_LIB_DIR% %GLFW_LIB_DEST%
    :: remove build directory, it can be fairly large, and we dont need it anymore
    rmdir /s /q %GLFW_BUILD_DIR%
) else (
    echo GLFW already built
)