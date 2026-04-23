cmake_minimum_required(VERSION 3.20)

project(DearerImGui LANGUAGES C CXX)

# ==================================================
# C/C++ Standard
# ==================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)


# ==================================================
# Source Files
# ==================================================
file(GLOB_RECURSE CPP_SOURCES
	Src/*.cpp
)

# ==================================================
# ImGui
# ==================================================
set(IMGUI_DIR Vendor/ImGui)

set(IMGUI_SOURCES
	${IMGUI_DIR}/imgui.cpp
	${IMGUI_DIR}/imgui_draw.cpp
	${IMGUI_DIR}/imgui_tables.cpp
	${IMGUI_DIR}/imgui_widgets.cpp
	${IMGUI_DIR}/imgui_demo.cpp
	${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
	${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)


# ==================================================
# GLAD
# ==================================================
add_library(glad STATIC
	Vendor/GLAD/src/glad.c
)
	
target_include_directories(glad PUBLIC
	Vendor/GLAD/include
)

# ==================================================
# GLFW
# ==================================================
add_subdirectory(Vendor/GLFW)

# ==================================================
# Create static library
# ==================================================
add_library(${PROJECT_NAME} STATIC
	${CPP_SOURCES}
	${IMGUI_SOURCES}
)
add_library(DearerImGui::DearerImGui ALIAS ${PROJECT_NAME})

# ==================================================
# Includes
# ==================================================
target_include_directories(${PROJECT_NAME} PUBLIC
	Src
	Vendor/ImGui
	Vendor/ImGui/backends
	Vendor/GLAD/include
	Vendor/GLFW/include
)

# ==================================================
# Link Libraries
# ==================================================
target_link_libraries(${PROJECT_NAME} PUBLIC
	glad
	glfw
)

# ==================================================
# Platform-specific OpenGL
# ==================================================
if (APPLE)
	target_link_libraries(${PROJECT_NAME}
		"-framework OpenGL"
	)
elseif (WIN32)
	find_package(OpenGL REQUIRED)
	target_link_libraries(${PROJECT_NAME} PUBLIC
		OpenGL::GL
	)
else ()
	find_package(OpenGL REQUIRED)
	find_package(X11 REQUIRED)
	
	target_link_libraries(${PROJECT_NAME}
		OpenGL::GL
		X11::X11
	)
endif()