find_package(glfw3 REQUIRED)
set(OpenGL_GL_PREFERENCE "LEGACY")
find_package(OpenGL 2 REQUIRED)

include_directories("../../")
add_executable(imgui)
target_sources(imgui PRIVATE
  main.cpp
  imgui_impl_glfw_gl3.cpp
  ../../glad/glad.c
  ../../imgui/imgui.cpp
  ../../imgui/imgui_draw.cpp
  ../../imgui/imgui_tables.cpp
  ../../imgui/imgui_widgets.cpp)
target_include_directories(imgui SYSTEM PRIVATE ../../imgui)
target_link_libraries(imgui PUBLIC glfw ${OPENGL_LIBRARIES})
